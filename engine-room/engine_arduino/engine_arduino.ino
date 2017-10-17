#include <PololuLedStrip.h>

// instantiate light strip on pin 2
PololuLedStrip<2> leds;

// list of all led colors in order
rgb_color colors[4];

void setup() 
{
  Serial.begin(9600); 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(53, INPUT);
}

void loop() 
{
  delay(20);
  
  // digitalRead of a pin is high if switch on, else low
  if (digitalRead(53)) 
  {
    colors[0] = getRed();
    colors[1] = getBlue();
    colors[2] = getGreen();
    colors[3] = getYellow();
  } else {
    colors[0] = getBlue();
    colors[1] = getYellow();
    colors[2] = getYellow();
    colors[3] = getBlue();
  }
  
  leds.write(colors, 4);

  Serial.println(10);
  
  
  // Sketch of eventual code
  // Read switch values
  // On switch change, set logic to change led colors
  // write new colors
  // On certain conditions (two reds in a row, etc) send serial command
}

rgb_color getGreen() 
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

rgb_color getRed() 
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

