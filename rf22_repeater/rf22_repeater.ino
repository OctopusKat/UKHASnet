// rf22_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RF22 class. RF22 class does not provide for addressing or reliability.
// It is designed to work with the other example rf22_server

#include <SPI.h>
#include <RF22.h>
#include <EEPROM.h>

// Singleton instance of the radio
RF22 rf22;

int n, count = 0, data_interval = 2, path = 0;
byte data_count = 97; // 'a'

//Msg format
// Repeat Value Data[Repeater ID 1, Repeater ID 2]
//e.g. 3>52.0,-0.0[A,A,B]

uint8_t data[30] = "3aL52.0,-0.0T26[]";
uint8_t id = 'X';

void setupRFM22(){  
  //GFSK_Rb2Fd5
  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  
  rf22.setTxPower(RF22_TXPOW_17DBM);
}

void setup() 
{
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
  Serial.begin(9600);
  randomSeed(analogRead(0));
  
  //Read EEPROM to detect if we already have set an ID for this node
  //http://arduino.cc/en/Reference/EEPROMRead
  id = EEPROM.read(0);
  
  for (int i=0; i<len; i++) {
    if (buf[i] == ']') {
      data[i] = id;
      data[i+1] = ']';
      break;
    }
  }
  
  
  if (!rf22.init()){
    Serial.println("RF22 init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  }
  else{
    Serial.println("RF22 Booted");
  }
  
  rf22.setFrequency(869.50);
  
  setupRFM22();
  delay(1000);
  
  //Send first packet
  Serial.println("Sending first packet");
  rf22.send(data, sizeof(data));
  rf22.waitPacketSent();
}

void loop()
{
  while (1)
  {
    count++;
    //Serial.print(count);
    
    // Listen for data
    uint8_t buf[RF22_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf22.waitAvailableTimeout(10000))
    { 
      // Should be a message for us now   
      if (rf22.recv(buf, &len))
      {
        Serial.print("got reply: ");
        Serial.println((char*)buf);
        //THIS IS WHERE WE SORT OUT THE REPEATER
        // Need to take the recieved buffer and decode it and add a reference
        
        if (buf[0] > '0'){
          
          //Reduce the repeat value
          buf[0] = buf[0] - 1;
          
          //Add the repeater ID
          //First find "]"
          
          for (int i=0; i<len; i++) {
            if(buf[i] == '['){
              path = 1;
            }
            if((buf[i] == id) && (path == 1)){
              break;
            }
            if (buf[i] == ']') {
              buf[i] = ',';
              buf[i + 1] = id;
              buf[i + 2] = ']';
              path = 0;
              
              Serial.print("Sent data: "); Serial.println((char*)buf);
              rf22.send(buf, sizeof(buf));
              rf22.waitPacketSent();
              break;
            }
          }
          

        }
        else{
          Serial.print("Stop ");
          Serial.println((char*)buf);
        }
      }
      else
      {
        Serial.println("recv failed");
      }
    }
    else
    {
      Serial.print(".");
    }
    
    if (count == data_interval){
      
      //**** Packet Tx Count ******
      //using a byte to keep track of transmit count
      // its meant to roll over
      data_count++;
      //0 packet is only sent on the first transmission so we need to move it along
      // when we roll over.
      // 98 = 'b' up to 122 = 'z'
      if(data_count == 123){
        data_count = 98; //'b'
      }
      data[1] = data_count;
      
      
      Serial.println("Sending");
      // Send a message
      
      rf22.send(data, sizeof(data));
   
      rf22.waitPacketSent();
      
      //**** Packet Tx Interval ******
      data_interval = random(20) + count;
      Serial.print("Next string: ");
      Serial.println(data_interval);
    }
  }
}
