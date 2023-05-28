#include "IMU.h"

ICM_20948_Status_e ICM_20948::initializeDMP() {
  ICM_20948_Status_e  result = ICM_20948_Stat_Ok; // Use result and worstResult to show if the configuration was successful
  ICM_20948_Status_e  worstResult = ICM_20948_Stat_Ok;
  result = i2cControllerConfigurePeripheral(0, MAG_AK09916_I2C_ADDR, AK09916_REG_RSV2, 10, true, true, false, true, true); if (result > worstResult) worstResult = result;
  result = i2cControllerConfigurePeripheral(1, MAG_AK09916_I2C_ADDR, AK09916_REG_CNTL2, 1, false, true, false, false, false, AK09916_mode_single); if (result > worstResult) worstResult = result;
  result = setBank(3); if (result > worstResult) worstResult = result; // Select Bank 3
  uint8_t mstODRconfig = 0x04; // Set the ODR configuration to 1100/2^4 = 68.75Hz
  result = write(AGB3_REG_I2C_MST_ODR_CONFIG, &mstODRconfig, 1); if (result > worstResult) worstResult = result; // Write one byte to the I2C_MST_ODR_CONFIG register  
  result = setClockSource(ICM_20948_Clock_Auto); if (result > worstResult) worstResult = result; // This is shorthand: success will be set to false if setClockSource fails
  result = setBank(0); if (result > worstResult) worstResult = result;                               // Select Bank 0
  uint8_t pwrMgmt2 = 0x40;                                                          // Set the reserved bit 6 (pressure sensor disable?)
  result = write(AGB0_REG_PWR_MGMT_2, &pwrMgmt2, 1); if (result > worstResult) worstResult = result; // Write one byte to the PWR_MGMT_2 register
  result = setSampleMode(ICM_20948_Internal_Mst, ICM_20948_Sample_Mode_Cycled); if (result > worstResult) worstResult = result;
  result = enableFIFO(false); if (result > worstResult) worstResult = result;
  result = enableDMP(false); if (result > worstResult) worstResult = result;
  ICM_20948_fss_t myFSS; // This uses a "Full Scale Settings" structure that can contain values for all configurable sensors
  myFSS.a = gpm16;
  myFSS.g = dps2000;
  result = setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS); if (result > worstResult) worstResult = result;
  result = enableDLPF(ICM_20948_Internal_Gyr, true); if (result > worstResult) worstResult = result;
  result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
  uint8_t zero = 0;
  result = write(AGB0_REG_FIFO_EN_1, &zero, 1); if (result > worstResult) worstResult = result;
  result = write(AGB0_REG_FIFO_EN_2, &zero, 1); if (result > worstResult) worstResult = result;
  result = intEnableRawDataReady(false); if (result > worstResult) worstResult = result;
  result = resetFIFO(); if (result > worstResult) worstResult = result;
  ICM_20948_smplrt_t mySmplrt;
  mySmplrt.g = 4;
  mySmplrt.a = 4;
  result = setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), mySmplrt); if (result > worstResult) worstResult = result;
  result = setDMPstartAddress(); if (result > worstResult) worstResult = result; // Defaults to DMP_START_ADDRESS
  result = loadDMPFirmware(); if (result > worstResult) worstResult = result;
  result = setDMPstartAddress(); if (result > worstResult) worstResult = result; // Defaults to DMP_START_ADDRESS
  result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
  uint8_t fix = 0x48;
  result = write(AGB0_REG_HW_FIX_DISABLE, &fix, 1); if (result > worstResult) worstResult = result;
  result = setBank(0); if (result > worstResult) worstResult = result; // Select Bank 0
  uint8_t fifoPrio = 0xE4;
  result = write(AGB0_REG_SINGLE_FIFO_PRIORITY_SEL, &fifoPrio, 1); if (result > worstResult) worstResult = result;
  const unsigned char accScale[4] = {0x10, 0x00, 0x00, 0x00};
  result = writeDMPmems(ACC_SCALE, 4, &accScale[0]); if (result > worstResult) worstResult = result; // Write accScale to ACC_SCALE DMP register
  const unsigned char accScale2[4] = {0x00, 0x01, 0x00, 0x00};
  result = writeDMPmems(ACC_SCALE2, 4, &accScale2[0]); if (result > worstResult) worstResult = result; // Write accScale2 to ACC_SCALE2 DMP register
  const unsigned char mountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
  const unsigned char mountMultiplierPlus[4] = {0x09, 0x99, 0x99, 0x99};  // Value taken from InvenSense Nucleo example
  const unsigned char mountMultiplierMinus[4] = {0xF6, 0x66, 0x66, 0x67}; // Value taken from InvenSense Nucleo example
  result = writeDMPmems(CPASS_MTX_00, 4, &mountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_01, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_02, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_10, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_11, 4, &mountMultiplierMinus[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_12, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_20, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_21, 4, &mountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(CPASS_MTX_22, 4, &mountMultiplierMinus[0]); if (result > worstResult) worstResult = result;
  const unsigned char b2sMountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
  const unsigned char b2sMountMultiplierPlus[4] = {0x40, 0x00, 0x00, 0x00}; // Value taken from InvenSense Nucleo example
  result = writeDMPmems(B2S_MTX_00, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_01, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_02, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_10, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_11, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_12, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_20, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_21, 4, &b2sMountMultiplierZero[0]); if (result > worstResult) worstResult = result;
  result = writeDMPmems(B2S_MTX_22, 4, &b2sMountMultiplierPlus[0]); if (result > worstResult) worstResult = result;
  result = setGyroSF(4, 3); if (result > worstResult) worstResult = result; // 4 = 225Hz (see above), 3 = 2000dps (see above)
  const unsigned char gyroFullScale[4] = {0x10, 0x00, 0x00, 0x00}; // 2000dps : 2^28
  result = writeDMPmems(GYRO_FULLSCALE, 4, &gyroFullScale[0]); if (result > worstResult) worstResult = result;
  const unsigned char accelOnlyGain[4] = {0x00, 0xE8, 0xBA, 0x2E}; // 225Hz
  result = writeDMPmems(ACCEL_ONLY_GAIN, 4, &accelOnlyGain[0]); if (result > worstResult) worstResult = result;
  const unsigned char accelAlphaVar[4] = {0x3D, 0x27, 0xD2, 0x7D}; // 225Hz
  result = writeDMPmems(ACCEL_ALPHA_VAR, 4, &accelAlphaVar[0]); if (result > worstResult) worstResult = result;
  const unsigned char accelAVar[4] = {0x02, 0xD8, 0x2D, 0x83}; // 225Hz
  result = writeDMPmems(ACCEL_A_VAR, 4, &accelAVar[0]); if (result > worstResult) worstResult = result;
  const unsigned char accelCalRate[4] = {0x00, 0x00}; // Value taken from InvenSense Nucleo example
  result = writeDMPmems(ACCEL_CAL_RATE, 2, &accelCalRate[0]); if (result > worstResult) worstResult = result;
  const unsigned char compassRate[2] = {0x00, 0x45}; // 69Hz
  result = writeDMPmems(CPASS_TIME_BUFFER, 2, &compassRate[0]); if (result > worstResult) worstResult = result;
  return worstResult;
}

void IMU::begin() {
  bool success = true;
  myICM.begin(PIN_IMU_CHIP_SELECT, SPI, 4000000); //Set IMU SPI rate to 4MHz

  //Give the IMU extra time to get its act together. This seems to fix the IMU-not-starting-up-cleanly-after-sleep problem...
  //Seems to need a full 25ms. 10ms is not enough.
  delay(25);

  myICM.initializeDMP();
  
  myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION);
  //myICM.enableDMPSensor(INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR); // without magnetometer
  myICM.enableDMPSensor(INV_ICM20948_SENSOR_ACCELEROMETER); // instead of _RAW_ACCELEROMETER
  myICM.enableDMPSensor(INV_ICM20948_SENSOR_GYROSCOPE); // instead of _RAW_GYROSCOPE
  
  
  myICM.setDMPODRrate(DMP_ODR_Reg_Quat9, 1);
  myICM.setDMPODRrate(DMP_ODR_Reg_Accel, 1);
  myICM.setDMPODRrate(DMP_ODR_Reg_Gyro, 1);
  

  myICM.enableFIFO();
  myICM.enableDMP();
  myICM.resetDMP();
  myICM.resetFIFO();
}

bool IMU::check() {
  return myICM.status == ICM_20948_Stat_Ok;
}

void IMU::update() {
  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);
  
  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
      q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
      q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
    }

    if ((data.header & DMP_header_bitmap_Accel) > 0) {
      // Extract the accelerometer data in g, and convert it to m/s²
      acc_x = ((float)data.Raw_Accel.Data.X / 2048.f) * 9.81f; 
      acc_y = ((float)data.Raw_Accel.Data.Y / 2048.f) * 9.81f;
      acc_z = ((float)data.Raw_Accel.Data.Z / 2048.f) * 9.81f;;
    }

    if ( (data.header & DMP_header_bitmap_Gyro) > 0 ) {
      gyr_x = (float)data.Raw_Gyro.Data.X; // Extract the raw gyro data
      gyr_y = (float)data.Raw_Gyro.Data.Y;
      gyr_z = (float)data.Raw_Gyro.Data.Z;
    }

  }

}
