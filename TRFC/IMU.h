#ifndef IMU_h
#define IMU_h

#include <SPI.h>
#include "ICM_20948.h"
#include "Pins.h"

class IMU {

public:
  void begin();
  bool check();

  void update();
  double q0, q1, q2, q3;
  float acc_x, acc_y, acc_z;
  float gyr_x, gyr_y, gyr_z;

  float temp() {return myICM.temp();}

private:
  ICM_20948_SPI myICM; // Integrated IMU, connected via SPI

};


#endif
