#include "imu_sensor.h"
extern MPU9250 IMU;

int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;

//int data[STORE_SIZE][5]; //array for saving past data
//byte currentIndex=0; //stores current data array index (0-255)
boolean trigger1=false; //first trigger (lower threshold)
boolean trigger2=false; //second trigger (upper threshold)
boolean trigger3=false; //third trigger (orientation change)

byte trigger1count=0; //stores the counts past since trigger 1 was set true
byte trigger2count=0; //stores the counts past since trigger 2 was set true
byte trigger3count=0; //stores the counts past since trigger 3 was set true
int angleChange=0;

//taken from example code
void mpu_read(){
  AcX=IMU.ax;
  AcY=IMU.ay;
  AcZ=IMU.az;
  GyX=IMU.gx;
  GyY=IMU.gy;
  GyZ=IMU.gz;
}

void fall_det(){

  mpu_read();
  //2050, 77, 1947 are values for calibration of accelerometer
  ax = (AcX)/16384.00;
  ay = (AcY)/16384.00;
  az = (AcZ)/16384.00;
  /*
  Serial.print("Ax: ");
  Serial.print(AcX);
  Serial.print(" Ay: ");
  Serial.print(AcY);
  Serial.print(" Az: ");
  Serial.println(AcZ)
  */

  //270, 351, 136 for gyroscope calibration
  gx = (GyX)/131.07;
  gy = (GyY)/131.07;
  gz = (GyZ)/131.07;

  //calculating Amplitute vector for 3 axis
  float Raw_AM = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
  int AM = Raw_AM * 10;  // as values are within 0 to 1, it is multiplied by 10 for using if else conditions

  Serial.println(AM);
  //Serial.println(PM);
  //delay(500);

  if (trigger3==true){
     trigger3count++;
     //Serial.println(trigger3count);
     if (trigger3count>=10){
        angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
        //delay(10);
        Serial.println(angleChange);
        if ((angleChange>=0) && (angleChange<=10)){ //if orientation changes remains between 0-10 degrees
            fall=true; trigger3=false; trigger3count=0;
            Serial.println(angleChange);
              }
        else{ //user regained normal orientation
           trigger3=false; trigger3count=0;
           Serial.println("Stage 3 DEACTIVATED");
        }
      }
   }
  if (trigger2count>=6){ //allow 0.5s for orientation change
    trigger2=false; trigger2count=0;
    Serial.println("Stage 2 DECACTIVATED");
    }
  if (trigger1count>=6){ //allow 0.5s for AM to break upper threshold
    trigger1=false; trigger1count=0;
    Serial.println("Stage 1 DECACTIVATED");
    }
  if (trigger2==true){
    trigger2count++;
    //angleChange=acos(((double)x*(double)bx+(double)y*(double)by+(double)z*(double)bz)/(double)AM/(double)BM);
    angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5); Serial.println(angleChange);
    if (angleChange>=30 && angleChange<=400){ //if orientation changes by between 80-100 degrees
      trigger3=true; trigger2=false; trigger2count=0;
      Serial.println(angleChange);
      Serial.println("Stage 3 ACTIVATED");
        }
    }
  if (trigger1==true){
    trigger1count++;
    if (AM>=15){ //if AM breaks upper threshold (3g)
      trigger2=true;
      Serial.println("Stage 2 ACTIVATED");
      trigger1=false; trigger1count=0;
      }
    }
  if (AM<=5 && trigger2==false){ //if AM breaks lower threshold (0.4g)
    trigger1=true;
    Serial.println("Stage 1 ACTIVATED");
    }
//It appears that delay is needed in order not to clog the port
  delay(50);
}
