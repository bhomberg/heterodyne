#include <PololuLedStrip.h>
#include <LiquidCrystal.h>

#define STRIP_LENGTH 41
#define GRIDSIZE 7

#define DEBUG_FLAG true
#define USE_SERIAL false

// GRB color order... 
#define RED rgb_color(0, 255, 0)
#define GREEN rgb_color(255, 0, 0)
#define BLUE rgb_color(0, 0, 255)
#define YELLOW rgb_color(255, 255, 0)
#define OFF rgb_color(0, 0, 0)

// TODO(bhomberg): update pin numbers
// LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

PololuLedStrip<2> leds;
rgb_color colors[STRIP_LENGTH];

#define SWITCH_ON 1
#define SWITCH_OFF 2
#define SWITCH_WALL 3  // That's a wall, not a switch.

#define NO 98  // Represents no pin
const int SWITCH_PINS[] = {
   8,  7,  6,  5,  4, NO,  3,
  NO, 13, 12, NO, 11, 10,  9,
  20, 19, 18, 17, 16, 15, 14,
  25, NO, 24, 23, 22, NO, 21,
  32, 31, 30, 29, 28, 27, 26,
  37, 36, 35, NO, 34, 33, NO,
  43, NO, 42, 41, 40, 39, 38,
};

const uint16_t LED_INDEXES[] = {
  13,  6, 30,  9, 36, NO, 38,
  NO,  7, 28, NO, 10, 32, 37,
   8, 19, 18, 17, 16, 15, 14,
  26, NO, 23, 22, 50, NO, 42,
  40, 29, 25, 27, 51, 46, 44,
  31, 33, 43, NO, 49, 48, NO,
  39, NO, 35, 45, 47, 53, 34,
};

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
    return SWITCH_ON;
  } else {
    return SWITCH_OFF;
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

void sendRedLightsMessage() {
  if (USE_SERIAL) {
    Serial.println(10);  // Send "Ahhhhhh!" audio signal.
  }
}

void sendBadLightsMessage() {
  if (USE_SERIAL) {
    Serial.println(11);  // Send "I don't like the lights." audio signal.
  }
}

bool update() {
  bool anyInvalid = false;
  bool anyEmpty = false;
  bool anyWallConstrainsViolated = false;
  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
    int row = getRow(index);
    int col = getCol(index);
    if (isWall(row, col)) {
      continue;
    } else if (isSwitchOn(row, col)) {
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
  
  bool victory = !anyInvalid && !anyEmpty && !anyWallConstrainsViolated;

  if (!anyEmpty && !victory) { // Board is full but incorrect.
    sendBadLightsMessage();  // Send "I don't like the lights." audio signal.
  }

  
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
  if (USE_SERIAL) {
    Serial.begin(9600);
  }
  // pinMode(LED_BUILTIN, OUTPUT);

  // Initial switch pins as INPUTs.
  for (int switch_index = 0; switch_index < GRIDSIZE * GRIDSIZE; switch_index++) {
    int pin = getSwitchPin(getRow(switch_index), getCol(switch_index));
    if (pin != NO) {
      pinMode(pin, INPUT);      
    }
  }
  
  // PololuLedStripBase::interruptFriendly = true;

  // lcd.begin(20, 4);
  // lcd.print("turn the lights on!  but do it right!");
}

//void loop() {
//  if (DEBUG_FLAG) {
//    if (USE_SERIAL) {
//      Serial.println("DEBUG");
//    }
//    for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
//      int value = getSwitchValue(getRow(index), getCol(index));
//      if (index % GRIDSIZE == 0) {
//        if (USE_SERIAL) {
//          Serial.println("");
//        }
//      }
//      if (value == SWITCH_WALL) {    
//        if (USE_SERIAL) {
//          Serial.print("X");
//        }
//      } else {
//        if (USE_SERIAL) {
//          Serial.print(value);
//        }
//      }
//      
//      if (USE_SERIAL) {
//        Serial.print(' ');
//      }
//    }
//
//    if (USE_SERIAL) {
//      Serial.print('\n');
//      Serial.print('\n');
//      Serial.flush();
//    }
////
//    if (isSwitchOn(3, 3)) {
//      //Serial.println("YES");
//      setColor(3, 3, RED);
//    } else {
//      //Serial.println("NO");
//      setColor(3, 3, BLUE);
//    }
//    leds.write(colors, STRIP_LENGTH);
//    delay(50);
//  } else {
//    bool victory = update();
//    // leds.write(colors, STRIP_LENGTH);
//    if (victory) {
//      delay(500);
//      setAllBlue();
//    }
//  }
//
//  delay(1000);
//}

void loop() {
  for(uint16_t i = 0; i < STRIP_LENGTH; i++)
  {
    colors[i] = OFF;
  }
  setColor(3, 3, BLUE);
  setColor(3, 2, GREEN);
  setColor(3, 0, RED);
  setColor(3, 4, YELLOW);
  setColor(3, 6, OFF);
  setColor(0, 0, OFF);
  setColor(0, 3, GREEN);
  //  1. You must disconnect 5V to upload properly.
  //  2. Why does the following not work?
//  if (isSwitchOn(3, 3)) {
//    setColor(4, 3, GREEN);
//  } else {
//    setColor(4, 3, RED);
//  }
  setColor(0, 3, BLUE);
  setColor(4, 3, YELLOW);
//  isSwitchOn(3, 3);
  leds.write(colors, STRIP_LENGTH);
  delay(10);
}

//void loop() {
//  bool victory = update();
//  leds.write(colors, STRIP_LENGTH);
//  delay(50);
//  if (victory) {
//    delay(500);
//    setAllBlue();
//  }
//}

