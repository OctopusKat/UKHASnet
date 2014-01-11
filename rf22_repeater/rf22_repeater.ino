/*
UKHASnet rf22_repeater code

based on rf22_client.pde/ino from the RF22 library

 Code for OneWire comes from:
// OneWire DS18S20, DS18B20, DS1822 Temperature Example
// http://www.pjrc.com/teensy/td_libs_OneWire.html

*/

#include <SPI.h>
#include <RF22.h>
#include <EEPROM.h>

//Required for DSB18b20 Temp sensor
#include <OneWire.h>
OneWire  ds(5);  // on pin 10 (a 4.7K resistor is necessary)


// Singleton instance of the radio
RF22 rf22;

int n, count = 0, data_interval = 2, path = 0;
byte data_count = 97; // 'a'
byte num_repeats = '3';
int rfm22_shutdown = 3, intTemp = 0;

//Msg format
// Repeat_value packet_sequence Data[Repeater ID 1, Repeater ID 2]
//e.g. 3hL52.0,-0.0[A,A,B]

char data[50];
char id = 'X';

void setupRFM22(){  
  //GFSK_Rb2Fd5
  rf22.setFrequency(869.50);
  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  rf22.setTxPower(RF22_TXPOW_17DBM);
}

void gen_Data(){
  
  //**** Temperature ******
  //Now we need to add the Temperature data (5bytes)
  long int temp = 0;
  /*
  while((temp = get_Temp()) == -1){
    delay(100);
  }
  */
  //temp = temp * 100;
  //temp = int(temp / 16);

  
  //**** Internal Temperature (RFM22) ******
  intTemp = rf22.temperatureRead( 0x00,0 ) / 2;  //from RFM22
  intTemp = intTemp - 64;
  
  //**** RSSI ******
  uint8_t rssi = rf22.rssiRead();

  //Put together the string
  n=sprintf(data, "%c%cL51.5,-0.05T%04ld,%dR%d[]", num_repeats, data_count, temp, intTemp, rssi);
  
  //scan through and insert the node_id into the data string
  // This will need to be moved later to allow for generation of dynamic
  // data strings
  uint8_t len_data = sizeof(data);
  for (int i=0; i<len_data; i++) {
    if (data[i] == '[') {
      data[i+1] = id;
      data[i+2] = ']';
      break;
    }
  }
}

void setup() 
{
  pinMode(rfm22_shutdown, OUTPUT);
  digitalWrite(rfm22_shutdown, LOW); //Turn the rfm22 radio on
  Serial.begin(9600);
  randomSeed(analogRead(0));
  
  //Read EEPROM to detect if we already have set an ID for this node
  //http://arduino.cc/en/Reference/EEPROMRead
  id = EEPROM.read(0);
  delay(500);
  
  if (!rf22.init()){
    //Serial.println("RF22 init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  }
  else{
    //Serial.println("RF22 Booted");
  }
  
  setupRFM22();
  delay(1000);
  
  gen_Data();
  //Send first packet
  //Serial.println("Sending first packet");
  Serial.print("tx: "); Serial.println((char*)data);
  rf22.send((byte*)data, sizeof(data));
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
        Serial.print("rx: ");
        Serial.println((char*)buf);
        
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
              
              //random delay to try and avoid packet collision
              delay(random(100, 500));
              
              //Serial.print("Repeat data: "); Serial.println((char*)buf);
              rf22.send(buf, sizeof(buf));
              rf22.waitPacketSent();
              break;
            }
          }
          

        }
        else{
          //Serial.print("Stop ");
          //Serial.println((char*)buf);
        }
      }
      else
      {
        //Serial.println("recv failed");
      }
    }
    else
    {
      //Serial.print(".");
    }
    
    if (count >= data_interval){
      
      //**** Packet Tx Count ******
      //using a byte to keep track of transmit count
      // its meant to roll over
      data_count++;
      //'a' packet is only sent on the first transmission so we need to move it along
      // when we roll over.
      // 98 = 'b' up to 122 = 'z'
      if(data_count > 122){
        data_count = 98; //'b'
      }
      
      gen_Data();
      
      
      //Serial.println("Sending");
      // Send a message
      
      Serial.print("tx: "); Serial.println((char*)data);
      rf22.send((byte*)data, sizeof(data));
      rf22.waitPacketSent();
      
      //**** Packet Tx Interval ******
      data_interval = random(1, 20) + count;
      //Serial.print("Next string on: ");
      //Serial.println(data_interval);
    }
  }
}

int16_t get_Temp(){
  byte i;
  byte present = 0;
  byte type_s;
  byte ds_data[12];
  byte addr[8];
  
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return(-1);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      return(-1);
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return(-1);
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    ds_data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (ds_data[1] << 8) | ds_data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (ds_data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - ds_data[6];
    }
  } else {
    byte cfg = (ds_data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  return(raw);
}
