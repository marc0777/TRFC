#ifndef IMU_h
#define IMU_h

#include <SPI.h>
#include "ICM_20948.h"
#include "Pins.h"

class IMU {

public:
  void begin();
  bool check();

  void getAGMT() {myICM.getAGMT();}
  bool dataReady() {return myICM.dataReady();}

  float accX() {return myICM.accX();}
  float accY() {return myICM.accY();}
  float accZ() {return myICM.accZ();}

  float gyrX() {return myICM.gyrX();}
  float gyrY() {return myICM.gyrY();}
  float gyrZ() {return myICM.gyrZ();}

  float magX() {return myICM.magX();}
  float magY() {return myICM.magY();}
  float magZ() {return myICM.magZ();}

  void update();
  double q0, q1, q2, q3;

  float temp() {return myICM.temp();}

private:
  ICM_20948_SPI myICM; // Integrated IMU, connected via SPI

};


#endif
