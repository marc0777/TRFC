//#ifndef POWERMANAGEMENT_H
//#define POWERMANAGEMENT_H

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
  //Record this time so we wait enough time before detecting certain sensors
  // if(status) qwiicPowerOnTime = millis(); 
}

//Read the VIN voltage
float readVIN() {
  int div3 = analogRead(PIN_VIN_MONITOR); //Read VIN across a 1/3 resistor divider
  float vin = (float)div3 * 3.0 * 2.0 / 16384.0; //Convert 1/3 VIN to VIN (14-bit resolution)
  vin = vin * 1.47; //Correct for divider impedance (determined experimentally)
  return (vin);
}

int lowBatteryReadings = 0; // Count how many times the battery voltage has read low
const int lowBatteryReadingsLimit = 10; 
const int sdPowerDownDelay = 100;

// Read the battery voltage
// If it is low, increment lowBatteryReadings
// If lowBatteryReadings exceeds lowBatteryReadingsLimit then powerDown
void checkBattery(void) {
  bool enableLowBatteryDetection = false;
  if (enableLowBatteryDetection) {
    float voltage = readVIN(); // Read the battery voltage
    if (voltage < 3.4) // Is the voltage low?
    {
      lowBatteryReadings++; // Increment the low battery count
      if (lowBatteryReadings > lowBatteryReadingsLimit) // Have we exceeded the low battery count limit?
      {
        // Gracefully powerDown

        //Save files before powerDown
        /*
        if (online.dataLogging == true)
        {
          sensorDataFile.sync();
          updateDataFileAccess(&sensorDataFile); // Update the file access time & date
          sensorDataFile.close(); //No need to close files. https://forum.arduino.cc/index.php?topic=149504.msg1125098#msg1125098
        }
        if (online.serialLogging == true)
        {
          serialDataFile.sync();
          updateDataFileAccess(&serialDataFile); // Update the file access time & date
          serialDataFile.close();
        }
        */
      
        delay(sdPowerDownDelay); // Give the SD card time to finish writing ***** THIS IS CRITICAL *****

        //SerialPrintln(F("***      LOW BATTERY VOLTAGE DETECTED! GOING INTO POWERDOWN      ***"));
        //SerialPrintln(F("*** PLEASE CHANGE THE POWER SOURCE AND RESET THE OLA TO CONTINUE ***"));
      
        //SerialFlush(); //Finish any prints

        powerDown(); // power down and wait for reset
      }
    }
    else lowBatteryReadings = 0; // Reset the low battery count (to reject noise)
  }
}

//Power down the entire system but maintain running of RTC
//This function takes 100us to run including GPIO setting
//This puts the Apollo3 into 2.36uA to 2.6uA consumption mode
//With leakage across the 3.3V protection diode, it's approx 3.00uA.
void powerDown() {
  //Prevent voltage supervisor from waking us from sleep
  detachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS));

  //WE NEED TO POWER DOWN ASAP - we don't have time to close the SD files
  //Save files before going to sleep
  //  if (online.dataLogging == true)
  //  {
  //    sensorDataFile.sync();
  //    sensorDataFile.close();
  //  }
  //  if (online.serialLogging == true)
  //  {
  //    serialDataFile.sync();
  //    serialDataFile.close();
  //  }

  //SerialFlush(); //Don't waste time waiting for prints to finish

  Wire.end(); //Power down I2C
  qwiic.end(); //Power down I2C
  SPI.end(); //Power down SPI

  power_adc_disable(); //Power down ADC. It it started by default before setup().

  Serial.end(); //Power down UART
  //SerialLog.end();

  //Force the peripherals off
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  //Disable pads
  for (int x = 0; x < 50; x++) {
    if ((x != ap3_gpio_pin2pad(PIN_POWER_LOSS)) &&
        //(x != ap3_gpio_pin2pad(PIN_LOGIC_DEBUG)) &&
        (x != ap3_gpio_pin2pad(PIN_MICROSD_POWER)) &&
        (x != ap3_gpio_pin2pad(PIN_QWIIC_POWER)) &&
        (x != ap3_gpio_pin2pad(PIN_IMU_POWER))) 
      am_hal_gpio_pinconfig(x, g_AM_HAL_GPIO_DISABLE);
  }

  setPowerLED(false);

  //Make sure PIN_POWER_LOSS is configured as an input for the WDT
  pinMode(PIN_POWER_LOSS, INPUT); // BD49K30G-TL has CMOS output and does not need a pull-up

  //We can't leave these power control pins floating
  setIMUPower(false);
  setMicroSDPower(false);
  setQwiicPower(false);

  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  //Keep the 32kHz clock running for RTC
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);

  while (1) am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP); // Stay in deep sleep until we get reset
}

void smartDelay(int ms) {
  for (int i = 0; i < ms; i++) {
    checkBattery();
    delay(1);
  }
}

//#endif