/*
UKHASnet rf22_repeater code

based on rf22_client.pde/ino from the RF22 library

 Code for OneWire comes from:
// OneWire DS18S20, DS18B20, DS1822 Temperature Example
// http://www.pjrc.com/teensy/td_libs_OneWire.html

*/

#include <SPI.h>
#include <RF22.h>

//Required for DSB18b20 Temp sensor
#include <OneWire.h>

//*************Node ID Setup ****************/
char id = 'B';  //Please register your node on the UKHASnet website
int node_type = 0; //0 = gateway, 1 = repeater with DS18b20 and voltage, 2 = bridge (not implemented)
char location_string[] = "51.498,-0.0527";

//*************One Wire Setup ****************/
// Please update the pin you have attached the onewire sensor network to
// Also hard code the address (run the UKHASnet_first_run sketch first to
//  search for the addresses
OneWire  ds(5);  // on pin (a 4.7K resistor is necessary)
byte address0[8] = {0x28, 0x38, 0x59, 0x57, 0x3, 0x0, 0x0, 0x8E}; // Ext DS18B20 28 38 59 57 3 0 0 8E

//*************Pin Setup ****************/
int rfm22_shutdown = 3; //This is the pin the SDN pin from the rfm22 radio module is connected to
int batt_pin = 0; //ADC 0 - measure battery voltage
int charge_pin = 1; //ADC 1 - measure solar voltage

//*************Misc Setup ****************/
byte num_repeats = '3'; //The number of hops the message will make before stopping

int n, count = 1, data_interval = 8, path = 0, intTemp = 0, battV = 0, chargeV = 0, packet_len;
byte data_count = 97; // 'a'
float ftemp;
char tempbuf[12] = "0";
char data[RF22_MAX_MESSAGE_LEN];

// Singleton instance of the radio
RF22 rf22;


void setupRFM22(){  
  
  if (!rf22.init()){
    //Serial.println("RF22 init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  }
  else{
    //Serial.println("RF22 Booted");
  }
  //GFSK_Rb2Fd5
  rf22.setFrequency(869.50);
  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  rf22.setTxPower(RF22_TXPOW_17DBM);
  
  //Wider AFC
  rf22.spiWrite(0x02A, 0x10);
}

int gen_Data(){
  
  //**** Internal Temperature (RFM22) ******
  intTemp = rf22.temperatureRead( 0x00,0 ) / 2;  //from RFM22
  intTemp = intTemp - 64;
  
  //**** RSSI ******
  uint8_t rssi = rf22.lastRssi();
  
  //**** Battery Voltage ******
  //Node A 4.11 = 879, 3.72 = 794
  //  V = (0.00458 * raw) + 0.0769
  battV = analogRead(batt_pin);
  
  //**** Charger Voltage ******
  chargeV = analogRead(charge_pin);
  
  if(node_type == 0){
    //For Gateway (no sensors attached)
    // Location is hard coded
     n=sprintf(data, "%c%cL%sT%dR%d[%c]", num_repeats, data_count, location_string, intTemp, rssi, id);
  }
  else if(node_type == 1){
    
    //**** Temperature ******
    //Now we need to add the Temperature data (5bytes)
    long int temp = 0;
    
    temp = get_Temp(address0);
    if(temp != -99){
      ftemp = (float)temp / 16.0;
      
      //convert float to string
      dtostrf(ftemp, 4, 2, tempbuf);
      
      //Put together the string
      n=sprintf(data, "%c%cT%d,%sR%dV%d,%d[%c]", num_repeats, data_count, intTemp, tempbuf, rssi, battV, chargeV, id);
    }
    else{
      n=sprintf(data, "%c%cT%dR%dV%d,%d[%c]", num_repeats, data_count, intTemp, rssi, battV, chargeV, id);
    }
  }
  return n;
}

void setup() 
{
  pinMode(rfm22_shutdown, OUTPUT);
  digitalWrite(rfm22_shutdown, HIGH); //Turn the rfm22 radio off
  
  Serial.begin(9600);
  randomSeed(analogRead(6));
  
  digitalWrite(rfm22_shutdown, LOW); //Turn the rfm22 radio on
  delay(1000);
  setupRFM22();
  delay(1000);
  
  packet_len = gen_Data();
  //Send first packet
  //Serial.println("Sending first packet");
  Serial.print("tx: "); Serial.println((char*)data);
  rf22.send((byte*)data, packet_len);
  rf22.waitPacketSent();
}

void loop()
{
  while (1)
  {
    count++;
    
    // Listen for data
    uint8_t buf[RF22_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf22.waitAvailableTimeout(10000))
    { 
      // Should be a message for us now   
      if (rf22.recv(buf, &len))
      {
       //= ;
       Serial.print("rx: ");
       for (int i=0; i<len; i++) {
         Serial.print((char)buf[i]);
         if (buf[i] == ']'){
           Serial.println();
           break;
         }
       }
        
        
        
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
              packet_len = i + 2;
              
              //random delay to try and avoid packet collision
              delay(random(50, 500));
              
              //Serial.print("Repeat data: "); Serial.println((char*)buf);
              rf22.send(buf, packet_len);
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
      
      packet_len = gen_Data();
      
      //Serial.print("tx: "); Serial.println((char*)data);
      rf22.send((byte*)data, packet_len);
      rf22.waitPacketSent();
      
      //**** Packet Tx Interval ******
      data_interval = random(10, 30) + count;
      //Serial.print("Next string on: ");
      //Serial.println(data_interval);
    }
    
    if((count % 30) == 0){
    //Reboot Radio
    digitalWrite(rfm22_shutdown, HIGH);
    delay(1000);
    digitalWrite(rfm22_shutdown, LOW); // Turn on Radio
    delay(1000);
    setupRFM22();
    delay(1000);
    }
  }
}

int16_t get_Temp(byte addr[8]){
  byte i;
  byte present = 0;
  byte type_s;
  byte ds_data[12];

  if (OneWire::crc8(addr, 7) != addr[7]) {
      return(-99);
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
