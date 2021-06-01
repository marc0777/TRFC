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

  bool logRTC = true;
  if (logRTC) {
    char timeString[37];
    getTimeString(timeString); // getTimeString is in TimeStamp.ino
    strcat(outputData, timeString);
  }

  bool logVIN = true;
  if (logVIN) {
    float voltage = readVIN();
    sprintf(tempData, "%.2f,", voltage);
    strcat(outputData, tempData);
  }

  if (myICM.dataReady()) {
    myICM.getAGMT(); //Update values

    sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.accX(), myICM.accY(), myICM.accZ());
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.gyrX(), myICM.gyrY(), myICM.gyrZ());
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.magX(), myICM.magY(), myICM.magZ());
    strcat(outputData, tempData);

    sprintf(tempData, "%.2f,", myICM.temp());
    strcat(outputData, tempData);
  }

  //Append all external sensor data on linked list to outputData
  //gatherDeviceValues();

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