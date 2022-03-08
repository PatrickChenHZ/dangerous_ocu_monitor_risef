#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

bool startpulse = false;

//for bio sensor v1 code
/*
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

char buf[100];

int32_t arrspo2[20];
int32_t arrheartrate[20];
int totalsamplespo2 = 0;
int totalsampleheartrate = 0;

int32_t finalspo2;
int32_t finalheartrate;
*/

// BIO v2 alg vars
static double fbpmrate = 0.95; // low pass filter coefficient for HRM in bpm
static uint32_t crosstime = 0; //falling edge , zero crossing time in msec
static uint32_t crosstime_prev = 0;//previous falling edge , zero crossing time in msec
static double bpm = 40.0;
static double ebpm = 40.0;
static double eir = 0.0; //estimated lowpass filtered IR signal to find falling edge without notch
static double firrate = 0.85; //IR filter coefficient to remove notch ,should be smaller than frate
static double eir_prev = 0.0;

double arrspo2[30];
double arrheartrate[30];
int totalsamplespo2 = 0;
int totalsampleheartrate = 0;

double finalspo2;
double finalheartrate;


#define TIMETOBOOT 3000 // wait for this time(msec) to output SpO2
#define SCALE 88.0 //adjust to display heart beat and SpO2 in the same scale
#define SAMPLING 5 //if you want to see heart beat more precisely , set SAMPLING to 1
#define FINGER_ON 30000 // if red signal is lower than this , it indicates your finger is not on the sensor
#define MINIMUM_SPO2 80.0

#define FINGER_ON 50000 // if ir signal is lower than this , it indicates your finger is not on the sensor
#define LED_PERIOD 100 // light up LED for this period in msec when zero crossing is found for filtered IR signal
#define MAX_BPS 180
#define MIN_BPS 45

double avered = 0; double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100;//calculate SpO2 by this sampling interval

double ESpO2 = 95.0;//initial value of estimated SpO2
double FSpO2 = 0.7; //filter factor for estimated SpO2
double frate = 0.95; // .95 default low pass filter for IR/red LED value to eliminate AC component

//End of BIO v2 alg vars

/*
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


    //Serial.print(F("red="));
    //Serial.print(redBuffer[i], DEC);
    //Serial.print(F(", ir="));
    //Serial.println(irBuffer[i], DEC);

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
      if(totalsamplespo2 > 1){
        if(spo2 != arrspo2[totalsamplespo2-1]){
          arrspo2[totalsamplespo2] = spo2; //store measurement into array
          totalsamplespo2++;
        }
      }
      else{
        arrspo2[totalsamplespo2] = spo2;
        totalsamplespo2++;
      }
    }
    if(validHeartRate == 1){
      //pulse_bpm = heartRate;
      if(totalsampleheartrate > 0){
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
*/

double HRM_estimator( double fir , double aveir){
  int CTdiff;

  // Heart Rate Monitor by finding falling edge
  eir = eir * firrate + fir * (1.0 - firrate); //estimated IR : low pass filtered IR signal

  if (((eir - aveir) * (eir_prev - aveir) < 0) && ((eir - aveir) < 0.0)){
    //find zero cross at falling edge
    crosstime = millis();//system time in msec of falling edge
    //Serial.print(crosstime); Serial.print(",CRT,");
    //Serial.println(crosstime_prev);
    //Serial.print("Cross Time :");
    CTdiff = crosstime-crosstime_prev;
    //Serial.println(CTdiff);
    //if ( ((crosstime - crosstime_prev ) > (60 * 1000 / MAX_BPS)) && ((crosstime - crosstime_prev ) < (60 * 1000 / MIN_BPS)) ) {
    if ((CTdiff > 333) && (CTdiff < 1333)){
      bpm = 60.0 * 1000.0 / (double)(crosstime - crosstime_prev) ; //get bpm
      // Serial.println("crossed");
      ebpm = ebpm * fbpmrate + (1.0 - fbpmrate) * bpm;//estimated bpm by low pass filtered
    }
    else{
      //Serial.println("faild to find falling edge");
    }
    crosstime_prev = crosstime;
  }
   eir_prev = eir;
   return (ebpm);
}

void updatebio_v2(){
  uint32_t ir, red , green;
  double fred, fir;
  double SpO2 = 0; //raw SpO2 before low pass filtered
  int idx=0;

  particleSensor.check(); //Check the sensor, read up to 3 samples

  int Ebpm = 60;

  while (1) {
    red = particleSensor.getRed();  //Sparkfun's MAX30105
    ir = particleSensor.getIR();  //Sparkfun's MAX30105
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate);//average red level by low pass filter
    aveir = aveir * frate + (double)ir * (1.0 - frate); //average IR level by low pass filter
    sumredrms += (fred - avered) * (fred - avered); //square sum of alternate component of red level
    sumirrms += (fir - aveir) * (fir - aveir);//square sum of alternate component of IR level
    Ebpm = (int) HRM_estimator(fir, aveir); //Ebpm is estimated BPM
    if (ir < FINGER_ON){
        ESpO2 = MINIMUM_SPO2; //indicator for finger detached
    }
    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      SpO2 = -23.3 * (R - 0.4) + 100;
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;
      //Serial.print("SPO2: ");
      //Serial.print(SpO2);Serial.print(",");Serial.print(ESpO2);
      //Serial.print(" BPM : "); //Serial.print(Ebpm);//Serial.print(", R:");
      //Serial.println(R);
      sumredrms = 0.0; sumirrms = 0.0; i = 0;

      //Serial.print("Heart Rate: ");
      //Serial.println(Ebpm);

      //it is expected this algrithm does not throw null/out of range datas, so all data is admitted to array
      arrspo2[totalsamplespo2] = ESpO2;
      arrheartrate[totalsampleheartrate] = Ebpm;
      totalsamplespo2++;
      totalsampleheartrate++;

      //sendSPOData(ESpO2);
      break;
    }
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
   // Serial.println(SpO2);
 }
}

/*
void biofilter(){

  //int32_t heartavg;
  //int32_t spo2avg;

  //filter out invalid measurements
  int referenceMeasurement = arrspo2[ 2*(totalsamplespo2/5) ];
  int medium[totalsamplespo2];
  int mediumIndex=0;

  for(int i=0; i<totalsamplespo2; i++){
    if(abs(referenceMeasurement - arrspo2[i]) > 13){
      arrspo2[i] = -998;
    }else{
      referenceMeasurement = arrspo2[i];
      medium[mediumIndex] = arrspo2[i];
      mediumIndex++;
    }
  }
  for(int i=0; i<totalsamplespo2; i++){
    if(i<=mediumIndex){
      arrspo2[i] = medium[i];
    }else{
      arrspo2[i] = 0;
    }
  }

  referenceMeasurement = arrheartrate[ 2*(totalsampleheartrate/5) ];
  int medium2[totalsampleheartrate];
  int mediumIndex2=0;

  for(int i=0; i<totalsampleheartrate; i++){
    if(abs(referenceMeasurement - arrheartrate[i]) > 20){
      arrheartrate[i] = -998;
    }else{
      referenceMeasurement = arrheartrate[i];
      medium2[mediumIndex2] = arrheartrate[i];
      mediumIndex2++;
    }
  }
  for(int i=0; i<totalsampleheartrate; i++){
    if(i<=mediumIndex2){
      arrheartrate[i] = medium2[i];
    }else{
      arrheartrate[i] = 0;
    }
  }


  totalsampleheartrate = 0;
  totalsamplespo2 = 0;
}
*/

void biofinalize_v2(){
  //very simple finalizer,this only remove min and max then average entire array
  double minhr=arrheartrate[0],maxhr=arrheartrate[0],minbo=arrspo2[0],maxbo=arrspo2[0];
  double sumhr=0,sumbo=0;
  for(int k=0;k<totalsamplespo2;k++){
    sumbo = sumbo + arrspo2[k];
    if(arrspo2[k] > maxbo){
      maxbo = arrspo2[k];
    }
    if(arrspo2[k] < minbo){
      minbo = arrspo2[k];
    }
  }
  for(int k=0;k<totalsampleheartrate;k++){
    sumhr = sumhr + arrheartrate[k];
    if(arrheartrate[k] > maxhr){
      maxhr = arrheartrate[k];
    }
    if(arrheartrate[k] < minhr){
      minhr = arrheartrate[k];
    }
  }
  finalspo2 = (sumbo - minbo - maxbo)/(totalsamplespo2 - 2);
  finalheartrate = (sumhr - minhr - maxhr)/(totalsampleheartrate - 2);
  //reset vars
  totalsamplespo2=0;
  totalsampleheartrate=0;
}

void pub_bio_data(){
  pubstr(String(pulse_bpm,2),"clients/wb/wb1hr");
  pubstr(String(bloodoxy,2),"clients/wb/wb1spo2");
}

/*
void getbiological(){
    //45 sec per reading
    //init is code v1 only

    //if(!pulsesensorinit){
    //  biosensorinit();
    //  pulsesensorinit = true;
    //}

    if((millis() > lastpulse + 45000) & !startpulse){
        startpulse = true;
        lastpulse = millis();
        //particleSensor.wakeUp();
        Serial.println("New Pulse round");
    }
    if(startpulse){
        //per reading only last 10sec
        if(lastpulse + 10000 > millis()){
            Serial.println("collecting pulse");
            //updatebiological();
            updatebio_v2();
        }
        else{
            //pulse period expired
            //particleSensor.shutDown();
            Serial.println("Finished.");
            //biofilter();
            biofinalize_v2();
            Serial.print("Pulse: ");
            Serial.print(finalheartrate);
            //sprintf(buf, "%lu\n", pulse_bpm); Serial.println(buf);
            Serial.print("Blood Oxygen: ");
            Serial.println(finalspo2);
            //sprintf(buf, "%lu\n", bloodoxy); Serial.println(buf);
            //commit to global
            bloodoxy = finalspo2;
            pulse_bpm = finalheartrate;
            pub_bio_data();
            //end of flag
            startpulse = false;
        }
    }
}
*/


unsigned long last_calc_bio = 0;

//constant reading, calculate average evry 25 second
void getbiological(){
    if(last_calc_bio + 25000 < millis()){
      biofinalize_v2();
      Serial.print("Pulse: ");
      Serial.print(finalheartrate);
      Serial.print("Blood Oxygen: ");
      Serial.println(finalspo2);
      //commit to global
      bloodoxy = finalspo2;
      pulse_bpm = finalheartrate;
      pub_bio_data();
      last_calc_bio = millis();
    }
    else{
      updatebio_v2();
    }
}

void pulse_loop(void * unused){
  //try to keep this at end of loop
  for(;;){
    getbiological();
    //purpose is to allow task schedular to do its job, delay() or yield() also works.
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
