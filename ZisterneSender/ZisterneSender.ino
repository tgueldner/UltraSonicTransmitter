#include <NewPing.h>
//#include <LowPower.h>
//#include <RFM12B.h>

#define TRIGGER_PIN  4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define VOLTAGE_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 500 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

//Simple RFM12B wireless demo - transimtter - no ack
//Glyn Hudson openenergymonitor.org GNU GPL V3 7/7/11
//Credit to JCW from Jeelabs.org for RFM12 
//#define RF69_COMPAT 0  // define this to use the RF69 driver i.s.o. RF12
#include <JeeLib.h>  //from jeelabs.org
#include <RF12.h>  //from jeelabs.org

#define myNodeID 10          //node ID of tx (range 0-30)
#define gatewayID 30
#define network     210      //network group (can be in the range 1-250).
#define RF_freq RF12_433MHZ     //Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define ACK_TIME 50 // # of ms to wait for an ack
#define MAX_RETRIES 5

typedef struct { int distance, battery; } PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;  

// Use pin 2 as wake up pin
const int wakeUpPin = 2;


//ISR(WDT_vect) { Sleepy::watchdogEvent(); }

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// Need an instance of the Radio Module
//RFM12B radio;

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int LED_PIN = 1;

void sendData()
{
    // Just a handler for the pin interrupt.
}
  
void setup()
{
  Serial.begin(9600); 
 
  // initialize the digital pin as an output.
  pinMode(LED_PIN, OUTPUT); 
  pinMode(wakeUpPin, INPUT); 
  pinMode(VOLTAGE_PIN, OUTPUT);
  
  Serial.println(F("RFM12B Transmitter - Zisterne"));

  Serial.print("Node: "); 
  Serial.print(myNodeID); 
  Serial.print(" Freq: "); 
  if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
  if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
  if (RF_freq == RF12_915MHZ) Serial.print("915Mhz"); 
  Serial.print(" Network: "); 
  Serial.println(network);
  
  initRadio();
}

void initRadio() {
  delay(32);
  rf12_initialize(myNodeID,RF_freq,network);   //Initialize RFM12 with settings defined above  

  rf12_easyInit(1);
  //radio.Initialize(myNodeID, RF_freq, network);
  //radio.Sleep(); //sleep right away to save power
  // wait another 2s for the power supply to settle
  //Sleepy::loseSomeTime(2000);  
  delay(2000);
}

int loopCounter = 0;
boolean wakeup = false;
boolean success = false;
  
void loop() { 
  if(loopCounter > 3) {
    loopCounter = 0;
    enterSleep();
  }
 
  // back on normal operations
  if(loopCounter == 0) {
    digitalWrite(VOLTAGE_PIN, HIGH);
    loopCounter++; delay(500);
  } else if(loopCounter == 1) {    
    emontx.distance=sonar.convert_cm(sonar.ping_median(5));
    emontx.battery=emontx.battery+4;
    Serial.print(F("distance:")); Serial.println(emontx.distance);
    loopCounter++; delay(500);
  } else if(loopCounter == 2) {
    Serial.print(F("Sending data..."));
    //rf12_recvDone();
    //rf12_sendNow(RF12_HDR_ACK, &emontx, sizeof emontx);
    //rf12_sendWait(2);
    //radio.Send(gatewayID, &emontx, sizeof emontx, true);
   rf12_easySend(&emontx, sizeof emontx);
   loopCounter++;
  } else if(loopCounter == 3) {
    int result = rf12_easyPoll();
    if(result == 1) {
      loopCounter++;
      Serial.println(F("done!"));
    } else if(result == 0){
      loopCounter++;
      Serial.println(F("error"));
    }
  }
}

void enterSleep(void)
{
  Serial.println(F("Going to sleep."));
  wakeup = false;
  // deactivate ultrasonic sensor pin
  digitalWrite(VOLTAGE_PIN, LOW);
  
  // go to sleep -> http://www.mikrocontroller.net/articles/RFM12#Zeitgeber_f.C3.BCr_Wake-Up_.28Wake-Up_Timer_Exxx_.._Fxxx.29
  //Bit	15 	14 	13 	12 	11 	10 	9 	8 	7 	6 	5 	4 	3 	2 	1 	0
  //←   1 	1 	1 	r4 	r3 	r2 	r1 	r0 	m7 	m6 	m5 	m4 	m3 	m2 	m1 	m0 	
  //int sleepTime = (B1111 * 256 ) + B11111111; // 1min
  int sleepTime = (B100 * 256 ) + B11111111; // 4sec
  rf12_control(RF_WAKEUP_TIMER | sleepTime);
  rf12_control(RF_WAKEUP_MODE);
  delay(100);
  
  /* Setup pin2 as an interrupt and attach handler. */
  attachInterrupt(0, pin2Interrupt, LOW);
  delay(100);
  
  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  //LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
  Sleepy::powerDown();
    
  /* The program will continue from here. */
  // Disable external pin interrupt on wake up pin.
  detachInterrupt(0); 
  Serial.println(F("Be back"));
  initRadio();
  wakeup = rf12_control(0x0000) && B10000;
}

void pin2Interrupt(void)
{
  /* This will bring us back from sleep. */
  
  /* We detach the interrupt to stop it from 
   * continuously firing while the interrupt pin
   * is low.
   */
  detachInterrupt(0);
}
