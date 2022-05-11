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
  sensors[6] = imu.q0;
  sensors[7] = imu.q1;
  sensors[8] = imu.q2;
  sensors[9] = imu.q3;

  sensors[10] = pres.readFloatPressure();
  sensors[11] = pres.readFloatAltitudeMeters();
  sensors[12] = pres.readTempC();
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
  sprintf(tempData, "%.2f,%.2f,%.2f,%.2f,", sensors[6], sensors[7], sensors[8], sensors[9]);
  strcat(outputData, tempData);

  sprintf(tempData, "%.2f,", sensors[10]);
  strcat(outputData, tempData);
  
  sprintf(tempData, "%.2f,", sensors[11]);
  strcat(outputData, tempData);
  
  sprintf(tempData, "%.2f,", sensors[12]);
  strcat(outputData, tempData);

  strcat(outputData, "\r\n");
}
