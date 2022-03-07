//simple Library for R200 UHF-RFID Module
#include <Arduino.h>
//lib global vars
byte response[40] = {};


void r200_version(){
  byte message[] = {0xBB, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x7E};
  //byte message2[] = {187, 0, 3, 0, 1, 0, 4, 126};
  Serial2.write(message, sizeof(message));
}

void req_multi_read(int requests=65535){
  //Temprorarily limted two hard coded command as their is check sum byte in the end
  if(requests == 65535){
    byte message[] = {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E};
    Serial2.write(message, sizeof(message));
  }
  else if(requests==10000){
    byte message[] = {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0x27, 0x10, 0x83, 0x7E};
    Serial2.write(message, sizeof(message));
  }
}

void card_found(int bytenumber){
  int datastart = 8;
  int dataend = 19;
  String epc_id = "";
  for(int b=0;b<=bytenumber;b++){
    if(b>=datastart && b<=dataend){
      epc_id = epc_id + String(response[b]);
    }
  }
  //set global var
  rfid_id = epc_id;
}

//constantly run this in main loop
void r200_parse(){
  boolean indata = false;
  int bytenumber = 0;
  byte income;
  String type = "";
  if (Serial2.available() > 0) {
    income = Serial2.read();
    if(income == 0xBB){
      //strat of packet
      indata = true;
    }
    else if(income == 0x7E){
      //end of packet
      //Serial.println();
      //process data when a packet ends
      if(type == "read"){
        card_found(bytenumber);
      }
      indata = false;
      bytenumber = 0;
    }
    else{
      if(indata){
        response[bytenumber] = income;
        bytenumber++;
        //Serial.print(income);
        //parse data type
        if(bytenumber == 1){
          if(income == 0x27){
            //continuous read
            type = "read";
          }
          if(income == 0x03){
            //read hardware verison
            type = "info";
          }
          if(income == 0xFF){
            //no data returned/failed request
            type = "fail";
          }
        }
      }
    }
  }
}
