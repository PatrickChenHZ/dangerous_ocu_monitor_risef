//simple Library for R200 UHF-RFID Module


void r200_version(){
  byte message[] = {0xBB, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x7E};
  Serial2.write(message, sizeof(message));
}

void req_multi_read(int requests=65535){
  if(requests == 65535){
    byte message[] = {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E};
  }
  else if(requests==10000){
    byte message[] = {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0x27, 0x10, 0x83, 0x7E};
  }
  Serial2.write(message, sizeof(message));
}

void r200_parse(){
  byte response[40] = {};
  boolean indata = false;
  int bytenumber = 0;
  byte income;
  string type = "";
  int datastart;
  int dataend;
  if (Serial2.available() > 0) {
    income = Serial2.read();
    if(income == 0xBB){
      //strat of packet
      indata = true;
    }
    else if(income == 0x7E){
      //end of packet
      indata = false;
      bytenumber = 0;
      //Serial.println();
    }
    else{
      if(indata){
          response[bytenumber] = income;
          bytenumber++;
          //Serial.print(income);
          if(bytenumber == 1){
            if(income == 0x27){
              //group read
              type = "read"
              datastart = 8;
              dataend = 19;
            }
            if(income == 0x03){
              //read hardware verison
              type = "info"
              datastart = 6;
              dataend = 14;
            }
            if(income == 0xFF){
              //read hardware verison
              type = "fail"
              datastart = 0;
              dataend = 0;
            }
          }
      }
    }
  }
  for(int p=0;k<=bytenumber;k++){

  }
}
