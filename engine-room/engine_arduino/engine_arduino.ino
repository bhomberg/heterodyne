#include <PololuLedStrip.h>
#include <LiquidCrystal.h>

const int GRIDSIZE = 7;
const int STRIP_LENGTH = GRIDSIZE * GRIDSIZE;

const int LED_STRIP_PIN = 2;
PololuLedStrip<LED_STRIP_PIN> leds;
rgb_color colors[STRIP_LENGTH];

const int SWITCH_ON = 1;
const int SWITCH_OFF = 2;
const int SWITCH_WALL = 3;  // That's a wall, not a switch.

const int NO = -1;  // Represents no pin
const int SWITCH_PINS_OFFSET = 0;
const int SWITCH_PINS[] = {
   2,  3,  4,  5,  6,  7,  8,
   9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50,
};

const int E = 99; // Empty cell. Everything else is a wall.
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
  return SWITCH_PINS[index] + SWITCH_PINS_OFFSET;
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

int isSwitchOn(int row, int col) {
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
  colors[index] = color;
}

void setAllBlue() {
  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
    int row = getRow(index);
    int col = getCol(index);
    setColor(row, col, getBlue());  // Victory! Make everything blue.
  }
}

bool update() {
  bool anyInvalid = false;
  bool anyEmpty = false;
  for (int index = 0; index < GRIDSIZE * GRIDSIZE; index++) {
    int row = getRow(index);
    int col = getCol(index);
    if (isWall(row, col)) {
      continue;
    } else if (isSwitchOn(row, col)) {
      if (illuminationCount(row, col) > 1) {
        anyInvalid = true;
        setColor(row, col, getRed());  // Conflicting lights.
        // TODO(dbieber): Send message
        // Serial.println(10);  // Send "Ahhhhhh!" audio signal.
      } else {
        setColor(row, col, getGreen());  // Light you've turned on directly.
      }
    } else {
      if (illuminationCount(row, col) > 0) {
        setColor(row, col, getYellow());  // Light you've turned on indirectly.
      } else {
        anyEmpty = true;
        setColor(row, col, getOff());  // Light that's still off.
      }
    }
  }
  bool victory = !anyInvalid && !anyEmpty;
  return victory;
}

// Color definitions
rgb_color getGreen() {
  rgb_color color;
  color.red = 255;
  color.blue = 0;
  color.green = 0;
  return color;
}

rgb_color getBlue() {
  rgb_color color;
  color.red = 0;
  color.blue = 255;
  color.green = 0;
  return color;
}

rgb_color getRed() {
  rgb_color color;
  color.red = 0;
  color.blue = 0;
  color.green = 255;
  return color;
}

rgb_color getYellow() {
  rgb_color color;
  color.red = 255;
  color.blue = 0;
  color.green = 255;
  return color;
}

rgb_color getOff() {
  // TODO(dbieber): How do you actually turn a light off?
  // bhomberg -- this should work as you expect it to
  rgb_color color;
  color.red = 0;
  color.blue = 0;
  color.green = 0;
  return color;
}

// TODO(bhomberg): update pin numbers
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// Arduino setup() and loop()
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initial switch pins as INPUTs.
  for (int switch_index = 0; switch_index < GRIDSIZE * GRIDSIZE; switch_index++) {
    int pin = getSwitchPin(getRow(switch_index), getCol(switch_index));
    pinMode(pin, INPUT);
  }

  lcd.begin(16, 2);
  lcd.print("turn the lights on!  but do it right!");
}

void loop() {
  delay(20);
  bool victory = update();
  leds.write(colors, STRIP_LENGTH);
  if (victory) {
    delay(500);
    setAllBlue();
    lcd.clear();
    lcd.print("good work! that was the Z4Z4P i needed!");
  }
}
