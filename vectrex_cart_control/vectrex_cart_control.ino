// Multi ROM cartridge selector
// for the VECTREX game system
// By A.Leverett 2021 (Coronavirus pandemic era)
// Extended by Phillip Riscombe-Burton 2022 (Ukraine invasion era)
//
// ROM_MAX should be set to the number of ROMS on the EPROM.
// If you have 5 ROM images on the EPROM then this no. should be 5
// If you have 128 then this no. should be 128
// Update the 'titles' String array with the names of the games on your EPROM.
//
// Remember this program is for both 'DIY Cart+8' and 'DIY Cart+32'
// Update the 'IS_8K_CART' bool depending on which you are building for.
// 'DIY Cart+8' is for 8KB images and 'DIY Cart+32' is for 32KB images.
// 8KB images will work on the +32 version as they only take up 8KB of the 32KB available
// 32KB images WILL NOT WORK on the +8 version
//
// Compatible with these ( and pin compatible ) EPROMs and how many images they will store.
// EPROM              8KB   32KB
// 27C010 / 27c1001   16    4
// 27C020 / 27c2001   32    8
// 27C040 / 27c4001   64    16 
// 27C080 / 27c801    128   32
//
// Save routine added. If you hold select until the display shows 'Saved!' then when you
// next power up the system with the cartridge in, the saved cartridge number will be selected 
// automatically!
//
//----------------------------------------------------------------------------------------

#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include "EEPROM.h"
#include <Wire.h>

#define RTN_CHECK 0          // oled - ticker scrolls text off screen
#define I2C_ADDRESS 0x3C     // 0X3C+SA0 - 0x3C or 0x3D
#define PIN_DOWN 8           // '-1' button
#define PIN_SELECT 9         // 'select' button
#define PIN_UP 10            // '+1' button
#define PIN_RESET 13         // reset pin ( only works from cart list program (rom 01))

// Either 8K or 32K cart
#define IS_8K_CART false

// Default game lists:
#if IS_8K_CART
  // x35 8K games requires 27c040 / 27c4001
  #define ROM_MAX 35
#else
  // x21 32K games requires 27c080 / 27c801 (x16 for 27c040 / 27c4001)
  #define ROM_MAX 21
#endif

#define FIRST_ROM 1 // Ignores 'Cart Menu' entry
#define SAVED_GAME_EPROM_ADDR 0 
#define SAVE_BUTTON_DURATION_MIN 2000
#define DEBOUNCE_DELAY 50
#define TICKER_TICK 50
#define MAX_CHAR_2X 11
#define WELCOME_MSG_SCREEN_TIME 6000
#define SELECTED_MSG_SCREEN_TIME 400
#define SAVED_MSG_SCREEN_TIME 800
#define RESET_DELAY 25

// Variables
int romNo = FIRST_ROM, romSave, selectButton, upButton, downButton;
String lastTitle;

// OLED
SSD1306AsciiAvrI2c oled;
TickerState state;
uint32_t tickTime = 0;

// Buttons
uint32_t lastDebounceTimeSelect = 0, lastSelectTime = 0;
int selectState = HIGH, lastSelectState = HIGH;

uint32_t lastDebounceTimeUp = 0, lastUpTime = 0;
int upState = HIGH, lastUpState = HIGH;

uint32_t lastDebounceTimeDown = 0, lastDownTime = 0;
int downState = HIGH, lastDownState = HIGH;

// Dispaly text for selected game
String titles[] = {
#if IS_8K_CART
  "Cart Menu", // Unused
  "Space Wars", 
  "Solar Quest",
  "Berzerk",
  "Clean Sweep",
  "Armor Attack",
  "Bedlam",
  "MineStormII",
  "Rip-Off",
  "Cosmic Chasm",
  "Hyperchase",
  "Test Rev. 4",
  "Scramble",
  "Star Castle",
  "Star Hawk",
  "Star Trek",
  "Heads Up",
  "Pole Position",
  "Fortress of Narzod",
  "Tour De France",
  "Mine Storm",
  "Polar Rescue",
  "Spike",
  "Blitz",
  "Mr Boston",
  "Spinball",
  "Web Wars",
  "Pitchers Duel",
  "3-D Mine Storm",
  "3-D Narrow Escape",
  "3-D Crazy Coaster",
  "Mail Plane",
  "AnimAction",
  "Melody Master",
  "Art Master"
#else
  "Menu - Alan Leverett",  // Initially skipped
  "Blox - Whitehat",
  "Castle Defender - Jumpman",
  "Castle Vs Castle - Hydrochous",
  "Climb it - Chris++",
  "Crash Trexicoot - LiKa",
  "Daisy Land - Princess Daisy",
  "Dino Runner - Hoid",
  "Donkey Kong - Lionpride",
  "Dont Fall - Mr. Chomp",
  "Fencing Simulator - Alberich",
  "Hole Run - Speedy G.",
  "Maze of Treasures - Sgt. Pepper",
  "Moon Shot - SchellLabs",
  "Pirates - G. Freeman",
  "Rush Defence - #Compile",
  "Silver Surfer - Hassel",
  "Space Defender - KusoTech",
  "Unknown - M4K5",
  "Vec Man - Master Control",
  "Vectroid - NATE__66"
#endif
};

//----------------------------------------------------------------------------------------

// Arduino methods

void setup() {

  setupTimeKeeping();
  getRomNoFromEeprom();
  setupIO();  
  loadCurrentGame();
  oledSetup();
}

void loop() {
  
  checkButtons();
  oledShowTitle(titles[currentRomNo()]);
}

//----------------------------------------------------------------------------------------
// SETUP METHODS
//

// Used to time non-blocking delays
void setupTimeKeeping() {
  
  Wire.begin();
  Wire.setClock(400000L);
}

// Prepare input and output pins
void setupIO() {
  
  pinMode(PIN_DOWN,   INPUT_PULLUP);
  pinMode(PIN_UP,     INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);

  // Active HIGH reset - cancel any current reset
  pinMode(PIN_RESET,  OUTPUT);
  digitalWrite(PIN_RESET, LOW);
  
  // set ports 0 to 7 as OUTPUT
  DDRD = B11111111;
}

// load saved ROM no. from persistent memory
void getRomNoFromEeprom() {
  
  EEPROM.get(SAVED_GAME_EPROM_ADDR, romSave);
  //Sanity check       
  if(romSave > ROM_MAX || romSave < FIRST_ROM)
  {
    EEPROM.put(SAVED_GAME_EPROM_ADDR, FIRST_ROM);
    romSave = FIRST_ROM;
  }
  // update current rom
  if (romSave != currentRomNo()) {
    romNo = (romSave + FIRST_ROM);
  }
}

// Setup tiny screen and display welcome message
void oledSetup() {
  
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.set2X();
  oled.println("  VECTREX   ");
  oled.println(" DIY CART+  ");
  delay(WELCOME_MSG_SCREEN_TIME);
  oled.clear();
}

// Bankswitch in the current game
void loadCurrentGame() {
  
  PORTD = currentRomNo();
}
  
//----------------------------------------------------------------------------------------
// LOOP METHODS
//

// Long game titles can be displayed as a Ticker
void oledShowTitleAsTicker(String title) {

  if(title != lastTitle) {
    lastTitle = title;
    oled.clear();
    oled.tickerInit(&state, Adafruit5x7, 0, true, 0, 128);
  }
  
  if (tickTime > millis()) return; 
  tickTime = millis() + TICKER_TICK;

  // Handle ticker logic
  int8_t rtn = oled.tickerTick(&state);
  if (rtn <= RTN_CHECK) {
    oled.tickerText(&state, title);
  }
}

// Display short text and blank out trailing spaces
void oledShowShortTitle(String title) {
  
  oled.print(title);
  for(unsigned int i = title.length(); i < MAX_CHAR_2X; i++) {
    oled.print(" ");
  }
}

void oledShowTitle(String title) {
  
  // first row
  oled.setCursor(0 , 0);
  oled.set2X();

  if(title.length() <= MAX_CHAR_2X) {
    oledShowShortTitle(title);
  } else {
    oledShowTitleAsTicker(title);
  }
  oled.println();

  // blank second row - spacer
  oled.set1X();
  oled.println();  
  //third row
  oled.print("Down    Select     Up     ");
}

void oledShowGameSelected() {
  
  oled.setCursor(0,0);
  oled.set2X();
  oled.clear();
  oled.println ("  SELECTED  ");
  delay(SELECTED_MSG_SCREEN_TIME);
  oled.clear();
}

void resetVectrex() {
  
  digitalWrite(PIN_RESET , HIGH);
  delay (RESET_DELAY);
  digitalWrite(PIN_RESET , LOW);
}

void saveGame() {
  
  EEPROM.put(SAVED_GAME_EPROM_ADDR , currentRomNo());
  oled.setCursor(0,0);
  oled.set2X();
  oled.clear();
  oled.println ("   SAVED!  ");
  delay(SAVED_MSG_SCREEN_TIME);
  oled.clear();
}

void checkSelectButton() {
  
  selectButton = digitalRead(PIN_SELECT);
  if (selectButton != lastSelectState) {
    lastDebounceTimeSelect = millis();
  }
  
  if (((millis() - lastDebounceTimeSelect) > DEBOUNCE_DELAY) && selectButton != selectState) {

    selectState = selectButton;

    if(selectButton == LOW) {
      
        resetVectrex();
        loadCurrentGame();
        oledShowGameSelected();
        lastSelectTime = millis();
        
      } else if((millis() - lastSelectTime) > SAVE_BUTTON_DURATION_MIN) {
          
          saveGame();
      }
  }
  lastSelectState = selectButton;
}

void checkUpButton() {
  
  upButton = digitalRead(PIN_UP);
  if (upButton != lastUpState) {
    lastDebounceTimeUp = millis();
  }

  if (((millis() - lastDebounceTimeUp) > DEBOUNCE_DELAY) && upButton != upState) {

    upState = upButton;

    if(upButton == LOW) {
        romNo++;
        if (romNo > ROM_MAX) romNo = FIRST_ROM;
      } else {
        lastUpTime = millis();
      }
  }
  lastUpState = upButton;
}

void checkDownButton() {
  
  downButton = digitalRead(PIN_DOWN);
  if (downButton != lastDownState) {
    lastDebounceTimeDown = millis();
  }

  if (((millis() - lastDebounceTimeDown) > DEBOUNCE_DELAY) && downButton != downState) {

    downState = downButton;

    if(downButton == LOW) {
        romNo--;
    if (romNo < FIRST_ROM) romNo = ROM_MAX;
      } else {
        lastDownTime = millis();
      }
  }
  lastDownState = downButton;
}

void checkButtons() {

  checkSelectButton();
  checkUpButton();
  checkDownButton();
}

int currentRomNo() {
  
  return romNo - FIRST_ROM;
}
