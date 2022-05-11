unsigned long lastReadTime = 0; //Used to delay until user wants to record a new reading
unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second
bool takeReading = true; //Goes true when enough time has passed between readings or we've woken from sleep

int sendfreq = 10;
int sentf = 0;

void redo() {
  checkBattery(); // Check for low battery

  uint64_t hertz = 111;
  uint64_t usBetweenReadings = 1000000ULL / hertz;

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
    if (terminalOut) SerialPrint(outputData); //Print to terminal


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
