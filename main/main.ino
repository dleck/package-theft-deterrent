/*
 * UCSD HARD Hacks April 1, 2016
 * Authors: Darren Eck, Janet Hoh
 * 
 */

#define FORCE_RATIO_THRESHOLD 0.9
#define FORCE_FLAT_THRESHOLD 50

/* begin Touchpad Initializations */
#include <Wire.h>
#include "Adafruit_MPR121.h"
#define _BV(bit) (1 << (bit))   // added for compatibility

/* Pubnub stuff */
#include <SPI.h>
#include <WiFi101.h>
#include <PubNub.h>

static char ssid[] = "deck";         // your network SSID (name)
static char pass[] = "hannah1337";        // your network password
int status = WL_IDLE_STATUS;                  // the Wifi radio's status

const static char pubkey[] = "pub-c-50f8f9ce-6321-4407-a581-ebc1c40e570f";
const static char subkey[] = "sub-c-1815d148-f8ba-11e5-a492-02ee2ddab7fe";
const static char channel[] = "test_channel";

char uart_str[80];
int adc_data[4];
WiFiClient *client;
char msg[40];


// You can have up to 4 on one i2c bus but one is enough!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
/* end Touchpad Initializations */


#define BUZZER_PIN 2  //PB30
#define FORCE_PIN 22  //PB00

#define GREEN_LED_PIN 4 //PB06
#define RED_LED_PIN 5   //PB07

#define CODE_LENGTH 4 // if you change this, change code[] initialization!

/*
 * Capacitive Touchpad
 * MPR121 -> SAM D21
 * GND -> GND
 * VCC -> VCC (3.3V)
 * SDA -> PA08
 * SCL -> PA09
 * IRQ -> PB04
 */
 

int newForceReading = 0;
int oldForceReading = 0;
boolean isArmed = false;

int code[CODE_LENGTH] = {0, 0, 0, 0};
int currCode[CODE_LENGTH] = {0, 0, 0, 0};
int codeCount = 0;

void setup() {
  while (!Serial);        // needed to keep from starting too fast
  Serial.begin(115200);
  
  // pin initialization
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FORCE_PIN, INPUT);

  // touchpad initialization
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found");
    while (1);
  }
  Serial.println("MPR121 found!");


  pubNubSetup();
  // LED Initialization
  initLEDs();
}

void loop() {
  
  monitorPackages();  //temporary to test

  if (isArmed) {
    monitorPackages();
  }
  else {
    setCode();
  }
}


void monitorPackages() {
  checkKeypadCode();
  checkPackages();
}

void setCode() {
  // keep track of code entered
  // record code after enter key is hit
  // FINALLY arm device

  // Get the currently touched pads
  currtouched = cap.touched();

  // TODO: solve for case where user presses multiple buttons SUPER SIMULANEOUSLY
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) && lasttouched == 0) {
//      Serial.print(i); Serial.println(" touched");
      code[codeCount] = i;
      codeCount++;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
//      Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;

  // full code entered, arm device
  if (!(codeCount < 4)) {
    // wait for all buttons to be released
    while(lasttouched != 0) {
      lasttouched = cap.touched();
    }

    // arm device
    ARM();
    // reset code entered
    codeCount = 0;
    Serial.print("Code: ");
    for (int i=0; i<CODE_LENGTH; i++) {
      Serial.print(code[i]);
    }
    Serial.println();
  }
  
}

void checkKeypadCode() {
  // see if code entered matches alarm code
  // so, we have to check for reset button
  // and also keep track of the numbers entered so far,
  // then compare when the enter key is hit
  // --> if match, disarm

  // Get the currently touched pads
  currtouched = cap.touched();

  // TODO: solve for case where user presses multiple buttons SUPER SIMULANEOUSLY
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) && lasttouched == 0) {
//      Serial.print(i); Serial.println(" touched");
      currCode[codeCount] = i;
      codeCount++;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
//      Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;

  // full code entered, check if correct
  if (!(codeCount < 4)) {
    // wait for all buttons to be released
    while(lasttouched != 0) {
      lasttouched = cap.touched();
    }

    // check code
    boolean isMatched = true;
    for (int i=0; i < CODE_LENGTH; i++) {
      if (code[i] != currCode[i]) {
        isMatched = false;
      }
    }

    if (isMatched) {
       Serial.println("CorrectCode!");
       // Disarm
       DISARM();
    }

    else {
       Serial.println("Wrong Code!");
       flashRedLED();
    }

    Serial.print("Correct Code: ");
    for (int i=0; i<CODE_LENGTH; i++) {
      Serial.print(code[i]);
    }
    Serial.println();
    Serial.print("Code Entered: ");
    for (int i=0; i<CODE_LENGTH; i++) {
      Serial.print(currCode[i]);
    }
    Serial.println();

    // reset code entered
    codeCount = 0;
  }
  
}

void checkPackages() {
  // check new force resistor value
  oldForceReading = newForceReading;
  newForceReading = analogRead(FORCE_PIN);
  
  // compare force values
  // update force value OR trigger theft alarm

  // trigger alarm
  float ratio = float(newForceReading)/float(oldForceReading);
  if ( oldForceReading > FORCE_FLAT_THRESHOLD && ratio < FORCE_RATIO_THRESHOLD ) {
    digitalWrite(BUZZER_PIN, HIGH);
    alertPhone();
    Serial.print("ALARM ACTIVATED! ");
  }

  Serial.print(oldForceReading > FORCE_FLAT_THRESHOLD);
  Serial.print(ratio < FORCE_RATIO_THRESHOLD);
  Serial.print("oldForceReading = ");
  Serial.print(oldForceReading);
  Serial.print(", newForceReading = ");
  Serial.print(newForceReading);
  Serial.print(", ratio = ");
  Serial.println(ratio);
  delay(1000);
}





/*---------------------LED HELPER FUNCTIONS---------------------------*/
void initLEDs() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
}
void greenLEDOn() {
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void greenLEDOff() {
  digitalWrite(GREEN_LED_PIN, LOW);
}

void redLEDOn() {
  digitalWrite(RED_LED_PIN, HIGH);
}

void redLEDOff() {
  digitalWrite(RED_LED_PIN, LOW);
}

void toggleLED(int ledPin) {
  digitalWrite(ledPin, !digitalRead(ledPin));
}

/*
 * This function leaves the red led ON after flashing
 */
void flashRedLED() {
  for (int i=0; i < 5; i++) {
    redLEDOff();
    delay(50);
    redLEDOn();
    delay(50);
  }
}

//void flashLED(int ledPin) {
//  // remember starting state of led
//  boolean lastState = digitalRead(ledPin);
//  for (int i=0; i < 5; i++) {
//    toggleLED(ledPin);
//    delay(50);
//  }
//  // set led back to original state
//  digitalWrite(ledPin, HIGH);
//}

/*--------------------------ARMING HELPER FUNCTIONS----------------------*/
void ARM() {
  isArmed = true;
  greenLEDOff();
  redLEDOn();

  Serial.println("Device Armed!");
}

void DISARM() {
  isArmed = false;
  redLEDOff();
  greenLEDOn();

  Serial.println("Unarmed!");
}






/*------------------------- PUBNUB STUFF ------------------------ */
void read_adc_channels(void)
{
  int channel_id;

  Serial.println("Reading ADC channels 0 - 3");

  for(channel_id = 0; channel_id < 4;channel_id++)
  {
    adc_data[channel_id] = analogRead(channel_id);
    sprintf(uart_str,"ch %d : %d\r\n",channel_id,adc_data[channel_id]);
    Serial.println(uart_str);
  }
}

void pubNubSetup() {
  
  while(!Serial) ;

  Serial.println("PubNub Arduino Zero WiFi101 example project V1.0");
  Serial.println("Jan 2016 / Atmel Corp");

  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi101 shield not present");

    while (true);
    // attempt to connect using WEP encryption:
  }
  Serial.println("Initializing Wifi...");
  // printMacAddress();

  String fv = WiFi.firmwareVersion();
  Serial.print("firmware version ");
  Serial.println(fv);


  // attempt to connect using WPA2 encryption:
  Serial.println("Connect to network...");
  status = WiFi.begin(ssid, pass);

  // if you're not connected, stop here:
  if ( status != WL_CONNECTED)
  {
    Serial.println("Connect fail");
    while (true);
  }
  else
  {
    Serial.print("connected to SSID: ");
    Serial.println(ssid);

    PubNub.begin(pubkey, subkey);
    Serial.println("PubNub set up");
  }
  
}

void alertPhone()
{
  /*
  read_adc_channels();

  sprintf(msg,"{\"ch0\":\"%d\"}",adc_data[0]);
  client = PubNub.publish(channel, msg);

  sprintf(msg,"{\"ch1\":\"%d\"}",adc_data[1]);
  client = PubNub.publish(channel, msg);

  sprintf(msg,"{\"ch2\":\"%d\"}",adc_data[2]);
  client = PubNub.publish(channel, msg);

  sprintf(msg,"{\"ch3\":\"%d\"}",adc_data[3]);
  client = PubNub.publish(channel, msg);
  */

  /*
  sprintf(buf, "{\"device\":\"%s\", \"temperature\":\"%d.%d\", \"light\":\"%d\", \"led\":\"%s\"}",
            PubNubChannel,
            (int)temperature, (int)((int)(temperature * 100) % 100),
            (((4096 - light) * 100) / 4096),
            port_pin_get_output_level(LED0_PIN) ? "0" : "1");

  */

  client = PubNub.publish(channel,"\"Someone stole your shit\"" );
  /*
  if (!client)
  {
    Serial.println("publishing error");
  }
  else
  {
    Serial.println("msg published");
  }
  */
  
  client->stop();

  //delay(1000);
}













