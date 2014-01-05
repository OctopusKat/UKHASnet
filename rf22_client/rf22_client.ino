// rf22_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RF22 class. RF22 class does not provide for addressing or reliability.
// It is designed to work with the other example rf22_server

#include <SPI.h>
#include <RF22.h>

// Singleton instance of the radio
RF22 rf22;

//Msg format
// ID,count,HH:MM:SS,Repeat Value,Data
//e.g. A,123,3,>52.0,-0.0[AAB]
uint8_t data[30] = "L52.0,-0.0[A]";
uint8_t id = 'A';

int n, count = 1;

void CharToByte(char* chars, byte* bytes, unsigned int count){
    for(unsigned int i = 0; i < count; i++)
    	bytes[i] = (byte)chars[i];
}

void setupRFM22(){  
  //GFSK_Rb2Fd5
  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  
  rf22.setTxPower(RF22_TXPOW_17DBM);
}

void setupRadio(){

  rf22.spiWrite(0x71, 0x00); // unmodulated carrier
  rf22.setTxPower(RF22_TXPOW_17DBM);
  rf22.spiWrite(0x07, 0x08); // turn tx on
  delay(1000);
}

void cw_ID(){
  rf22.spiWrite(0x07, 0x01); //off
  delay(1000);
  rf22.spiWrite(0x07, 0x08); //on
  delay(1000);
  rf22.spiWrite(0x07, 0x01); //off
  delay(1000);
  rf22.spiWrite(0x07, 0x08); //on
  delay(1000);
  rf22.spiWrite(0x07, 0x01); //off
  }

void setup() 
{
  Serial.begin(9600);
  
  if (!rf22.init()){
    Serial.println("RF22 init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  }
  else{
    Serial.println("RF22 Booted");
  }
  
  rf22.setFrequency(869.50);
}

void loop()
{
  while (1)
  {
    count++;

    setupRFM22();
    
    Serial.println("Sending to rf22_server");
    // Send a message to rf22_server
    rf22.send(data, sizeof(data));
   
    rf22.waitPacketSent();

    if ((count % 10) ==0){
    setupRadio();
    cw_ID();
    }
    
    delay(5000);
    
  }
}
