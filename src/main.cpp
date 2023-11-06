/*
 Name:		Sketch1.ino
 Created:	24.01.2021 09:44:12
 Author:	Andi
*/
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include "main.h"

RotaryEncoder encoder(2, 3);
LiquidCrystal_I2C lcd(0x27, 16, 4);

uint8_t arrowup[8] = { // Pfeil nach oben
	B00100,
	B01110,
	B10101,
	B00100,
	B00100,
	B00100,
	B00100,
	B00100};

uint8_t arrowdn[8] = { // Pfeil nach unten
	B00100,
	B00100,
	B00100,
	B00100,
	B00100,
	B10101,
	B01110,
	B00100};

bool buttonPressed = false;
bool buttonState = true;
bool buttonReading = true;
bool lastButtonState = true;
bool rotaryTurned = false;
bool modeActive = false;
bool clockSet = false;
bool parameterChange = false;
bool initMode = false;
bool newPressureSet = false;
bool toggler = false;

int encoderCount = 0;
unsigned long actualTime = 0;
unsigned long clockTime = 0;
unsigned long oldTime = 0;
byte newmode = 0;
double pressure = 0;
int pressureSet = 0;
int pressureupperLim = 0;
int pressurelowerLim = 0;
int pressureSetValue = 0;
int modifiedPressure = 0;
int modifiedPressureMax = 0;
int modifiedPressureMin = 0;
int pressureValue = 0;
int oldSetP = 100;
int oldP = 0;
int counter = 0;
short timeHandler = 0;
byte actualScreen = 99;
byte screenSelect = 0;
byte prog = 1;
byte mode = 0;

constexpr char modeLabels[MODE_MAX_INDEX + 1][21] = {
	"Constant            ",
	"Increasing          ",
	"Sine                ",
	"Square              ",
	"Random              ",
	"Step Up             ",
	"Sensor Cal          ",
};

void setup()
{
	Serial.begin(SERIALSPEED);
	randomSeed(analogRead(A0));
	pinMode(PUMP_PIN, OUTPUT);
	pinMode(ACTIVE_LED_PIN, OUTPUT);
	pinMode(AIR_RELEASE_PIN, OUTPUT);
	pinMode(ENABLE_PIN, INPUT_PULLUP);
	pinMode(SELECT_PIN, INPUT);

	attachInterrupt(digitalPinToInterrupt(SELECT_PIN), buttonPressedInterrupt, FALLING);
	attachInterrupt(digitalPinToInterrupt(2), rotaryTurnedInterrupt, CHANGE);
	attachInterrupt(digitalPinToInterrupt(3), rotaryTurnedInterrupt, CHANGE);

	lcd.init();
	lcd.backlight();
	lcd.createChar(0, arrowup);
	lcd.createChar(1, arrowdn);
	lcd.clear();

	changeScreen();
}

void loop()
{
	if (buttonPressed)
	{
		if (screenSelect == MENU_START_SCREEN)
		{
			screenSelect = MENU_SELECT_SCREEN;
			changeScreen();
			actualScreen = screenSelect;
			buttonPressed = false;
		}

		// wenn Modus aktiv und button pressed dann Modus verlassen und zurück zum select screen
		if ((modeActive == true) && (buttonPressed == true) && (parameterChange == false))
		{
			modeActive = false;
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			screenSelect = MENU_SELECT_SCREEN;
			changeScreen();
			actualScreen = screenSelect;
			updateSelectScreen();
			encoderCount = mode;
			encoder.setPosition(encoderCount);
			buttonPressed = false;
		}

		if ((modeActive == true) && (buttonPressed == true) && (parameterChange == true))
		{
			parameterChange = false;
			buttonPressed = false;
		}

		if ((actualScreen != screenSelect) && (modeActive == false) && (buttonPressed == true))
		{
			if ((screenSelect < MENU_MIN_INDEX) || (screenSelect > MENU_MAX_INDEX))
			{
				screenSelect = MENU_SELECT_SCREEN;
				digitalWrite(PUMP_PIN, LOW);
				digitalWrite(AIR_RELEASE_PIN, LOW);
				changeScreen();
				actualScreen = screenSelect;
				buttonPressed = false;
			}
		}

		//Eintritt in den operationellen Modus
		if (((modeActive == false) && (actualScreen = MENU_SELECT_SCREEN)) && ((digitalRead(ENABLE_PIN) == LOW) && (buttonPressed == true)))
		{
			modeActive = true;
			initMode = false;
			digitalWrite(ACTIVE_LED_PIN, HIGH);
			buttonPressed = false;
		}
		
		// Notaus aktiv
		if ((digitalRead(ENABLE_PIN) == HIGH) && (buttonPressed == true))
		{
			modeActive = false;
			lcd.setCursor(0, 1);
			lcd.print("                    ");
			lcd.setCursor(0, 1);
			lcd.print("Notaus aktiv");
			Serial.println("Notaus");
			buttonPressed = false;
		}

		// If the screen changes to modeSelectScreen call updateModeSelectScreen
		// to make sure the mode selection is immediately visible (otherwise it
		// would only show after the next rotaryTurned event).
		if (actualScreen == screenSelect)
		{
			updateSelectScreen();
		}

		buttonPressed = false;
	}

	if (modeActive == false)
	{
		digitalWrite(PUMP_PIN, LOW);
		digitalWrite(AIR_RELEASE_PIN, LOW);
		digitalWrite(ACTIVE_LED_PIN, LOW);
	}

	if (rotaryTurned && (modeActive == false))
	{
		// hier Verzweigung für verschiedene Anzeigen
		if (actualScreen == MENU_SELECT_SCREEN)
		{
			if (encoderCount < MODE_MIN_INDEX)
			{
				encoderCount = MODE_MIN_INDEX;
				encoder.setPosition(encoderCount);
			}
			else if (encoderCount > MODE_MAX_INDEX)
			{
				encoderCount = MODE_MAX_INDEX;
				encoder.setPosition(encoderCount);
			}
			if (mode != encoderCount)
			{
				mode = encoderCount;
			}
			updateSelectScreen();
		}

		if (actualScreen == MENU_MAIN_SCREEN)
		{
			if (encoderCount < 1)
			{
				encoderCount = 1;
				encoder.setPosition(encoderCount);
			}
			else if (encoderCount > 100)
			{
				encoderCount = 100;
				encoder.setPosition(encoderCount);
			}
		}

		rotaryTurned = false;
	}

	if (rotaryTurned && (modeActive == true))
	{
		parameterChange = true;
		rotaryTurned = false;
	}

	if ((modeActive == true) && (digitalRead(ENABLE_PIN) == true))
	{
		digitalWrite(PUMP_PIN, LOW);
		digitalWrite(AIR_RELEASE_PIN, LOW);
		modeActive = false;
		screenSelect = MENU_SELECT_SCREEN;
		changeScreen();
		actualScreen = screenSelect;
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print("Notaus aktiv");
	}

	// Eintritt in Messmodus ##############################################################################
	if ((modeActive == true) && (screenSelect != MENU_MAIN_SCREEN))
	{
		screenSelect = MENU_MAIN_SCREEN;
		changeScreen();
		LCD_Update_pressure(pressureValue);
		LCD_Update_pressureSet(pressureSet);
		actualScreen = screenSelect;
	}

	switch (mode)
	{
	case MODE_RANDOM:
		modeRandom();
		break;
	case MODE_CONST_OUT:
		modeConstant();
		break;
	case MODE_SINE:
		modeSine();
		break;
	case MODE_INC_OUT:
		modeIncrease();
		break;
	case MODE_SQUARE:
		modeSquare();
		break;
	case MODE_STEPUP:
		modeStepUp();
		break;
	default:
		// statements
		break;
	}
}

void changeScreen()
{
	lcd.clear();
	switch (screenSelect)
	{
	case MENU_START_SCREEN:
		displayStartScreen();
		break;
	case MENU_SELECT_SCREEN:
		displaySelectScreen();
		break;
	case MENU_MAIN_SCREEN:
		displayOperatingScreen();
		break;
	default:
		displayStartScreen();
		break;
	}
}

void displayStartScreen()
{
	lcd.setCursor(0, 0);
	lcd.print("###  Universal   ###");
	lcd.setCursor(0, 1);
	lcd.print("###  Controller  ###");
	lcd.setCursor(0, 2);
	lcd.print("#   Push Button    #");
	lcd.setCursor(0, 3);
	lcd.print("#   to continue    #");
}

void displaySelectScreen()
{
	lcd.setCursor(0, 0);
	lcd.print("Menu:");
	lcd.setCursor(0, 2);
	lcd.print("Turn knob to select ");
	lcd.setCursor(0, 3);
	lcd.print("and press to enter  ");
	delay(1000);
	lcd.setCursor(0, 2);
	lcd.print("                    ");
	lcd.setCursor(0, 3);
	lcd.print("                    ");
}

void displayOperatingScreen()
{
	lcd.setCursor(0, 0);
	lcd.print(modeLabels[mode]);
	lcd.setCursor(0, 1);
	lcd.print("Pressure:");
	lcd.setCursor(14, 1);
	lcd.print(" Bar");
	lcd.setCursor(0, 2);
	lcd.print("Set:");
	lcd.setCursor(14, 2);
	lcd.print(" Bar");
}

void updateSelectScreen()
{
	lcd.setCursor(0, 1);
	lcd.print(modeLabels[mode]);

	switch (mode)
	{
	case MODE_CONST_OUT:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_INC_OUT:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_SINE:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_SQUARE:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_RANDOM:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_STEPUP:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	case MODE_CAL:
		lcd.setCursor(0, 1);
		lcd.print("                    ");
		lcd.setCursor(0, 1);
		lcd.print(modeLabels[mode]);
		break;
	default:
		break;
	}
}

void LCD_Update_pressure(int pressure)
{
	lcd.setCursor(10, 1);
	if (pressure < 0)
	{
		return;
	}
	if (pressure < 1000)
	{
		lcd.print(' ');
	}
	if (pressure < 100)
	{
		lcd.print(' ');
	}
	if (pressure < 10)
	{
		lcd.print(' ');
	}
	lcd.print(pressure);
}

void LCD_Update_pressureSet(int pressureSet)
{
	lcd.setCursor(10, 2);
	if (pressureSet < 1000)
	{
		lcd.print(' ');
	}
	if (pressureSet < 100)
	{
		lcd.print(' ');
	}
	if (pressureSet < 10)
	{
		lcd.print(' ');
	}
	lcd.print(pressureSet);
}

void rotaryTurnedInterrupt()
{
	encoder.tick();
	int newPos = encoder.getPosition();
	if (encoderCount != newPos)
	{
		encoderCount = newPos;
		rotaryTurned = true;
	}
}

void buttonPressedInterrupt()
{
	while (buttonPressed == false)
	{
		int i = 0;
		while (i < 5000)
		{
			if (digitalRead(SELECT_PIN) == true)
			{
				i++;
			}
		}
		buttonPressed = true;
	}
}

void getPressureValue()
{
	pressure = 0;
	pressureValue = 0;
	for (int i = 0; i < 10; i++)
	{
		pressureValue = analogRead(PRESSUREIN_PIN);
		pressure = pressure + pressureValue;
	}
	pressureValue = pressure / 10;
	pressureValue = int(pressureValue);
	pressureValue = map(pressureValue, 30, 1023, 0, 300);
	if (oldP != pressureValue)
	{
		LCD_Update_pressure(pressureValue);
		oldP = pressureValue;
	}
}

void getPressureSetValue()
{
	pressureSet = 0;
	pressureSetValue = 0;
	for (int x = 0; x < 10; x++)
	{
		pressureSetValue = analogRead(SETPRESSURE_PIN);
		pressureSet = pressureSet + pressureSetValue;
	}
	pressureSet = pressureSet / 10;
	pressureSet = map(pressureSet, 0, 1023, 0, 300);
	pressureupperLim = pressureSet + 25;
	pressurelowerLim = pressureSet - 5;

	if (oldSetP != pressureSet)
	{
		LCD_Update_pressureSet(pressureSet);
		newPressureSet = true;
		oldSetP = pressureSet;
	}
}

void modeConstant()
{
	// Mode Constant output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_CONST_OUT) && (actualScreen == MENU_MAIN_SCREEN))
	{
		actualTime = millis(); 				// Überprüfung Druck alle 150 ms
		if ((actualTime - oldTime) > 150)
		{
			getPressureValue();	   			// Istwert
			getPressureSetValue(); 			// Sollwert
			oldTime = actualTime;
		}

		if (pressureValue > pressureupperLim)	//Druck zu hoch
		{ // Regelung
			digitalWrite(PUMP_PIN, LOW);		//Pumpe aus
			digitalWrite(AIR_RELEASE_PIN, LOW);	//Ablassventil öffnen
			lcd.setCursor(19, 1);
			lcd.write(1);						//arrow down
		}
		else if (pressureValue <= pressurelowerLim)//Druck zu niedrig
		{
			digitalWrite(PUMP_PIN, HIGH);		//Pumpe ein
			digitalWrite(AIR_RELEASE_PIN, HIGH);	//Ablassventil schließen
			lcd.setCursor(19, 1);
			lcd.write(0);						//arrow up
		}
		else if (pressureValue >= pressureSet)	//Druck erreicht oder halten
		{
			digitalWrite(PUMP_PIN, LOW);		//Pumpe aus
			digitalWrite(AIR_RELEASE_PIN, HIGH);	//Ablassventil schließen
			lcd.setCursor(19, 1);
			lcd.print('-');						//konstant
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
		}

		// Abstelltimer
		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			int remTime = 600 - timeHandler;
			lcd.setCursor(0, 3);
			if (remTime < 100)
			{
				lcd.print(' ');
			}
			if (remTime < 10)
			{
				lcd.print(' ');
			}
			lcd.print(remTime);
			clockSet = false;
		}

		if (timeHandler >= 600)
		{
			modeActive = false;
			timeHandler = 0;
			screenSelect = MENU_SELECT_SCREEN;
			changeScreen();
			actualScreen = screenSelect;
			updateSelectScreen();
		}
	}
}

void modeRandom()
{
	// Mode Random output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_RANDOM) && (actualScreen == MENU_MAIN_SCREEN))
	{
		actualTime = millis();

		if (((actualTime - oldTime) > 150) || (initMode == false))
		{
			// Serial.println(initMode);
			getPressureValue();	   // Istwert
			getPressureSetValue(); // Sollwert
			oldTime = actualTime;
		}

		if ((timeHandler >= 30) || (initMode == false))
		{
			// getPressureSetValue(1);
			modifiedPressure = random(pressureSet);
			modifiedPressureMax = modifiedPressure + 25;
			modifiedPressureMin = modifiedPressure - 5;
			timeHandler = 0;
			lcd.setCursor(10, 3);
			if (modifiedPressure < 1000)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 100)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 10)
			{
				lcd.print(' ');
			}
			lcd.print(modifiedPressure);
			if (initMode == false)
			{
				initMode = true;
			}
		}

		// Serial.println(randomPressure);

		// Regelung ##################################

		if (pressureValue > modifiedPressureMax)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			lcd.setCursor(19, 1);
			lcd.write(1);
			// Serial.println("F1");
		}
		else if (pressureValue <= modifiedPressureMin)
		{
			digitalWrite(PUMP_PIN, HIGH);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.write(0);
			// Serial.println("F2");
		}
		else if (pressureValue >= modifiedPressure)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.print('-');
			// Serial.println("F3");
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			// Serial.println("F4");
		}

		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			int remainingTime = 30 - timeHandler;
			lcd.setCursor(0, 3);
			if (remainingTime < 100)
			{
				lcd.print(' ');
			}
			if (remainingTime < 10)
			{
				lcd.print(' ');
			}
			lcd.print(remainingTime);
			clockSet = false;
		}
	}
}

void modeIncrease()
{
	// Mode Increasing output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_INC_OUT) && (actualScreen == MENU_MAIN_SCREEN))
	{
		if (initMode == false)
		{
			counter = 1;
		}

		actualTime = millis();
		if (((actualTime - oldTime) > 200) || (initMode == false))
		{
			getPressureValue();	   // Istwert
			getPressureSetValue(); // Sollwert
			oldTime = actualTime;
		}

		if ((timeHandler >= INTERVALL_TIME) || (initMode == false) || (newPressureSet == true))
		{
			if (newPressureSet != true)
			{
				counter++;
			};
			timeHandler = 0;
			float pCalc = (pressureSet / 10) * counter;
			modifiedPressure = int(pCalc);
			modifiedPressureMax = modifiedPressure + 25;
			modifiedPressureMin = modifiedPressure - 5;
			// Serial.print(counter);
			// Serial.println(modifiedPressure);
			newPressureSet = false;
			lcd.setCursor(10, 3);
			if (modifiedPressure < 1000)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 100)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 10)
			{
				lcd.print(' ');
			}
			lcd.print(modifiedPressure);
			if (initMode == false)
			{
				initMode = true;
			}
		}

		if (counter > 10)
		{
			counter = 0;
		}

		// Regelung ##################################
		if (pressureValue > pressureupperLim)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			lcd.setCursor(19, 1);
			lcd.write(1);
		}
		else if (pressureValue <= pressurelowerLim)
		{
			digitalWrite(PUMP_PIN, HIGH);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.write(0);
		}
		else if (pressureValue >= pressureSet)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.print('-');
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
		}

		// Erhöhungsintervall
		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			int remTime = INTERVALL_TIME - timeHandler;
			lcd.setCursor(0, 3);
			if (remTime < 100)
			{
				lcd.print(' ');
			}
			if (remTime < 10)
			{
				lcd.print(' ');
			}
			lcd.print(remTime);
			clockSet = false;
		}
	}
}

void modeSine()
{
	// Mode Sine output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_SINE) && (actualScreen == MENU_MAIN_SCREEN))
	{
		// Initialisierung bei Eintritt
		if (initMode == false)
		{
			counter = 270;	 // initialise counter
			initMode = true; // reset InitMode
		}

		actualTime = millis();
		if ((actualTime - oldTime) > 150)
		{
			getPressureValue();	   // Istwert
			getPressureSetValue(); // Sollwert
			oldTime = actualTime;
		}

		if (counter > 630)
		{
			counter = 270;
		}

		if (timeHandler >= 1)
		{
			counter = counter + 20;
			// Serial.print(counter);
			modifiedPressure = (pressureSet * (1 + (sin((6.28 / 360) * counter))));
			// Serial.print("	");
			// Serial.println(randomPressure);
			modifiedPressure = modifiedPressure / 2;
			modifiedPressureMax = modifiedPressure + 25;
			modifiedPressureMin = modifiedPressure - 5;
			timeHandler = 0;
			lcd.setCursor(10, 3);
			if (modifiedPressure < 1000)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 100)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 10)
			{
				lcd.print(' ');
			}
			lcd.print(modifiedPressure);
		}

		if (pressureValue > modifiedPressureMax)
		{ // Regelung
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			lcd.setCursor(19, 1);
			lcd.write(1);
			// Serial.println("F1");
		}
		else if (pressureValue <= modifiedPressureMin)
		{
			digitalWrite(PUMP_PIN, HIGH);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.write(0);
			// Serial.println("F2");
		}
		else if (pressureValue >= modifiedPressure)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.print('-');
			// Serial.println("F3");
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			// Serial.println("F4");
		}

		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			clockSet = false;
		}
	}
}

void modeSquare()
{
	// Mode Square output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_SQUARE) && (actualScreen == MENU_MAIN_SCREEN))
	{
		actualTime = millis();

		if (((actualTime - oldTime) > 150) || (initMode == false))
		{
			// Serial.println(initMode);
			getPressureValue();	   // Istwert
			getPressureSetValue(); // Sollwert
			oldTime = actualTime;
		}

		if ((timeHandler >= 15) || (initMode == false))
		{
			// getPressureSetValue(1);

			modifiedPressure = pressureSet * toggler;
			modifiedPressureMax = modifiedPressure + 25;
			modifiedPressureMin = modifiedPressure - 5;
			timeHandler = 0;
			toggler = !toggler;
			lcd.setCursor(10, 3);
			if (modifiedPressure < 1000)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 100)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 10)
			{
				lcd.print(' ');
			}
			lcd.print(modifiedPressure);
			if (initMode == false)
			{
				initMode = true;
			}
		}

		// Serial.println(randomPressure);

		// Regelung ##################################

		if (pressureValue > modifiedPressureMax)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			lcd.setCursor(19, 1);
			lcd.write(1);
			// Serial.println("F1");
		}
		else if (pressureValue <= modifiedPressureMin)
		{
			digitalWrite(PUMP_PIN, HIGH);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.write(0);
			// Serial.println("F2");
		}
		else if (pressureValue >= modifiedPressure)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.print('-');
			// Serial.println("F3");
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			// Serial.println("F4");
		}

		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			int remainingTime = 15 - timeHandler;
			lcd.setCursor(0, 3);
			if (remainingTime < 100)
			{
				lcd.print(' ');
			}
			if (remainingTime < 10)
			{
				lcd.print(' ');
			}
			lcd.print(remainingTime);
			clockSet = false;
		}
	}
}

void modeStepUp()
{
	// Mode Step Up output aktiv #########################################################################
	if ((modeActive == true) && (mode == MODE_STEPUP) && (actualScreen == MENU_MAIN_SCREEN))
	{
		if (initMode == false)
		{
			counter = 1;
		}

		actualTime = millis();
		if (((actualTime - oldTime) > 200) || (initMode == false))
		{
			getPressureValue();	   // Istwert
			getPressureSetValue(); // Sollwert
			oldTime = actualTime;
		}

		if ((timeHandler >= 15) || (initMode == false) || (newPressureSet == true))
		{
			if ((newPressureSet != true) && (toggler == false))
			{
				counter++;
			};
			timeHandler = 0;
			toggler = !toggler;
			float pCalc = ((pressureSet / 10) * counter) * toggler;
			modifiedPressure = int(pCalc);
			modifiedPressureMax = modifiedPressure + 15;
			modifiedPressureMin = modifiedPressure - 5;
			// Serial.print(counter);
			// Serial.println(modifiedPressure);
			newPressureSet = false;
			lcd.setCursor(10, 3);
			if (modifiedPressure < 1000)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 100)
			{
				lcd.print(' ');
			}
			if (modifiedPressure < 10)
			{
				lcd.print(' ');
			}
			lcd.print(modifiedPressure);
			if (initMode == false)
			{
				initMode = true;
			}
		}

		if (counter >= 10)
		{
			counter = 0;
		}

		// Regelung ##################################
		if (pressureValue > modifiedPressureMax)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, LOW);
			lcd.setCursor(19, 1);
			lcd.write(1);
		}
		else if (pressureValue <= modifiedPressureMin)
		{
			digitalWrite(PUMP_PIN, HIGH);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.write(0);
		}
		else if (pressureValue >= modifiedPressure)
		{
			digitalWrite(PUMP_PIN, LOW);
			digitalWrite(AIR_RELEASE_PIN, HIGH);
			lcd.setCursor(19, 1);
			lcd.print('-');
		}
		else
		{
			digitalWrite(AIR_RELEASE_PIN, HIGH);
		}

		// Erhöhungsintervall
		if (clockSet == false)
		{
			clockTime = millis();
			clockSet = true;
		}

		if ((millis() - clockTime) > 1000)
		{
			timeHandler++;
			Serial.println(timeHandler);
			int remTime = 15 - timeHandler;
			lcd.setCursor(0, 3);
			if (remTime < 100)
			{
				lcd.print(' ');
			}
			if (remTime < 10)
			{
				lcd.print(' ');
			}
			lcd.print(remTime);
			clockSet = false;
		}
	}
}
