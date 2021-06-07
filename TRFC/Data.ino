uint64_t measurementStartTime; //Used to calc the actual update rate. Max is ~80,000,000ms in a 24 hour period.
unsigned long measurementCount = 0; //Used to calc the actual update rate.
unsigned long measurementTotal = 0; //The total number of recorded measurements. (Doesn't get reset when the menu is opened)
unsigned int totalCharactersPrinted = 0; //Limit output rate based on baud rate and number of characters to print

//Query each enabled sensor for its most recent data
void getData() {
  measurementCount++;
  measurementTotal++;

  char tempData[50];
  outputData[0] = '\0'; //Clear string contents

  bool logRTC = false;
  if (logRTC) {
    char timeString[37];
    getTimeString(timeString); // getTimeString is in TimeStamp.ino
    strcat(outputData, timeString);
  }

  bool logVIN = false;
  if (logVIN) {
    float voltage = readVIN();
    sprintf(tempData, "%.2f,", voltage);
    strcat(outputData, tempData);
  }

  if (imu.dataReady()) {
    imu.getAGMT(); //Update values

    sprintf(tempData, "%.2f,%.2f,%.2f,", imu.accX(), imu.accY(), imu.accZ());
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,%.2f,%.2f,", imu.gyrX(), imu.gyrY(), imu.gyrZ());
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,%.2f,%.2f,", imu.magX(), imu.magY(), imu.magZ());
    strcat(outputData, tempData);

    imu.update();
    sprintf(tempData, "%.2f,%.2f,%.2f,%.2f,", imu.q0, imu.q1, imu.q2, imu.q3);
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,", imu.temp());
    strcat(outputData, tempData);
  }

  bool logHertz = false;
  if (logHertz) {
    //Calculate the actual update rate based on the sketch start time and the
    //number of updates we've completed.
    uint64_t currentMillis = millis();

    float actualRate = measurementCount * 1000.0 / (currentMillis - measurementStartTime);
    sprintf(tempData, "%.02f,", actualRate); //Hz
    strcat(outputData, tempData);
  }

  bool printMeasurementCount = false;
  if (printMeasurementCount) {
    sprintf(tempData, "%d,", measurementTotal);
    strcat(outputData, tempData);
  }

  strcat(outputData, "\r\n");

  totalCharactersPrinted += strlen(outputData);
}