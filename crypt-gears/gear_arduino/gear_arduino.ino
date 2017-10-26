// PIN NUMBERS
int output_lights[7] = {38, 39, 40, 41, 42, 43, 44};

int red_button = 30;

int pot = A0;

int photoresistors[3] = {A1, A2, A3};

// if the potentiometer is between pot_locs[2*i] and pod_locs[2*i+1], then we 
// say we are in position i.  otherwise, we are between positions.
int pot_locs[14] = {1024, 885, 825, 690, 640, 450, 390, 280, 190, 70, 50, 24, 12, 0};


// STATE OF THE WORLD

// 0: we have not started to calibrate.
// 1: we are calibrating.
// 2: we are unlocking.
// 3: we have won.
int state = 0;

// The last known state of the selector.
// NOTE: selector_pos starts at 0 when we begin to calibrate, goes up to 6 when
// we finish calibration and start checking positions, and then back to 0 when
// we perhaps win.
int last_selector_pos = 0;

// photoresistor_history[i][j] is the state of the photoresistor i when we were
// at selector position j.  Only meaningful in state 1, and then only for
// 0 <= j < the current selector gear position.  When we transition to state 2
// we use this to populate gear_types and gear_orientations.
// NOTE: The photoresistor is positioned down.
boolean photoresistor_history[3][7];

// The state of the ith gear, if we know it.  This is not its current position,
// but rather the position corresponding to selector gear 0!  Only meaningful
// in state 2; set from photoresistor_history when we transition there.
// Type is 0, 1, 2, in order of the correct solution.
// Orientation is 0 in the correct solution, 1 if we are one step clockwise
// from it, and so on.
int gear_types[3];
int gear_orientations[3];

// State of the output lights (should match output_lights).
// Only meaningful if we are in state 2.  If while in state 2,
// these are all true, we move to state 3 and win.
// Note they get filled in in reverse order (see note by selector pos).
boolean correct_positions[7];


// CONSTANTS DEFINING THE PUZZLE

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


// MAIN TOPLEVEL STUFF

// this runs once when the program starts
void setup() 
{
  Serial.begin(9600); 
  
  // init all output lights as outputs which are off
  for(int i=0; i<7; i++) 
  {
    pinMode(output_lights[i], OUTPUT);
    digitalWrite(output_lights[i], LOW);
  }
}

// this is run continuously after setup has completed
void loop() 
{
  delay(50);
  int selector_pos = getSelectorGearPosition();
  Serial.println(selector_pos);
  return;
  if (selector_pos == -2)
  {
    sendCastleErrorMessage();
    return;
  }
  else if (selector_pos == -1)
  {
    // we haven't made it to a new position yet, do nothing.
    return;
  }

  // LE GRAND STATE MACHINE
  // NOTE: with some exceptions the state machine falls through: if we advance
  // position we immediately do what we would have done there.
  if (state == 0)
  {
    // TODO(benkraft): Should we only be starting here when the red button is
    // pressed?
    if (selector_pos == 0)
    {
      // We have started calibrating!
      state = 1;
    }
  }

  if (state == 1)
  {
    if (selector_pos < last_selector_pos
        || selector_pos > last_selector_pos + 1)
    {
      // They have gone backwards or too fast.
      // TODO(benkraft): We may wish to debounce here, or say it's ok for them
      // to go back and forth as long as they get there eventually (and we only
      // consider the position they last got to in order).
      state = 0;
      resetLights();
    }
    else if (selector_pos == last_selector_pos + 1)
    {
      // Fill in new photoresistor states.
      // TODO(benkraft): We almost certainly want to wait until we're closer to
      // the middle of this state to check; probably this will be solved at the
      // same time as debouncing.
      for (int i=0; i<3; i++)
      {
        photoresistor_history[i][selector_pos] = getPhotoresistor(i);
      }

      // If we are at the end, move on to state 2.
      if (selector_pos == 6)
      {
        state = 2;
        // This fills in gear_types and gear_orientations.
        // We check that it passes each time, and that
        // we see one of each gear type.
        int failed = false;
        int types_seen = 0;
        for (int i=0; i<3; i++)
        {
          if (!computeGearPosition(i))
          {
            failed = true;
          }
          else
          {
            types_seen |= 1 << gear_types[i];
          }
        }

        if (types_seen != 7)
        {
          failed = true;
        }


        if (failed)
        {
          // Or should we say you failed calibration, not an error?
          sendCastleErrorMessage();
          state = 0;
          resetLights();
        }
        else
        {
          for (int i=0; i<7; i++)
          {
            correct_positions[i] = false;
          }
        }
      }
    }
  }

  if (state == 2)
  {
    if (selector_pos > last_selector_pos
        || selector_pos < last_selector_pos - 1)
    {
      // They have gone backwards or too fast.
      // TODO(benkraft): As above, we may wish to debounce.
      state = 0;
      resetLights();
    }
    else if (selector_pos == last_selector_pos - 1)
    {
      // Fill in new connection states.
      correct_positions[selector_pos] = haveConnection(selector_pos);
      if (correct_positions[selector_pos])
      {
        showCorrect(selector_pos);
      }

      if (selector_pos == 0)
      {
        // If we are back to the beginning, and have won, yay!
        boolean winning = true;
        for (int i=0; i<7; i++)
        {
          if (!correct_positions[i])
          {
            winning = false;
            break;
          }
        }

        if (winning)
        {
          state = 3;
          sendCastleSuccessMessage();
        }
      }
    }
  }

  // TODO(benkraft): How do we exit state 3?  When the button is pressed?

  last_selector_pos = selector_pos;
  delay(50);
}


// LOGIC FOR GEAR STUFF

// Reads photoresistor_history[i][], which must be filled in entirely, and sets
// gear_types[i] and gear_orientations[i].
// Returns true for success, false if there was an issue (in which case output
// arrays may not be set).
boolean computeGearPosition(int i)
{
  // Hold on tight; this is kinda bespoke and messy.
  int positions_white = 0;
  for (int j=0; j<7; j++)
  {
    positions_white += photoresistor_history[i][j];
  }

  // If we saw 2 white, they should be in 1, 4 or 2, 5, and that means we have
  // gear 1, in positions 2 or 1 respectively.
  if (positions_white == 2)
  {
    gear_types[i] = 1;
    if (photoresistor_history[i][1] && photoresistor_history[i][4])
    {
      gear_orientations[i] = 2;
      return true;
    }
    else if (photoresistor_history[i][2] && photoresistor_history[i][5])
    {
      gear_orientations[i] = 1;
      return true;
    }
    else
    {
      return false;
    }
  }

  // If we saw 3 white, they should be in 0, 3, 6, and that means we have gear
  // 1, in position 0.
  else if (positions_white == 3)
  {
    if (photoresistor_history[i][0] && photoresistor_history[i][3]
        && photoresistor_history[i][6])
    {
      gear_types[i] = 1;
      gear_orientations[i] = 0;
      return true;
    }
    else
    {
      return false;
    }
  }

  // If we saw 3 black, they should be in 0, 3, 6, and that means we have gear
  // 0 in position 1.
  else if (positions_white == 4)
  {
    if (!photoresistor_history[i][0] && !photoresistor_history[i][3]
        && !photoresistor_history[i][6])
    {
      gear_types[i] = 0;
      gear_orientations[i] = 1;
      return true;
    }
    else
    {
      return false;
    }
  }

  // If we saw 2 black, we could have them in 1, 4 or 2, 5, in which case we
  // have gear 0 in positions 0 or 2 respectively; or they could be in 0, 4 or
  // 1, 5 or 2, 6, in which case we have gear 2 in positions 1, 0, or 2
  // respectively.
  else if (positions_white == 5)
  {
    if (!photoresistor_history[i][1] && !photoresistor_history[i][4])
    {
      gear_types[i] = 0;
      gear_orientations[i] = 0;
      return true;
    }
    else if (!photoresistor_history[i][2] && !photoresistor_history[i][5])
    {
      gear_types[i] = 0;
      gear_orientations[i] = 2;
      return true;
    }
    else if (!photoresistor_history[i][0] && !photoresistor_history[i][4])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 1;
      return true;
    }
    else if (!photoresistor_history[i][1] && !photoresistor_history[i][5])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 0;
      return true;
    }
    else if (!photoresistor_history[i][2] && !photoresistor_history[i][6])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 3;
      return true;
    }
    else
    {
      return false;
    }
  }

  // If we saw 1 black, it should be in 3, and we have gear 2 in position 2.
  else if (positions_white == 6)
  {
    if (!photoresistor_history[i][3])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 2;
      return true;
      
    }
    else
    {
      return false;
    }
  }

  // Otherwise, something is wrong.
  else
  {
    return false;
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
boolean haveConnection(int rotation)
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


// COMMUNICATION

// turn on light / open solenoid for selector connection #n
void showCorrect(int n)
{
  // turn on light
  digitalWrite(output_lights[n], HIGH);
  
  // TODO(bhomberg): open solenoid??
}

// Reset all the lights.  (Caller should reset internal state.)
void resetLights()
{
  // turn off all lights
  for(int i=0; i<7; i++)
  {
    digitalWrite(output_lights[i], LOW);
  }

  // TODO(bhomberg): close all solenoids (??)
}

// Tell the user they have won!
void sendCastleSuccessMessage() 
{
  // 20's -- 2nd puzzle message
  // -0   -- first message from this puzzle
  Serial.println(20);
}

// Tell the user something has gone horribly wrong.
// Caller likely wants to reset internal state here.
void sendCastleErrorMessage()
{
  // TODO(benkraft): May or may not need to wait a tick to be sure here.
  // TODO(benkraft): May want to log debugging info here.
  Serial.println(21);
}

void printState()
{
  // TODO(benkraft): internal state too?
  // print out position of the pot
  Serial.print(getSelectorGearPosition());
  // print out the three photoresistors.
  for (int i=0; i<3; i++)
  {
    Serial.print(" ");
    Serial.print(getPhotoresistor(i));
  }
  Serial.println("");
}


// INPUT READING

// True if white gear is in front of photo resistor.  False if black paper.
boolean getPhotoresistor(int i) 
{
  // sees white gear
  if(analogRead(photoresistors[i]) > 850)
  {
    return true;
  }
  // sees black paper
  return false;
}

// based on the pot value, we know which of the 7 positions the selector gear is in
// returns an int 0-6 inclusive, -1 if we are in between positions, and -2 if we
// have an unreasonable value
int getSelectorGearPosition() 
{
  int val = analogRead(pot);
  Serial.print(val);
  Serial.print(" ");

  if (val > pot_locs[0])
  {
    return -2;
  }
  
  for (int i=0; i<7; i++)
  {
    if (val > pot_locs[2*i])
    {
      return -1;
    }
    else if (val <= pot_locs[2*i] && val >= pot_locs[2*i+1])
    {
      return i;
    }
  }

  return -2;
}
