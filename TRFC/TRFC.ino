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

Uart SerialLog(1, 13, 12); // TX/RX marked on board, couold be used to connect a wireless module

char outputData[512 * 2]; // Global sensor data storage, factor of 512 for easier recording to SD in 512 chunks

unsigned int readingTime;
float sensors[13];

/**
 * Turn on and off the integrated status LED
 * @param status true to turn on the LED
 */
void setStatusLED(bool status) {
  pinMode(PIN_STAT_LED, OUTPUT);
  digitalWrite(PIN_STAT_LED, status);
}

void error() {
  setStatusLED(true);
  while(1) checkBattery(); 
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
  smartDelay(10);
    
  //Standard SdFat initialization
  if (!sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24))) { 

    //Give SD more time to power up, then try again
    smartDelay(250);

    // Trying again standard SdFat initialization
    if (!sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24))) {
      SerialPrintln(F("SD init failed (second attempt). Is card present? Formatted?"));
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      success = false;
    }
  }

  // Change to root directory. All new file creation will be in root
  // checking success to only execute chdir if no error occurred so far
  if (success && !sd.chdir()) {
    SerialPrintln(F("SD change directory failed"));
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
      SerialPrintln(F("Failed to create sensor data file"));
      return;
    }

    updateDataFileCreate(&sensorDataFile); // Update the file create time & date
}

void setup() {
  // Initializes low power detection
  pinMode(PIN_POWER_LOSS, INPUT); // BD49K30G-TL has CMOS output and does not need a pull-up
  delay(1); // Let PIN_POWER_LOSS stabilize
  if (digitalRead(PIN_POWER_LOSS) == LOW) powerDown(); //Check PIN_POWER_LOSS just in case we missed the falling edge
  attachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS), powerDown, FALLING);

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
    SerialPrintln(F("The sensor did not respond. Please check wiring."));
    while(1); //Freeze
  }

  pres.setReferencePressure(102100.0); // set to correct value day of launch!!!

  setStatusLED(false); // Configuration ended, ready to go

  measurementStartTime = millis();
}

void loop() {
  redo();
  //printpres();
  
}
