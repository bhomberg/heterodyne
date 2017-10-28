// PIN NUMBERS
int output_lights[7] = {38, 39, 40, 41, 42, 43, 44};

int red_button = 30;

int calibration_light = 31;

int pot = A0;

int photoresistors[3] = {A1, A2, A3};

// CALIBRATION

// if the potentiometer is between pot_locs[2*i] and pod_locs[2*i+1], then we 
// say we are in position i.  otherwise, we are between positions.
// TODO(benkraft): Consider narrowing these.
int pot_locs[14] = {1024, 885,
                    825, 690,
                    640, 450,
                    390, 280,
                    190, 70,
                    50, 24,
                    12, 5};

// above this = white; below = black (unlodged is undefined)
int photoresistor_thresholds[3] = {90, 50, 70};

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

// These store the state of the photoresistors in the current calibration
// phose.  photoresistor_totals[i][j] is the sum of the values we saw from
// photoresistor i in selector position j, and photoresistor_counts[i][j] is
// the number of such values.  (We use these to compute the average value.)
// 
// Only meaningful in state 1, and then only for 0 <= j <= the current selector
// gear position.  When we transition to state 2 we use this to populate
// gear_types and gear_orientations.
// NOTE: The photoresistor is positioned down.
long int photoresistor_totals[3][7];
long int photoresistor_counts[3][7];

// The state of the ith gear, if we know it.  This is not its current position,
// but rather the position corresponding to selector gear 0!  Only meaningful
// in state 2; set from photoresistor_{totals,counts} when we transition there.
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

// State of various timers.  For the blinkers, -1 is not blinking,
// [0, BLINK_SPEED) is on, [BLINK_SPEED, 2*BLINK_SPEED) is off.
// For debug, we just log on 0.
int calibration_blink_state = -1;
int winning_blink_state = -1;
int debug_state = 0;

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

// SETTINGS

// How long in ms we wait between loops
int FRAME_SPEED = 10; // 100 fps
// How many frames to wait before toggling blinking lights
int BLINK_SPEED = 50; // 0.5s
// How many frames to wait before logging debug data, or 0 for never
int DEBUG_FREQ = 100; // 1s

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
  delay(FRAME_SPEED);

  printState();
  
  int selector_pos = getSelectorGearPosition();

  // LE GRAND STATE MACHINE
  // NOTE: with some exceptions the state machine falls through: if we advance
  // position we immediately do what we would have done there.
  if (state == 0)
  {
    setCalibration(false);
    if (selector_pos < 0)
    {
      return;
    }
    else if (selector_pos == 0 && buttonPressed())
    {
      // We have started calibrating!
      state = 1;
      // initialize photoresistor data.
      for (int i=0; i<3; i++)
      {
        for (int j=0; j<7; j++)
        {
          photoresistor_totals[i][j] = 0;
          photoresistor_counts[i][j] = 0;
        }
      }
    }
  }

  if (state == 1)
  {
    blinkCalibration();
    if (selector_pos >= 0 && (selector_pos < last_selector_pos
        || selector_pos > last_selector_pos + 1))
    {
      // They have gone backwards or too fast.
      state = 0;
      resetLights();
    }
    else if (selector_pos >= 0)
    {
      // Fill in new photoresistor states.
      for (int i=0; i<3; i++)
      {
        photoresistor_totals[i][selector_pos] += getRawPhotoresistor(i);
        photoresistor_counts[i][selector_pos]++;
      }
    }
    // If we are at the end, move on to state 2.
    else if (selector_pos < 0 && last_selector_pos == 6)
    {
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
          printGearRead(i, true);
        }
        else
        {
          types_seen |= 1 << gear_types[i];
          printGearRead(i, false);
        }
      }

      if (types_seen != 7)
      {
        failed = true;
      }


      if (failed)
      {
        state = 0;
        resetLights();
      }
      else
      {
        state = 2;
        for (int i=0; i<7; i++)
        {
          correct_positions[i] = false;
        }
      }
    }
    else
    {
      return;

    }
  }

  if (state == 2)
  {
    setCalibration(true);
    if (selector_pos >= 0 && (selector_pos > last_selector_pos
        || selector_pos < last_selector_pos - 1))
    {
      Serial.println("RESET");
      // They have gone backwards or too fast.
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
    }
    else if (last_selector_pos == 0)
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
        winning_blink_state = 0;
        sendCastleSuccessMessage();
      }
    
    }
    else if (selector_pos < 0)
    {
      return;
    }
  }

  if (state == 3)
  {
    if (buttonPressed())
    {
      state = 0;
      winning_blink_state = -1;
      resetLights();
    }
    else
    {
      blinkWinningLights();
    }
    return;
  }

  last_selector_pos = selector_pos;
}


// LOGIC FOR GEAR STUFF

// Reads photoresistor_history[i][], which must be filled in entirely, and sets
// gear_types[i] and gear_orientations[i].
// Returns true for success, false if there was an issue (in which case output
// arrays may not be set).
boolean computeGearPosition(int i)
{
  // Hold on tight; this is kinda bespoke and messy.

  // First, figure out what, on average, we saw.
  boolean photoresistor_history[7];
  int positions_white = 0;
  for (int j=0; j<7; j++)
  {
    // TODO(benkraft): If ambient light variation is an issue, we could compute
    // the largest and smallest values we saw, and if they are far enough
    // apart, set a dynamic threshold halfway between.
    photoresistor_history[j] = (
      photoresistor_totals[i][j] / photoresistor_counts[i][j]
      > photoresistor_thresholds[i]);
    Serial.print(photoresistor_history[j]);

    positions_white += photoresistor_history[j];
  }

  // If we saw 2 white, they should be in 1, 4 or 2, 5, and that means we have
  // gear 1, in positions 2 or 1 respectively.
  if (positions_white == 2)
  {
    gear_types[i] = 1;
    if (photoresistor_history[1] && photoresistor_history[4])
    {
      gear_orientations[i] = 1;
      return true;
    }
    else if (photoresistor_history[2] && photoresistor_history[5])
    {
      gear_orientations[i] = 2;
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
    if (photoresistor_history[0] && photoresistor_history[3]
        && photoresistor_history[6])
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
    if (!photoresistor_history[0] && !photoresistor_history[3]
        && !photoresistor_history[6])
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
    if (!photoresistor_history[1] && !photoresistor_history[4])
    {
      gear_types[i] = 0;
      gear_orientations[i] = 2;
      return true;
    }
    else if (!photoresistor_history[2] && !photoresistor_history[5])
    {
      gear_types[i] = 0;
      gear_orientations[i] = 0;
      return true;
    }
    else if (!photoresistor_history[0] && !photoresistor_history[4])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 2;
      return true;
    }
    else if (!photoresistor_history[1] && !photoresistor_history[5])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 3;
      return true;
    }
    else if (!photoresistor_history[2] && !photoresistor_history[6])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 0;
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
    if (!photoresistor_history[3])
    {
      gear_types[i] = 2;
      gear_orientations[i] = 1;
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
  digitalWrite(calibration_light, LOW);
}

void setCalibration(boolean on)
{
  calibration_blink_state = -1;
  if (on)
  {
    digitalWrite(calibration_light, HIGH);
  }
  else
  {
    digitalWrite(calibration_light, LOW);
  }
}

void blinkCalibration()
{
  calibration_blink_state = (calibration_blink_state + 1) % (2 * BLINK_SPEED);
  if (calibration_blink_state < BLINK_SPEED)
  {
    digitalWrite(calibration_light, HIGH);
  }
  else
  {
    digitalWrite(calibration_light, LOW);
  }
}

void blinkWinningLights()
{
  winning_blink_state = (winning_blink_state + 1) % (2 * BLINK_SPEED);
  int val;
  if (winning_blink_state < BLINK_SPEED)
  {
    val = HIGH;
  }
  else
  {
    val = LOW;
  }
  for (int i=0; i<7; i++)
  {
    digitalWrite(output_lights[i], val);
  }
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
  Serial.println(21);
}

void printState()
{
  if (DEBUG_FREQ <= 0)
  {
    return;
  }

  if (debug_state == 0)
  {
    // print out position of the pot
    Serial.print("pot: ");
    Serial.print(getSelectorGearPosition());
    Serial.print(" (");
    Serial.print(analogRead(pot));
    Serial.println(")");
    // print out the three photoresistors.
    Serial.print("photoresistors:");
    for (int i=0; i<3; i++)
    {
      Serial.print(" ");
      Serial.print(getRawPhotoresistor(i));
    }
    Serial.println("");
    
    Serial.print("state: ");
    Serial.println(state);
    Serial.print("last selector pos: ");
    Serial.println(last_selector_pos);

    // history gets printed when we finish it

    if (state == 2)
    {
      for (int i=0; i<3; i++)
      {
        Serial.print("gear ");
        Serial.print(i);
        Serial.print("type: ");
        Serial.print(gear_types[i]);
        Serial.print(", orientation: ");
        Serial.print(gear_orientations[i]);
        Serial.println("");
      }
    }
    
    Serial.println("");
  }

  debug_state = (debug_state + 1) % DEBUG_FREQ;
}


void printGearRead(int i, boolean failed)
{
  if (DEBUG_FREQ > 0)
  {
    Serial.print("GEAR ");
    Serial.print(i);
    if (failed)
    {
      Serial.println(" none");
    }
    else
    {
      Serial.print(" type ");
      Serial.print(gear_types[i]);
      Serial.print(", orientation ");
      Serial.println(gear_orientations[i]);
    }

    for (int i=0; i<3; i++)
    {
      Serial.print("history ");
      Serial.print(i);
      Serial.print(":");
      for (int j=0; j<7; j++)
      {
        Serial.print(" ");
        Serial.print(photoresistor_totals[i][j]);
        Serial.print("/");
        Serial.print(photoresistor_counts[i][j]);
        Serial.print("=");
        Serial.print(photoresistor_totals[i][j] / photoresistor_counts[i][j]);
      }
      Serial.println("");
    }
  }
}


// INPUT READING

// Raw value of the photoresistor.  We'll average this over time
// to figure out what it sees.
int getRawPhotoresistor(int i) 
{
  return analogRead(photoresistors[i]);
}

// based on the pot value, we know which of the 7 positions the selector gear is in
// returns an int 0-6 inclusive, -1 if we are in between positions, and -2 if we
// have an unreasonable value
int getSelectorGearPosition() 
{
  int val = analogRead(pot);

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

// True if the button is pressed.
boolean buttonPressed()
{
  return digitalRead(red_button);
}
