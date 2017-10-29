#include <Servo.h>

Servo latch;
int servopin = 10;

void setup() 
{
  latch.attach(servopin);
}

void loop()
{
  latch.write(0);
  delay(3000);
  latch.write(180);
  delay(6000);
}
