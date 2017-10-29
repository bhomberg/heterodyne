#include <PololuLedStrip.h>
#include <LiquidCrystal.h>

#define STRIP_LENGTH 41
#define GRIDSIZE 7

#define DEBUG_FLAG true
#define USE_SERIAL true

// GRB color order... 
#define RED rgb_color(0, 255, 0)
#define GREEN rgb_color(255, 0, 0)
#define BLUE rgb_color(0, 0, 255)
#define YELLOW rgb_color(255, 255, 0)
#define OFF rgb_color(0, 0, 0)

#define ORANGE rgb_color(143, 255, 0)
#define CYAN rgb_color(255, 0, 255)

// TODO(bhomberg): update pin numbers
// LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

PololuLedStrip<2> leds;
rgb_color colors[STRIP_LENGTH];

bool isVictorious = false;
bool isHot = false;
bool isBad = false;
bool isEmpty = false;
int badAttempts = 0;

#define SWITCH_ON 1
#define SWITCH_OFF 2
#define SWITCH_WALL 3  // That's a wall, not a switch.

#define NO 98  // Represents no pin
const uint16_t SWITCH_PINS[] = {
  12, 26,  9, 11, 18, NO, 32,
  NO, 17, 16, NO, 47, 50, 34,
   7,  6, 15,  8, 46, 51, 36,
  28, NO, 30, 22, 45, NO, 35,
  23, 37, 39, 41, 42, 44, 33,
  24, 19, 29, NO, 43, 52, NO,
  25, NO, 31, 38, 40, 49, 53,
};

const uint16_t LED_INDEXES[] = {
   6,  5, 38, 37, 32, NO, 31,
  NO,  4, 39, NO, 33, 29, 30,
   7,  3, 40, 36, 34, 28, 27,
   8, NO,  1,  0, 35, NO, 26,
   9,  2, 15, 16, 20, 21, 25,
  10, 12, 14, NO, 19, 22, NO,
  11, NO, 13, 17, 18, 23, 24,
};

// NEW:
#define E 99  // Empty cell. Everything else is a wall.
const int PUZZLE[] = {
  E, E, E, E, E, 0, E,
  2, E, E, 2, E, E, E,
  E, E, E, E, E, E, E,
  E, 1, E, E, E, 1, E,
  E, E, E, E, E, E, E,
  E, E, E, 1, E, E, 2,
  E, 1, E, E, E, E, E,
};

int getGridIndex(int row, int col) {
  return row * GRIDSIZE + col;
}

int getRow(int grid_index) {
  return grid_index / GRIDSIZE;
}

int getCol(int grid_index) {
  return grid_index % GRIDSIZE;
}

int getSwitchPin(int row, int col) {
  int index = getGridIndex(row, col);
  return SWITCH_PINS[index];
}

int getSwitchValue(int row, int col) {
  int switch_pin = getSwitchPin(row, col);
  if (switch_pin == NO) {
    return SWITCH_WALL;
  } else if (digitalRead(switch_pin)) {
    return SWITCH_OFF;
  } else {
    return SWITCH_ON;
  }
}

bool isSwitchOn(int row, int col) {
  if (!isValid(row, col)) {
    return false;
  }
  return getSwitchValue(row, col) == SWITCH_ON;
}

bool isValid(int row, int col) {
  if (row < 0 || col < 0) {
    return false;
  }
  if (row >= GRIDSIZE || col >= GRIDSIZE) {
    return false;
  }
  return true;
}

int directionalIlluminationCount(int row, int col, int dr, int dc) {
  row += dr;
  col += dc;
  int count = 0;
  while (isValid(row, col)) {
    if (isWall(row, col)) {
      break;
    }
    if (isSwitchOn(row, col)) {
      count += 1;
    }
    row += dr;
    col += dc;
  }
  return count;
}

int adjacentLightsCount(int row, int col) {
  return (
      isSwitchOn(row - 1, col) +
      isSwitchOn(row + 1, col) +
      isSwitchOn(row, col - 1) +
      isSwitchOn(row, col + 1)
  );
}

int illuminationCount(int row, int col) {
  int count = (
    directionalIlluminationCount(row, col, 1, 0) +
    directionalIlluminationCount(row, col, 0, 1) +
    directionalIlluminationCount(row, col, -1, 0) +
    directionalIlluminationCount(row, col, 0, -1));
  if (isSwitchOn(row, col)) {
    count += 1;
  }
  return count;
}

bool isWall(int row, int col) {
  int index = getGridIndex(row, col);
  return PUZZLE[index] != E;  // Not empty -> wall.
}

void setColor(int row, int col, rgb_color color) {
  int index = getGridIndex(row, col);
  uint16_t colorIndex = LED_INDEXES[index];
  if (colorIndex == NO) {
    return;
  }
  colors[colorIndex] = color;
}

void setAllBlue() {
  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
    int row = getRow(index);
    int col = getCol(index);
    setColor(row, col, BLUE);  // Victory! Make everything blue.
  }
}

void sendStartMessage() {
  if (USE_SERIAL) {
    Serial.println(10);  // Send start audio signal.
  }
}

void sendRedLightsMessage() {
  if (USE_SERIAL) {
    if (!isHot) {
      Serial.println(11);  // Send "Ahhhhhh!" audio signal.
    }
  }
}

void stopHotMessage() {
  if (USE_SERIAL) {
    Serial.println(19);  // Stop Ahhh!
  }
}

void sendBadLightsMessage() {
  if (USE_SERIAL) {
    if (badAttempts == 1) {
      Serial.println(12);  // Send "I don't like the lights." audio signal.
    } else if (badAttempts == 2) {
      Serial.println(13);  // Send "I don't like the lights." audio signal.      
    } else {
      Serial.println(14);  // Send "I don't like the lights." audio signal.      
    }
  }
}

void sendVictoryMessage() {
  if (USE_SERIAL) {
    Serial.println(15);  // Send win message.
  }
}

bool update() {
  bool anyInvalid = false;
  bool anyEmpty = false;
  bool anyWallConstrainsViolated = false;
  int switchesOn = 0;
  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
    int row = getRow(index);
    int col = getCol(index);
    if (isWall(row, col)) {
      continue;
    } else if (isSwitchOn(row, col)) {
      switchesOn++;
      if (illuminationCount(row, col) > 1) {
        anyInvalid = true;
        setColor(row, col, RED);  // Conflicting lights.
        sendRedLightsMessage();  // Send "Ahhhhhh!" audio signal.
      } else {
        setColor(row, col, GREEN);  // Light you've turned on directly.
      }
    } else {
      if (illuminationCount(row, col) > 0) {
        setColor(row, col, YELLOW);  // Light you've turned on indirectly.
      } else {
        anyEmpty = true;
        setColor(row, col, OFF);  // Light that's still off.
      }
    }
    // TODO(dbieber): Add wall counts.
    if (PUZZLE[index] != E) {
      int count = adjacentLightsCount(row, col);
      if (count != PUZZLE[index]) {
        anyWallConstrainsViolated = true;
      }
    }
  }

  if (switchesOn == 0) {
    badAttempts = 0;
  }
  if (isEmpty && switchesOn == 1) {
    sendStartMessage();
  }
  isEmpty = (switchesOn == 0);
  
  bool victory = !anyInvalid && !anyEmpty && !anyWallConstrainsViolated;

  if (!anyEmpty && !victory) { // Board is full but incorrect.
    if (!isBad) { // just got to bad state
      badAttempts++;
      sendBadLightsMessage();  // Send "I don't like the lights." audio signal.
    }
  }
  if (victory && !isVictorious) {
    // We've gone from a non-winning state to a winning state.
    sendVictoryMessage();
  }
  isVictorious = victory;

  if (isHot && !anyInvalid) { // was hot, now fixed.
    stopHotMessage();
  }
  isBad = !anyEmpty && !victory;
  isHot = anyInvalid;
  return victory;
}

// Color definitions
rgb_color getGreen() {
  return rgb_color(255, 0, 0);
}

rgb_color getBlue() {
  return rgb_color(0, 255, 0);
}

rgb_color getRed() {
  return rgb_color(0, 0, 255);
}

rgb_color getYellow() {
  return rgb_color(255, 0, 255);
}

rgb_color getOff() {
  return rgb_color(0, 0, 0);
}

// Arduino setup() and loop()
void setup() {
  isVictorious = false;
  isHot = false;
  isBad = false;
  isEmpty = false;
  badAttempts = 0;
  
  if (USE_SERIAL) {
    Serial.begin(9600);
  }
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(22, INPUT_PULLUP);
  // Initial switch pins as INPUTs.
//  for (int switch_index = 0; switch_index < GRIDSIZE * GRIDSIZE; switch_index++) {
//    int pin = getSwitchPin(getRow(switch_index), getCol(switch_index));
//    if (pin != NO) {
//      pinMode(pin, INPUT);      
//    }
//  }
  
  // PololuLedStripBase::interruptFriendly = true;

  // lcd.begin(20, 4);
  // lcd.print("turn the lights on!  but do it right!");
}

//void loop() {
//  Serial.println("DEBUG");
//  for (int pin = 2; pin <= 53; pin++) {
//    Serial.print(digitalRead(pin));
//    Serial.print(" ");
//  }
//  delay(100);
//}

//void loop() {
//  if (USE_SERIAL) {
//    Serial.println("DEBUG");
//  }
//  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
//    int value = getSwitchValue(getRow(index), getCol(index));
//    if (index % GRIDSIZE == 0) {
//      if (USE_SERIAL) {
//        Serial.println("");
//      }
//    }
//    if (value == SWITCH_WALL) {    
//      if (USE_SERIAL) {
//        Serial.print("X");
//      }
//    } else {
//      if (USE_SERIAL) {
//        Serial.print(value);
//      }
//    }
//    
//    if (USE_SERIAL) {
//      Serial.print(' ');
//    }
//  }
//
//  if (USE_SERIAL) {
//    Serial.print('\n');
//    Serial.print('\n');
//    Serial.flush();
//  }
//
//  delay(1000);
//}

//void loop() {
//  for(uint16_t i = 0; i < STRIP_LENGTH; i++)
//  {
//    colors[i] = CYAN;
//  }
//  if (true) { //digitalRead(22)) {
//    setColor(3, 3, GREEN);
//  }
//  else {
//    setColor(3, 3, GREEN);  
//  }
//  leds.write(colors, STRIP_LENGTH);
//  delay(50);
//}

//void loop() {
//  for(uint16_t i = 0; i < STRIP_LENGTH; i++)
//  {
//    colors[i] = OFF;
//  }
//  setColor(3, 3, BLUE);
//  setColor(3, 2, GREEN);
//  setColor(3, 0, RED);
//  setColor(3, 4, YELLOW);
//  setColor(3, 6, OFF);
//  setColor(0, 0, OFF);
//  setColor(0, 3, GREEN);
//  //  1. You must disconnect 5V to upload properly.
//  //  2. Why does the following not work?
////  if (isSwitchOn(3, 3)) {
////    setColor(4, 3, GREEN);
////  } else {
////    setColor(4, 3, RED);
////  }
//  setColor(0, 3, BLUE);
//  setColor(4, 3, YELLOW);
////  isSwitchOn(3, 3);
//  leds.write(colors, STRIP_LENGTH);
//  delay(10);
//}

void loop() {
  bool victory = update();
  if (victory) {
    delay(500);
    setAllBlue();
  }
  delay(50);
  leds.write(colors, STRIP_LENGTH);
}
