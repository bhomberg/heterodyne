void setup() {
  Serial.begin(9600);
}

void loop() {
  //int s[] = {1, 2, 3, 4, 10, 11, 12, 13, 14, 15, 19, 20};
  int s[] = {2, 10, 19, 11, 2};
  
  for(int i=0; i<sizeof(s) / sizeof(int); i++)
  {
    Serial.println(s[i]);
    delay(5000);
  }
}
