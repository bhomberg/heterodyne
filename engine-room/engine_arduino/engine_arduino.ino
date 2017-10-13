#include <PololuLedStrip.h>

PololuLedStrip<2> leds;
rgb_color colors[4];

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
  
  colors[0] = getRed();
  colors[1] = getBlue();
  colors[2] = getGreen();
  colors[3] = getYellow();
  
  leds.write(colors, 4);

  Serial.println('red'); // m is char*
}

rgb_color getRed() 
{
  rgb_color color;
  color.red = 255;
  color.blue = 0;
  color.green = 0;
  return color;
}

rgb_color getBlue() 
{
  rgb_color color;
  color.red = 0;
  color.blue = 255;
  color.green = 0;
  return color;
}

rgb_color getGreen() 
{
  rgb_color color;
  color.red = 0;
  color.blue = 0;
  color.green = 255;
  return color;
}

rgb_color getYellow() 
{
  rgb_color color;
  color.red = 255;
  color.blue = 0;
  color.green = 255;
  return color;
}

