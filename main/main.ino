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

void setup() {
  Serial.begin(115200);
  // pin initialization
  pinMode(FORCE_PIN, INPUT);

}

void loop() {
  forceReading = analogRead(FORCE_PIN);
  Serial.println(forceReading);
  delay(100);
}






