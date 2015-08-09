//#define RF69_COMPAT 0  // define this to use the RF69 driver i.s.o. RF12
#include <JeeLib.h>

#define myNodeID 30          //node ID of Rx (range 0-30) 
#define network     210      //network group (can be in the range 1-250).
#define RF_freq RF12_433MHZ     //Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define ACK_TIME 50 // # of ms to wait for an ack

typedef struct { int distance, battery; } PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;  

const int emonTx_NodeID=10;            //emonTx node ID
const int LED_pin= 9;

void setup() {
  
  pinMode(OUTPUT, LED_pin);
  digitalWrite(LED_pin, HIGH);
  delay(1000);
  
  Serial.begin(9600); 
  Serial.println(F("RF12B Zisternen Gateway")); 
  
 Serial.print("Node: "); 
 Serial.print(myNodeID); 
 Serial.print(" Freq: "); 
 if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
 if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
 if (RF_freq == RF12_915MHZ) Serial.print("915Mhz");  
 Serial.print(" Network: "); 
 Serial.println(network);
 
  delay(32);
  rf12_initialize(myNodeID, RF_freq, network);
  // wait another 2s for the power supply to settle
  delay(2000);
  digitalWrite(LED_pin, LOW);
}

void loop() {
  
  if (rf12_recvDone()) {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
    
      int node_id = (rf12_hdr & 0x1F);		  //extract nodeID from payload
      Serial.print("node "); Serial.println(node_id, DEC);
          
      if (node_id == emonTx_NodeID)  {             //check data is coming from node with the correct ID
        digitalWrite(LED_pin, HIGH);
        emontx=*(PayloadTX*) rf12_data;            // Extract the data from the payload 
        Serial.print("distance: "); Serial.println(emontx.distance); 
        Serial.print("battery: "); Serial.println(emontx.battery); 
        delay(500);
        digitalWrite(LED_pin, LOW);
      }
    }
  }
}

