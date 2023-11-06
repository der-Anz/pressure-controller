#pragma once

constexpr auto SERIALSPEED = 115200;

// assign arduino pins
// Interruptpins of MEGA2560 2, 3, 18, 19, 20, 21
constexpr auto SELECT_PIN = 18;       // Button of Rotary
constexpr auto PRESSUREIN_PIN = A8;   // 0 - 5 V vom Drucksensor
constexpr auto SETPRESSURE_PIN = A10; // Poti 0 - 5 V
constexpr auto PUMP_PIN = 8;          // Kompressor Relais
constexpr auto ENABLE_PIN = 52;       // Notaus (öffner)
constexpr auto AIR_RELEASE_PIN = 53;  // Ablassventil
constexpr auto ACTIVE_LED_PIN = 13;
constexpr auto INTERVALL_TIME = 300;

// menu screens
constexpr auto MENU_MIN_INDEX = 0;
constexpr auto MENU_START_SCREEN = 0;
constexpr auto MENU_SELECT_SCREEN = 1;
constexpr auto MENU_MAIN_SCREEN = 2;
constexpr auto MENU_MAX_INDEX = 2;

// mode select
constexpr auto MODE_MIN_INDEX = 0;
constexpr auto MODE_CONST_OUT = 0;
constexpr auto MODE_INC_OUT = 1;
constexpr auto MODE_SINE = 2;
constexpr auto MODE_SQUARE = 3;
constexpr auto MODE_RANDOM = 4;
constexpr auto MODE_STEPUP = 5;
constexpr auto MODE_CAL = 6;
constexpr auto MODE_MAX_INDEX = 6;

// flags
extern bool buttonPressed;
extern bool buttonState;
extern bool buttonReading;
extern bool lastButtonState;
extern bool rotaryTurned;
extern bool modeActive;
extern bool clockSet;
extern bool parameterChange;
extern bool initMode;
extern bool newPressureSet;
extern bool toggler;

extern int encoderCount;
extern unsigned long actualTime;
extern unsigned long clockTime;
extern unsigned long oldTime;
extern byte newmode;
extern double pressure;
extern int pressureSet;
extern int pressureupperLim;
extern int pressurelowerLim;
extern int pressureSetValue;
extern int modifiedPressure;
extern int modifiedPressureMax;
extern int modifiedPressureMin;
extern int pressureValue;
extern int oldSetP;
extern int oldP;
extern int counter;
extern short timeHandler;
extern byte actualScreen;
extern byte screenSelect;
extern byte prog;
extern byte mode;

void modeConstant();
void modeRandom();
void modeIncrease();
void modeSine();
void modeSquare();
void modeStepUp();

void changeScreen();
void displayStartScreen();
void displaySelectScreen();
void displayOperatingScreen();
void updateSelectScreen();
void LCD_Update_pressure(int pressure);
void LCD_Update_pressureSet(int pressureSet);
void rotaryTurnedInterrupt();
void buttonPressedInterrupt();
void getPressureValue();
void getPressureSetValue();