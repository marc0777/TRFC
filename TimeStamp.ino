void getTimeString(char timeStringBuffer[]) {
  //reset the buffer
  timeStringBuffer[0] = '\0';

  myRTC.getTime();

  bool logDate = false;
  if (logDate) {
    char rtcDate[12]; //10/12/2019,
    sprintf(rtcDate, "%02d/%02d/20%02d,", myRTC.dayOfMonth, myRTC.month, myRTC.year);
    strcat(timeStringBuffer, rtcDate);
  }

  bool logTime = true;
  if (logTime) {
    char rtcTime[13]; //09:14:37.41,
    sprintf(rtcTime, "%02d:%02d:%02d.%02d,", myRTC.hour, myRTC.minute, myRTC.seconds, myRTC.hundredths);
    strcat(timeStringBuffer, rtcTime);
  }
  
  bool logMicroseconds = false;
  if (logMicroseconds) {
    char microseconds[11]; //
    sprintf(microseconds, "%lu,", micros());
    strcat(timeStringBuffer, microseconds);
  }
}
