#include "IMU.h"

void IMU::begin() {
  bool success = true;
  pinMode(PIN_IMU_POWER, OUTPUT);
  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected

  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, false);
  delay(10);
  digitalWrite(PIN_IMU_POWER, true);
  //Allow ICM to come online. Typical is 11ms. Max is 100ms. 
  delay(100);
  myICM.begin(PIN_IMU_CHIP_SELECT, SPI, 4000000); //Set IMU SPI rate to 4MHz

  //Give the IMU extra time to get its act together. This seems to fix the IMU-not-starting-up-cleanly-after-sleep problem...
  //Seems to need a full 25ms. 10ms is not enough.
  delay(25);

  //Update the full scale and DLPF settings
  myICM.enableDLPF(ICM_20948_Internal_Acc, false); // IMU accelerometer Digital Low Pass Filter

  myICM.enableDLPF(ICM_20948_Internal_Gyr, false); // IMU gyro Digital Low Pass Filter

  ICM_20948_dlpcfg_t dlpcfg;
  dlpcfg.a = 7; // IMU accelerometer DLPF bandwidth
  dlpcfg.g = 7; // IMU gyro DLPF bandwidth
  myICM.setDLPFcfg((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), dlpcfg);

  ICM_20948_fss_t FSS;
  FSS.a = 3; // IMU accelerometer full scale, set to maximum or +-16 g
  FSS.g = 0; // IMU gyro full scale
  myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), FSS);

  myICM.initializeDMP();
  myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION);
  myICM.setDMPODRrate(DMP_ODR_Reg_Quat9, 0);

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
    if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
      if ((data.header & DMP_header_bitmap_Quat9) > 0) {
        q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
        q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
        q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
        q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
      }
    }

    if ((data.header & DMP_header_bitmap_Accel) > 0) {
      float acc_x = (float)data.Raw_Accel.Data.X; // Extract the raw accelerometer data
      float acc_y = (float)data.Raw_Accel.Data.Y;
      float acc_z = (float)data.Raw_Accel.Data.Z;
    }

    if ( (data.header & DMP_header_bitmap_Gyro) > 0 ) {
      float x = (float)data.Raw_Gyro.Data.X; // Extract the raw gyro data
      float y = (float)data.Raw_Gyro.Data.Y;
      float z = (float)data.Raw_Gyro.Data.Z;
    }
    
    if ( (data.header & DMP_header_bitmap_Compass) > 0 ) {
      float x = (float)data.Compass.Data.X; // Extract the compass data
      float y = (float)data.Compass.Data.Y;
      float z = (float)data.Compass.Data.Z;
    }

  }

}
