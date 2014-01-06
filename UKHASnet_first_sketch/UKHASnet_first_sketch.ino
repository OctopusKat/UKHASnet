//UKHASnet_first_sketch

#include <EEPROM.h>

byte node_ID;

void setup(){
  Serial.begin(9600);
  Serial.println("Welcome to UKHASnet EEPROM programing sketch");
  Serial.println("Please enter the assigned node ID:");
}

void loop(){
  while(1){
    if (Serial.available()){
      node_ID = Serial.read();
      EEPROM.write(0, node_ID);
      Serial.println("Done, node ID written");
      delay(1000);
      Serial.println("Check node_ID (written to address 1)");
      Serial.print("node ID: ");
      Serial.println(EEPROM.read(0));
      delay(1000);
      break;
    }
  }
}
