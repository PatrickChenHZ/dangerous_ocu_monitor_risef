/*
Vars that are decleared in global scope
float microparticle

*/

#include "sensors/R200.h"
#include "sensors/gp2y10h1.h"

void particle_setup(){
  Serial1.begin(2400);
}

void rfid_setup(){
  Serial2.begin(115200);
}

void barometer_setup(){

}

void dht22_setup(){

}

void gas_setup(){

}

void particle_read(){
  float aa=median_average_filter();
  microparticle=aa*800;
}
