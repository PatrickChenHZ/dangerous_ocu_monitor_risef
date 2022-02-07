#include <MPU9250_asukiaaa.h>
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
float angleChange=0;

void mpu_read(){
  /*
  AcX=IMU.ax;
  AcY=IMU.ay;
  AcZ=IMU.az;
  GyX=IMU.gx;
  GyY=IMU.gy;
  GyZ=IMU.gz;
  */
  myIMU.accelUpdate();
  AcX = myIMU.accelX();
  AcY = myIMU.accelY();
  AcZ = myIMU.accelZ();

  myIMU.gyroUpdate();
  GyX=myIMU.gyroX();
  GyY=myIMU.gyroY();
  GyZ=myIMU.gyroZ();

}

void fall_det(){
  mpu_read();
  //2050, 77, 1947 are values for calibration of accelerometer
  /*
  ax = (AcX)/16384.00;
  ay = (AcY)/16384.00;
  az = (AcZ)/16384.00;
  */

  ax = (int)1000 * AcX;
  ay = (int)1000 * AcY;
  az = (int)1000 * AcZ;

  /*
  Serial.print("Ax: ");
  Serial.print(AcX);
  Serial.print(" Ay: ");
  Serial.print(AcY);
  Serial.print(" Az: ");
  Serial.println(AcZ);
  */

  //270, 351, 136 for gyroscope calibration
  /*
  gx = (GyX)/131.07;
  gy = (GyY)/131.07;
  gz = (GyZ)/131.07;
  */
  gx = (float)GyX;
  gy = (float)GyY;
  gz = (float)GyZ;

  /*
  Serial.print("Gx: ");
  Serial.print(GyX);
  Serial.print(" Gy: ");
  Serial.print(GyY);
  Serial.print(" Gz: ");
  Serial.println(GyZ);
  */

  //calculating Amplitute vector for 3 axis
  //float Raw_AM = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
  //float AM = Raw_AM;  // as values are within 0 to 1, it is multiplied by 10 for using if else conditions
  float AM = myIMU.accelSqrt();

  //Serial.println(AM);
  //Serial.println(PM);
  //delay(500);

  if (trigger3==true){
     trigger3count++;
     //Serial.println(trigger3count);
     if (trigger3count>=10){
        angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
        //delay(10);
        Serial.println(angleChange);
        if ((angleChange>=0) && (angleChange<=200)){ //if orientation changes remains between 0-10 degrees
            fall=true; trigger3=false; trigger3count=0;
            //Serial.println(angleChange);
              }
        else{ //user regained normal orientation
           trigger3=false; trigger3count=0;
           Serial.println("Stage 3 DEACTIVATED");
           pubstr("Stage 3 DEACTIVATED","clients/wb/wb1/debug");
        }
      }
   }
  if (trigger2count>=6){ //allow 0.5s for orientation change
    trigger2=false; trigger2count=0;
    Serial.println("Stage 2 DECACTIVATED");
    pubstr("Stage 2 DECACTIVATED","clients/wb/wb1/debug");
    }
  if (trigger1count>=6){ //allow 0.5s for AM to break upper threshold
    trigger1=false; trigger1count=0;
    Serial.println("Stage 1 DECACTIVATED");
    pubstr("Stage 1 DECACTIVATED","clients/wb/wb1/debug");
    }
  if (trigger2==true){
    trigger2count++;
    //angleChange=acos(((double)x*(double)bx+(double)y*(double)by+(double)z*(double)bz)/(double)AM/(double)BM);
    angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5); Serial.println(angleChange);
    if (angleChange>=100 && angleChange<=700){ //if orientation changes by between 80-100 degrees
      trigger3=true; trigger2=false; trigger2count=0;
      //Serial.println(angleChange);
      Serial.println("Stage 3 ACTIVATED");
      pubstr("Stage 3 ACTIVATED","clients/wb/wb1/debug");
        }
    }
  if (trigger1==true){
    trigger1count++;
    if (AM>fall_upperthreshold){ //if AM breaks upper threshold (3g)
      trigger2=true;
      Serial.println("Stage 2 ACTIVATED");
      pubstr("Stage 2 ACTIVATED","clients/wb/wb1/debug");
      trigger1=false; trigger1count=0;
      }
    }
  if (AM<=fall_lowerthreshold && trigger2==false){ //if AM breaks lower threshold (0.4g)
    trigger1=true;
    Serial.println("Stage 1 ACTIVATED");
    pubstr("Stage 1 ACTIVATED","clients/wb/wb1/debug");
    }
//It appears that delay is needed in order not to clog the port
  delay(50);
}
