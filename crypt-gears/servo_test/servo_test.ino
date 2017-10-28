#include <Servo.h>

Servo latch;
int servopin = 8;

void setup() 
{
  latch.attach(servopin);
}

void loop()
{
  latch.write(0);
  delay(10000);
  latch.write(180);
  delay(10000);
}
