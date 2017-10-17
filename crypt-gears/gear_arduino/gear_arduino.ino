// hard-coded pins
int gear_connections[3][12] = {{ 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13}, 
                               {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25}, 
                               {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37}};
                               
int output_lights[7] = {38, 39, 40, 41, 42, 43, 44};

int gear_lodged[3] = {45, 46, 47};

int pot = A0;

// TODO(bhomberg): calculate these numbers for real once mechanism is built; this is approx
int pot_locs[8] = {100, 200, 300, 400, 500, 600, 700, 800};

// TODO(benkraft): add any state variables you need

// this runs once when the program starts
void setup() 
{
  Serial.begin(9600); 
  
  // init all gear connections as inputs
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<12; j++)
    {
      pinMode(gear_connections[i][j], INPUT);
    }
  }
  
  // init all output lights as outputs which are off
  for(int i=0; i<7; i++) 
  {
    pinMode(output_lights[i], OUTPUT);
    digitalWrite(output_lights[i], LOW);
  }
  
  // init all outputs for gear being lodged correctly
  for(int i=0; i<3; i++)
  {
    pinMode(gear_lodged[i], OUTPUT);
    digitalWrite(gear_lodged[i], LOW);
  }
}

// this is run continuously after setup has completed
void loop() 
{
  delay(50);
  // print out position of the pot
  Serial.print(getSelectorGearPosition());
  Serial.print("  ");
  Serial.println(getConnection(gear_connections[0][0], gear_connections[0][1]));
  
  // TODO(benkraft)
  // this should probably be a state machine since the loop function loops
  // every cycle -- check if gears are removed, if so, reset (check by making sure gear orientations/types are the same)
  // first state -- gear insertion
  // some kind of check that all gears are good
  // second state -- move through selector gear connections
  // verify which ones should be connections
  // if connection is good, turn on specific light/solenoid for feedback
  // if all 7 connections were good, send message for success
  // if you need to send any other messages back to the castle, lemme know

}

void sendCastleSuccessMessage() 
{
  // 20's -- 2nd puzzle message
  // -0   -- first message from this puzzle
  Serial.println(20);
}

void sendCastleErrorMessage()
{
  Serial.println(21);
}

// TODO(benkraft): change return signature to however you want to represent gears
// returns which gear type is inserted + the orientation of the gear
void getOrientationAndTypeForGear(int gear_num)
{
  // so the basic idea here is that each gear is attached in 12 places
  // a pair of them will be connected
  // on gear A, they're consecutive
  // on gear B, they're 1 away
  // on gear C, they're 2 away
  // since they're flip symmetric(yay!) this will uniquely identify each gear + its orientation
  
  // example: check whether slots 0 and 1 are connected on gear #0
  getConnection(gear_connections[0][0], gear_connections[0][1]);
}

// all pins are tied to ground with a 12kOhm resistor
// we drive one of the pins high (changing its impedance mode) and read the other pin
// if high, we're connected, if low, not connected
// pins on separate gears will never be connected
// returns true if the pins are connected
// side effects: briefly turns pin1 high, but returns it to input mode after
boolean getConnection(int pin1, int pin2) 
{
  pinMode(pin1, OUTPUT);
  digitalWrite(pin1, HIGH);
  
  boolean rtrn = false;
  if (digitalRead(pin2)) {
    rtrn = true;
  }
  delay(2000);
  
  digitalWrite(pin1, LOW);  
  pinMode(pin1, INPUT);
  
  return rtrn;
}

// based on the pot value, we know which of the 7 positions the selector gear is in
// returns an int 0-6 inclusive or -1 in error cases
// if the pot value is out of range, throws an error and informs the castle operators that the puzzle is broken
// the position of the selector gear uniquely determines the position of all of the other gears
int getSelectorGearPosition() 
{
  int val = analogRead(pot);
  
  for (int i=0; i<7; i++)
  {
    if (val > pot_locs[i] && val < pot_locs[i+1])
    {
      return i;
    }
  }
  
  // else: val is below pot_locs[0] or greater than pot_locs[7]
  // puzzle is borken
  sendCastleErrorMessage();
  return -1;
}

// turn on light / open solenoid for selector connection #n
void showCorrect(int n)
{
  // turn on light
  digitalWrite(output_lights[n], HIGH);
  
  // TODO(bhomberg): open solenoid??
}

// gears have been taken out / selector gear goes back to beginning, reset everything
void reset()
{
  // TODO(benkraft): clear all known gears

  // turn off all lights
  for(int i=0; i<7; i++)
  {
    digitalWrite(output_lights[i], LOW);
  }
  for(int i=0; i<3; i++) 
  {
    digitalWrite(gear_lodged[i], LOW);
  }

  // TODO(bhomberg): close all solenoids (??)
}

