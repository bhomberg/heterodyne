#include <PololuLedStrip.h>



void setup() 
{
  Serial.begin(9600); 
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() 
{
  // TEMP, delete once real LEDs come in
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);
  
  

  Serial.println('red'); // m is char*
}

void red()
{
  
}


