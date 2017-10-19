// hard-coded pin numbers for input/output
int gear_connections[3][12] = {{ 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13}, 
                               {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25}, 
                               {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37}};
                               
int output_lights[7] = {38, 39, 40, 41, 42, 43, 44};

int gear_lodged[3] = {45, 46, 47};

int pot = A0;

// TODO(bhomberg): calculate these numbers for real once mechanism is built; this is approx
int pot_locs[8] = {100, 200, 300, 400, 500, 600, 700, 800};

// State of the output lights (should match output_lights).
// Reset when you move any gears.  This is the only state we need;
// everything about the positions of gears we determine at need.
// When each of these are set to 1, you win!
boolean correct_positions[7] = {
  false, false, false, false, false, false, false};

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
  
  // First, get the state of the world.
  int gear_types[3];
  int gear_orientations[3];
  boolean should_reset = false;
  for (int i=0; i<3; i++)
  {
    getOrientationAndTypeForGear(i, &gear_types[i], &gear_orientations[i]);
    showLodged(i, should_reset);
    if (gear_types[i] = -1)
    {
      should_reset = true;
    }
  }

  // If not all gears are lodged, reset -- which may be a no-op -- and exit.
  if (should_reset)
  {
    reset();
    return;
  }

  // TODO(benkraft): figure out whether there is a connection on current
  // selector position
  int selector_pos = getSelectorGearPosition();
  correct_positions[selector_pos] = haveConnection(
    selector_pos, gear_types, gear_orientations);

  // check if we won
  int winning = true;
  for (int i=0; i<7; i++)
  {
    if (!correct_positions[i])
    {
      winning = false;
    }
  }

  if (winning)
  {
    sendCastleSuccessMessage();
  }
}

// Check if we have a connection.  rotation is the amount we have rotated,
// relative to 0 being the start position.  (That is, the position of the
// selector gear.)
boolean haveConnection(int rotation, int gear_types[],
                       int gear_orientations[])
{
  // TODO(benkraft): Implement.
  return false;
}

void sendCastleSuccessMessage() 
{
  // 20's -- 2nd puzzle message
  // -0   -- first message from this puzzle
  Serial.println(20);
  // TODO(benkraft): Do nothing until we reset.
}

void sendCastleErrorMessage()
{
  // TODO(benkraft): May or may not need to wait a tick to be sure here.
  Serial.println(21);
}

// Return the type and orientation of the gear in the nth position, or -1 for
// both if no gear is lodged.
void getOrientationAndTypeForGear(int gear_num, int* type, int* orientation)
{
  // TODO(benkraft): Implement.
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

// turn on light for gear #n lodged, or not
void showLodged(int n, boolean isLodged)
{
  digitalWrite(gear_lodged[n], isLodged ? HIGH : LOW);
}

// gears have been taken out / selector gear goes back to beginning, reset everything
void reset()
{
  // turn off all lights
  for(int i=0; i<7; i++)
  {
    digitalWrite(output_lights[i], LOW);
  }
  for(int i=0; i<3; i++) 
  {
    digitalWrite(gear_lodged[i], LOW);
  }

  // reset internal state: you have to start over.
  for(int i=0; i<7; i++)
  {
    correct_positions[i] = 0;
  }

  // TODO(bhomberg): close all solenoids (??)
}



