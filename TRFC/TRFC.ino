#include <Wire.h>
#include <EEPROM.h> // Built in EEPROM, currently unused tho
#include <SPI.h>
#include <SdFat.h>
#include "RTC.h"
#include "SparkFunBME280.h" // Sparkfun library for the pressure sensor inside the Pitot tube

#include "Pins.h"
#include "IMU.h"

uint64_t measurementStartTime; //Used to calc the actual update rate. Max is ~80,000,000ms in a 24 hour period.

TwoWire qwiic(1); // I²C bus on the Qwiic port, connected to pads 8 and 9
SdFat sd; // SD card interface, communicating via SPI
SdFile sensorDataFile; //File that all sensor data is written to
char sensorDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep

IMU imu;
BME280 pres;
APM3_RTC myRTC; // Integrated Artemis RTC

Uart SerialLog(1, BREAKOUT_PIN_RX, BREAKOUT_PIN_TX); // TX/RX marked on board, couold be used to connect a wireless module

char outputData[512 * 2]; // Global sensor data storage, factor of 512 for easier recording to SD in 512 chunks

unsigned int readingTime;
float sensors[13];

//Returns next available log file name
//Checks the spots in EEPROM for the next available LOG# file name
//Updates EEPROM and then appends to the new log file.
char* findNextAvailableLog(const char *fileLeader) {
  int newFileNumber = 1;

  SdFile newFile; //This will contain the file for SD writing

  if (newFileNumber < 2) //If the settings have been reset, let's warn the user that this could take a while!
  {
    Serial.println("Finding the next available log file.");
    Serial.println("This could take a long time if the SD card contains many existing log files.");
  }

  if (newFileNumber > 0)
    newFileNumber--; //Check if last log file was empty. Reuse it if it is.

  //Search for next available log spot
  static char newFileName[40];
  while (1)
  {
    sprintf(newFileName, "%s%05u.TXT", fileLeader, newFileNumber); //Splice the new file number into this file name. Max no. is 99999.

    if (sd.exists(newFileName) == false) break; //File name not found so we will use it.

    //File exists so open and see if it is empty. If so, use it.
    newFile.open(newFileName, O_READ);
    if (newFile.fileSize() == 0) break; // File is empty so we will use it. Note: we need to make the user aware that this can happen!

    newFile.close(); // Close this existing file we just opened.

    newFileNumber++; //Try the next number
    if (newFileNumber >= 100000) break; // Have we hit the maximum number of files?
  }
  
  newFile.close(); //Close this new file we just opened

  newFileNumber++; //Increment so the next power up uses the next file #

  // Have we hit the maximum number of files?
  if (newFileNumber >= 100000) {
    Serial.print("***** WARNING! File number limit reached! (Overwriting ");
    Serial.print(newFileName);
    Serial.println(") *****");
    newFileNumber = 100000; // This will overwrite Log99999.TXT next time thanks to the newFileNumber-- above
  }
  else {
    Serial.print("Logging to: ");
    Serial.println(newFileName);    
  }

  return (newFileName);
}

void setPowerLED(bool status) {
  pinMode(PIN_PWR_LED, OUTPUT);
  digitalWrite(PIN_PWR_LED, status); 
}

void setMicroSDPower(bool status) {
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, !status);
}

void setIMUPower(bool status) {
  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, status);
}

void setQwiicPower(bool status) {
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  digitalWrite(PIN_QWIIC_POWER, status);
}

/**
 * Turn on and off the integrated status LED
 * @param status true to turn on the LED
 */
void setStatusLED(bool status) {
  pinMode(PIN_STAT_LED, OUTPUT);
  digitalWrite(PIN_STAT_LED, status);
}

/**
 * Initializes the I²C bus on the Qwiic connector
 */
void beginQwiic() {
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  setQwiicPower(true);
  qwiic.begin();
  qwiic.setPullups(true); // Enable internal pull-up resistors on I²C bus
}

/**
 * Initializes micro SD card with exfat filesystem
 * @return true if all went well
 */
bool beginSD() {
  bool success = true;
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  delay(1);
  setMicroSDPower(true);

  // Max power up time for a standard micro SD is 250ms
  delay(10);
    
  //Standard SdFat initialization
  if (!sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24))) { 

    //Give SD more time to power up, then try again
    delay(250);

    // Trying again standard SdFat initialization
    if (!sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24))) {
      Serial.println("SD init failed (second attempt). Is card present? Formatted?");
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      success = false;
    }
  }

  // Change to root directory. All new file creation will be in root
  // checking success to only execute chdir if no error occurred so far
  if (success && !sd.chdir()) {
    Serial.println("SD change directory failed");
    success = false;
  }

  return success;
}

/** 
 * Add CIPO pull-up
 * @return true if all went well
 */
bool enableCIPOpullUp() {
  ap3_err_t retval = AP3_OK;
  am_hal_gpio_pincfg_t cipoPinCfg = AP3_GPIO_DEFAULT_PINCFG;
  cipoPinCfg.uFuncSel = AM_HAL_PIN_6_M0MISO;
  cipoPinCfg.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA;
  cipoPinCfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
  cipoPinCfg.uIOMnum = AP3_SPI_IOM;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  padMode(MISO, cipoPinCfg, &retval);
  return retval == AP3_OK;
}

void updateDataFileCreate(SdFile *dataFile) {
  myRTC.getTime(); //Get the RTC time so we can use it to update the create time
  //Update the file create time
  dataFile->timestamp(T_CREATE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
}

void beginDataLogging() {
    //If we don't have a file yet, create one. Otherwise, re-open the last used file
    if (strlen(sensorDataFileName) == 0)
      strcpy(sensorDataFileName, findNextAvailableLog("dataLog_m_"));

    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (sensorDataFile.open(sensorDataFileName, O_CREAT | O_APPEND | O_WRITE) == false) {
      Serial.println("Failed to create sensor data file");
      return;
    }

    updateDataFileCreate(&sensorDataFile); // Update the file create time & date
}

void setup() {
  setPowerLED(true); // We're up!
  setStatusLED(true); // Starting configuration

  String(0.0f); // just in case the arduino compiler is dumb

  Serial.begin(115200);
  SerialLog.begin(38400);
  SPI.begin(); //Needed if SD is disabled
  beginQwiic();
  beginSD();
  enableCIPOpullUp();
  analogReadResolution(14); //Increase from default of 10
  beginDataLogging();
  imu.begin();

  pres.setI2CAddress(0x76);

  if (pres.beginI2C(qwiic) == false) {
    Serial.println("The sensor did not respond. Please check wiring.");
    while(1); //Freeze
  }

  pres.setPressureOverSample(4);
  pres.setFilter(3);

  pres.setReferencePressure(102100.0); // set to correct value day of launch!!!

  setStatusLED(false); // Configuration ended, ready to go

  measurementStartTime = millis();
}

//Query each enabled sensor for its most recent data
void getData() {
  readingTime = micros();

  imu.update();
  sensors[0] = imu.acc_x;
  sensors[1] = imu.acc_y;
  sensors[2] = imu.acc_z;

  imu.update();
  sensors[3] = imu.gyr_x;
  sensors[4] = imu.gyr_y;
  sensors[5] = imu.gyr_z;

  imu.update();
  sensors[6] = imu.q1;
  sensors[7] = imu.q2;
  sensors[8] = imu.q3;

  sensors[9] = pres.readFloatPressure();
  sensors[10] = pres.readFloatAltitudeMeters();
  sensors[11] = pres.readTempC();
}

void dataToStr() {
  char tempData[50];
  outputData[0] = '\0'; //Clear string contents

  sprintf(tempData, "%lu,", readingTime);
  strcat(outputData, tempData);

  imu.update();
  sprintf(tempData, "%.2f,%.2f,%.2f,", sensors[0], sensors[1], sensors[2]);
  strcat(outputData, tempData);

  imu.update();
  sprintf(tempData, "%.2f,%.2f,%.2f,", sensors[3], sensors[4], sensors[5]);
  strcat(outputData, tempData);

  imu.update();
  sprintf(tempData, "%.2f,%.2f,%.2f,", sensors[6], sensors[7], sensors[8]);
  strcat(outputData, tempData);

  sprintf(tempData, "%.2f,", sensors[9]);
  strcat(outputData, tempData);
  
  sprintf(tempData, "%.2f,", sensors[10]);
  strcat(outputData, tempData);
  
  sprintf(tempData, "%.2f,", sensors[11]);
  strcat(outputData, tempData);

  strcat(outputData, "\r\n");
}

unsigned long lastReadTime = 0; //Used to delay until user wants to record a new reading
unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second
bool takeReading = true; //Goes true when enough time has passed between readings or we've woken from sleep

int sendfreq = 10;
int sentf = 0;
int sentfs = 0;

uint64_t hertz = 111;
uint64_t usBetweenReadings = 1000000ULL / hertz;

void loop() {
  if ((micros() - lastReadTime) >= usBetweenReadings) takeReading = true;

  //Is it time to get new data?
  if (takeReading) {
    takeReading = false;
    lastReadTime = micros();

    getData(); //Query all enabled sensors for data
    dataToStr();

    //Output to TX pin
    bool serialOut = true;
    if (serialOut && sentf >= sendfreq) {
      SerialLog.print(outputData);
    } else sentf++;

    //Print to terminal
    bool terminalOut = false;
    if (terminalOut && sentfs >=sendfreq) {
      Serial.print(outputData); //Print to terminal
    } else sentfs++;

    //Record to SD
    bool sdOut = true;
    if (sdOut) {
      digitalWrite(PIN_STAT_LED, HIGH);
      sensorDataFile.write(outputData, strlen(outputData)); //Record the buffer to the card
      //Force sync every 500ms
      if (millis() - lastDataLogSyncTime > 500) {
        lastDataLogSyncTime = millis();
        sensorDataFile.sync();
      }
      digitalWrite(PIN_STAT_LED, LOW);
    }
  }
}
