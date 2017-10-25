void setup() {
  Serial.begin(9600);
}

void loop() {
  int s[] = {1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 19, 20, 21};
  
  for(int i=0; i<13; i++)
  {
    Serial.println(s[i]);
    delay(2000);
  }
}
