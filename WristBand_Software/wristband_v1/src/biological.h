#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"


uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

bool startpulse = false;
char buf[100];

int32_t arrspo2[20];
int32_t arrheartrate[20];
int totalsamplespo2 = 0;
int totalsampleheartrate = 0;

int32_t finalspo2;
int32_t finalheartrate;

void biosensorinit(){
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false)
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();

    /*
    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
    */
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
}

//this require full loop time
void updatebiological()
{
  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
  for (byte i = 25; i < 100; i++)
  {
    redBuffer[i - 25] = redBuffer[i];
    irBuffer[i - 25] = irBuffer[i];
  }

  //take 25 sets of samples before calculating the heart rate.
  for (byte i = 75; i < 100; i++)
  {
    while (particleSensor.available() == false)
      particleSensor.check();


    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //finished with this sample so move to next sample


      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[i], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);

    //data pre-process
    if(validSPO2 == 1){
      //bloodoxy = spo2;
      if(totalsamplespo2 != 0){
        if(spo2 != arrspo2[totalsampleheartrate-1]){
          arrspo2[totalsampleheartrate] = spo2;
          totalsamplespo2++;
        }
      }
      else{
        arrspo2[totalsampleheartrate] = spo2;
        totalsamplespo2++;
      }
    }
    if(validHeartRate == 1){
      //pulse_bpm = heartRate;
      if(totalsampleheartrate != 0){
        if(heartRate != arrheartrate[totalsampleheartrate-1]){
          arrheartrate[totalsampleheartrate] = heartRate;
          totalsampleheartrate++;
        }
      }
      else{
        arrheartrate[totalsampleheartrate] = heartRate;
        totalsampleheartrate++;
      }
    }
  }

    //After gathering 25 new samples recalculate HR and SP02
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
}

void biofilter(){
  int32_t heartavg;
  int32_t spo2avg;

  //reset vars
  totalsampleheartrate = 0;
  totalsamplespo2 = 0;
}

void getbiological(){
    //45 sec per reading
    if(!pulsesensorinit){
      biosensorinit();
      pulsesensorinit = true;
    }
    if((millis() > lastpulse + 45000) & !startpulse){
        startpulse = true;
        lastpulse = millis();
        particleSensor.wakeUp();
        Serial.println("New Pulse round");
    }
    if(startpulse){
        //per reading only last 10sec
        if(lastpulse + 10000 > millis()){
            //particleSensor.wakeUp();
            Serial.println("collecting pulse");
            updatebiological();
        }
        else{
            //pulse period expired
            particleSensor.shutDown();
            Serial.println("Finished.");
            biofilter();
            Serial.print("Pulse: ");
            sprintf(buf, "%lu\n", pulse_bpm); Serial.println(buf);
            Serial.print("Blood Oxygen: ");
            sprintf(buf, "%lu\n", bloodoxy); Serial.println(buf);
            startpulse = false;
        }
    }
}
