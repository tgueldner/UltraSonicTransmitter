//#define RF69_COMPAT 0  // define this to use the RF69 driver i.s.o. RF12
#include <JeeLib.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define DEBUG 1

#define myNodeID 30          //node ID of Rx (range 0-30) 
#define network     210      //network group (can be in the range 1-250).
#define RF_freq RF12_433MHZ     //Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define ACK_TIME 50 // # of ms to wait for an ack

typedef struct { int distance, battery; } PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;  

const int emonTx_NodeID=10;            //emonTx node ID
const int LED_pin= 9;

// Update these with values suitable for your network.
byte mac[]    = {  0x20, 0x73, 0x76, 0x7C ,0xA0, 0xDC };
byte server[] = { 192, 168, 10, 230 };

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(LED_pin, HIGH);
}

void normal(char *str)
{
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(LED_pin, LOW);
}

void setup() {
  
  pinMode(OUTPUT, LED_pin);
  
  Serial.begin(9600); 
  Serial.println(F("RF12B Zisternen Gateway"));
  
  connectToMQTT();
  
  Serial.print(F("Node: ")); 
  Serial.print(myNodeID); 
  Serial.print(F("Freq: ")); 
  if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
  if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
  if (RF_freq == RF12_915MHZ) Serial.print("915Mhz");  
  Serial.print(F("Radio network: ")); 
  Serial.println(network);
 
  Serial.print(F("Init radio ..."));
  delay(32);
  rf12_set_cs(8);
  rf12_initialize(myNodeID, RF_freq, network);
  //rf12_set_cs(8);
  // wait another 2s for the power supply to settle
  delay(2000);
  Serial.println(F("done"));
}

void connectToMQTT() {
  Serial.print(F("Starting network ..."));
  if(Ethernet.begin(mac) == 0) {
    error("failed");
    return;
  } else {
    normal("done");	
  }
  
  // print your local IP address:
  Serial.print(F("IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();
  
  Serial.print(F("Connect to MQTT server..."));
  if (client.connect("zisterne")) {
    normal("done");
  } else {
    error("failed");
  }
}

void loop() {
  
  if (rf12_recvDone()) {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
      
      digitalWrite(LED_pin, HIGH);
      int node_id = (rf12_hdr & 0x1F);		  //extract nodeID from payload
#if DEBUG
      Serial.print("node "); Serial.println(node_id, DEC);
#endif
          
      emontx=*(PayloadTX*) rf12_data;            // Extract the data from the payload 
#if DEBUG
      Serial.print("distance: "); Serial.println(emontx.distance); 
      Serial.print("battery: "); Serial.println(emontx.battery);
#endif
      char value[5];
      sprintf(value, "%d", emontx.distance);
      char topic[30];
      sprintf(topic, "zisterne/sensor%d/distance", node_id);
#if DEBUG
      Serial.print("Sending [");
      Serial.print(value);
      Serial.print("] to ");
      Serial.println(topic);
#endif
      if(!client.connected()) {
        connectToMQTT();
      }
      client.publish(topic, value);
      delay(500);
      digitalWrite(LED_pin, LOW);
    }
  }
  
  digitalWrite(LED_pin, LOW);
  
  // ?
  //client.loop();
  
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

