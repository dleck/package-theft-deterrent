/*
 * UCSD HARD Hacks April 1, 2016
 * Authors: Darren Eck, Janet Hoh
 * 
 */

#define BUZZER_PIN 0 //TODO: SET LATER
#define FORCE_PIN 22 //PB00

/*
 * Capacitive Touchpad
 * MPR121 -> SAM D21
 * GND -> GND
 * VCC -> VCC (3.3V)
 * SDA -> PA08
 * SCL -> PA09
 * IRQ -> PB04
 */
 

int forceReading = 0;
boolean isArmed = false;

void setup() {
  Serial.begin(115200);
  // pin initialization
  pinMode(FORCE_PIN, INPUT);

}

void loop() {
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
}

void checkKeypadCode() {
  // see if code entered matches alarm code
  // so, we have to check for reset button
  // and also keep track of the numbers entered so far,
  // then compare when the enter key is hit
  // --> if match, disarm
}

void checkPackages() {
  // check new force resistor value
  // compare force values
  // update force value OR trigger theft alarm
}





