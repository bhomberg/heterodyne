const int buttons[] = {2, 3, 4, 5, 6, 7}

void setup() {
  for(int i=0; i < 6; i++)
  {
    pinMode(buttons[i], INPUT);
  }
}

void loop() {
  for(int i=0; i<6; i++)
  {
    s = digitalRead(buttons[i]);

    // button is on
    if(s == HIGH)
    {
      Serial.println(i+1);
      delay(1000); // debounce
    }

  }
}