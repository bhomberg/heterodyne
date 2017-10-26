const int buttons[] = {2, 3, 4, 5, 6};

void setup() {
  Serial.begin(9600);

  for(int i=0; i < 5; i++)
  {
    pinMode(buttons[i], INPUT);
  }
}

void loop() {
  for(int i=0; i<5; i++)
  {
    int s = digitalRead(buttons[i]);

    // button is on
    if(s == HIGH)
    {
      Serial.println(buttons[i]);
      delay(1000); // debounce
    }

  }
}
