// hard-coded pin numbers for input/output
// TODO(benkraft): I am assuming for the sake of argument that these start at
// the top and go clockwise; fix once we know the correct positions.
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

int have_won = false;

// Structure of the gears, represented as pairs that are connected.
// Orientation is as described in getOrientationAndTypeForGear.
int gear_0_connections[6][2] = {
  {2, 5}, {2, 8}, {2, 11}, {5, 8}, {5, 11}, {8, 11}};

int gear_1_connections[12][2] = {
  {0, 5}, {0, 7}, {5, 7},
  {1, 6}, {1, 11}, {6, 11},
  {2, 4}, {2, 9}, {4, 9},
  {3, 8}, {3, 10}, {8, 10}};

int gear_2_connections[9][2] = {
  {0, 4}, {0, 8}, {4, 8},
  {1, 5}, {1, 9}, {5, 9},
  {2, 6}, {2, 10}, {6, 10}};

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
  } else if (have_won) {
    // If we are not resetting, and we have already won, no need to do more
    // work; just wait for the reset.
    return;
  }

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
    have_won = true;
    sendCastleSuccessMessage();
  }
}

// True if positions pos1 and pos2 are connected on gear_num.
boolean positionsConnected(int gear_num, int pos1, int pos2)
{
  // We just do things the simplistic way: do a bunch of casework and search
  // through the array one at a time.
  if (gear_num == 0)
  {
    for (int i=0; i<6; i++)
    {
      if (gear_0_connections[i][0] == pos1
          && gear_0_connections[i][1] == pos2
          || gear_0_connections[i][0] == pos2
          && gear_0_connections[i][1] == pos1)
      {
        return true;
      }
    }
  }
  else if (gear_num == 1)
  {
    for (int i=0; i<12; i++)
    {
      if (gear_1_connections[i][0] == pos1
          && gear_1_connections[i][1] == pos2
          || gear_1_connections[i][0] == pos2
          && gear_1_connections[i][1] == pos1)
      {
        return true;
      }
    }
  }
  else  // (gear_num == 2)
  {
    for (int i=0; i<9; i++)
    {
      if (gear_2_connections[i][0] == pos1
          && gear_2_connections[i][1] == pos2
          || gear_2_connections[i][0] == pos2
          && gear_2_connections[i][1] == pos1)
      {
        return true;
      }
    }
  }
  return false;
}

// Check if we have a connection.  rotation is the amount we have rotated,
// relative to 0 being the start position.  (That is, the position of the
// selector gear.)  Call only if all gears are lodged.
boolean haveConnection(int rotation, int gear_types[],
                       int gear_orientations[])
{
  // TODO(benkraft): Make sure these match the actual directions of rotation in
  // the final mechanism.
  int rotation_0 = (rotation + gear_orientations[0]) % 12;
  int rotation_1 = (12 - rotation + gear_orientations[1]) % 12;
  int rotation_2 = (rotation + gear_orientations[2]) % 12;
  // These are indexed by the ordering on the left side.
  boolean first_connections[3] = {false, false, false};
  boolean second_connections[3] = {false, false, false};
  for (int i = 11; i<14; i++)
  {
    for (int j = 2; j<5; j++)
    {
      if (positionsConnected(
            gear_types[0], (rotation_0 + i) % 12, (rotation_0 + j) % 12))
      {
        first_connections[j - 2] = true;
      }
    }
  }
  for (int i = 8; i<11; i++)
  {
    for (int j = 2; j<5; j++)
    {
      if (first_connections[(12 - i) % 3] && positionsConnected(
            gear_types[0], (rotation_0 + i) % 12, (rotation_0 + j) % 12))
      {
        second_connections[j - 2] = true;
      }
    }
  }
  for (int i = 8; i<11; i++)
  {
    for (int j = 0; j<2; j++)
    {
      if (second_connections[i % 3] && positionsConnected(
            gear_types[0], (rotation_0 + i) % 12, (rotation_0 + j) % 12))
      {
        return true;
      }
    }
  }
  return false;
}

void sendCastleSuccessMessage() 
{
  // 20's -- 2nd puzzle message
  // -0   -- first message from this puzzle
  Serial.println(20);
}

void sendCastleErrorMessage()
{
  // TODO(benkraft): May or may not need to wait a tick to be sure here.
  Serial.println(21);
}

// Return the type and orientation of the gear in the nth position, or -1 for
// both if no gear is lodged.
// Type is 0, 1, 2, in order of the correct solution.
// Orientation is 0 in the correct solution, 1 if we are one step clockwise
// from it, and so on.
void getOrientationAndTypeForGear(int gear_num, int* type, int* orientation)
{
  *type = -1;
  *orientation = -1;
  for (int i=0; i<12; i++) {
    if (getGearConnection(gear_num, i, (i + 3) % 12))
    {
      *type = 0;
      *orientation = (4 + i) % 12;  // actually equivalent mod 3
      return;
    }
    else if (getGearConnection(gear_num, i, (i + 6) % 12))
    {
      *type = 1;
      *orientation = i % 6;  // actually equivalent mod 3
      return;
    }
    else if (getGearConnection(gear_num, i, (i + 2) % 12))
    {
      *type = 2;
      *orientation = (4 + i) % 12;  // actually equivalent mod 4
      return;
    }
  }
}

boolean getGearConnection(int gear_num, int slot1, int slot2)
{
  return getConnection(gear_connections[gear_num][slot1],
                       gear_connections[gear_num][slot2]);
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
  have_won = false;
  for(int i=0; i<7; i++)
  {
    correct_positions[i] = 0;
  }

  // TODO(bhomberg): close all solenoids (??)
}
