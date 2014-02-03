// Experimental Incubator
//
// Wiki: http://wiki.techinc.nl/index.php/Fermentation_controller
// Food Hacking Base project page: http://foodhackingbase.org/wiki/Experimental_Incubator
// Code: https://github.com/foodhackingbase/incubator
// Programmers: larsm (post@larsm.org), Marcel, 
//
// Code was used from: http://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_%28SKU:_DFR0009%29, http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
//
// required libraries:
// OneWire: http://www.pjrc.com/teensy/td_libs_OneWire.html
// DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library
// Tiny RTC: http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
// Time: http://www.pjrc.com/teensy/td_libs_Time.html
//
// Hardware: arduino uno + prototyping shield + DS18B20 temperature sensor + arduino keypad shield (6 keys and 16x2 lcd)
// Connecting everything: http://wiki.techinc.nl/index.php/Fermentation_controller#An_overview_of_pins_used

// libraries
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>


// DS18B20 temperature sensor
#define ONE_WIRE_PIN 11 					// the pin where the temperature sensor is connected
OneWire ourWire(ONE_WIRE_PIN); 				// init oneWire instance
DallasTemperature sensors(&ourWire); 		// init Dallas Temperature Library

// LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 		// the pin where the display is connected
// define some values used by the LCD panel and keys
int lcd_key     = 0;						// temp variable
int adc_key_in  = 0;						// the pin where the keys are connected
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the keys
int read_LCD_keys()
{
	adc_key_in = analogRead(0);             // read the value from the sensor
	if (adc_key_in > 1000) return btnNONE;  // We make this the 1st option for speed reasons since it will be the most likely result
	// For V1.1 us this threshold
	// if (adc_key_in < 50)   return btnRIGHT;
	// if (adc_key_in < 250)  return btnUP;
	// if (adc_key_in < 450)  return btnDOWN;
	// if (adc_key_in < 650)  return btnLEFT;
	// if (adc_key_in < 850)  return btnSELECT;

	// For V1.0 comment the other threshold and use the one below:
	if (adc_key_in < 50)   return btnRIGHT;
	if (adc_key_in < 195)  return btnUP;
	if (adc_key_in < 380)  return btnDOWN;
	if (adc_key_in < 555)  return btnLEFT;
	if (adc_key_in < 790)  return btnSELECT;

	return btnNONE;  						// when all others fail, return this...
}

void setup() // is executed once at the start
{
	sensors.begin();                        // init temperature sensor

	lcd.begin(16, 2);                       // init lcd display

	tmElements_t tm;						// init Tiny RTC
	if (RTC.chipPresent() && !RTC.read(tm)) {
		RTC.set(1294911050); 				// chip found, set time?
	}
}

void print2digits(int number) {				// helper function to display 2 digits with trailing 0. larsm: why not use printf("%02d",number);?
	if (number >= 0 && number < 10) {
		lcd.print('0');
	}
	lcd.print(number);
}

void loop() 								// is executed in a loop
{
	tmElements_t tm;						// create Tiny RTC object
	char t[7];								// make char array. larsm: why?
	int i;									// make variable. larsm: why?
	
	lcd.setCursor(11,1);                    // setCursor(x position(0-15), line(0-1))
	sensors.requestTemperatures();          // request temperature
	lcd.print(sensors.getTempCByIndex(0) ); // display temperature
	
	lcd.setCursor(8,0);                     // setCursor(x position(0-15), line(0-1))
	
	if (RTC.read(tm)) {						// read and display Tiny RCT time
		print2digits(tm.Hour);
		lcd.print(':');
		print2digits(tm.Minute);
		lcd.print(':');
		print2digits(tm.Second);
	} else {								// Tiny RCT not sending time
		if (RTC.chipPresent()) {			// Tiny RCT found
			lcd.print("stopped");
		} else {							// Tiny RCT not found
			lcd.print("error  ");
		}
		delay(3000);						// display error message for 3 seconds
	}
	
	lcd.setCursor(0,1);                     // setCursor(x position(0-15), line(0-1))
	lcd_key = read_LCD_keys();           	// read the keys

	switch (lcd_key)                        // depending on which key was pushed, we perform an action
	{
	case btnRIGHT:
		{
			lcd.print("RIGHT ");
			break;
		}
	case btnLEFT:
		{
			lcd.print("LEFT  ");
			break;
		}
	case btnUP:
		{
			lcd.print("UP    ");
			break;
		}
	case btnDOWN:
		{
			lcd.print("DOWN  ");
			break;
		}
	case btnSELECT:
		{
			lcd.print("SELECT");
			break;
		}
	case btnNONE:
		{
			lcd.print("      ");
			break;
		}
	}
}
