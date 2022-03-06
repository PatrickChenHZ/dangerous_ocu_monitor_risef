//simple Library for R200 UHF-RFID Module
void version(){

}

void req_multi_read(int requests=65535){
  byte message[] = {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0x27, 0x10, 0x83, 0x7E};
  Serial2.write(message, sizeof(message));
}

void multi_read_processing(){
  if (Serial2.available() > 0) {

    // read the incoming byte:
    income = Serial2.read();

    if(income == 0xBB){
      Serial.print("Start");
    }
    else if(income == 0x7E){
      Serial.println("End");
    }
    else{
      Serial.print(income);
    }
  }
}
