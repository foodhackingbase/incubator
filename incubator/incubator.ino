// Experimental Incubator
//
// Wiki: http://wiki.techinc.nl/index.php/Fermentation_controller
// Food Hacking Base project page: http://foodhackingbase.org/wiki/Experimental_Incubator
// Code: https://github.com/foodhackingbase/incubator
// Programmers: larsm <post@larsm.org>, Marcel <bigmac@xs4all.nl> 
//
// Code was used from: http://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_%28SKU:_DFR0009%29
//
// required libraries:
// OneWire: http://www.pjrc.com/teensy/td_libs_OneWire.html
// DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library
// Tiny RTC: http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
// Time: http://www.pjrc.com/teensy/td_libs_Time.html
// NEW LiquidCrystal_I2C: https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
//
// Hardware: arduino uno + prototyping shield + DS18B20 temperature sensor + arduino keypad shield (6 keys and 16x2 lcd)
// Connecting everything: http://wiki.techinc.nl/index.php/Fermentation_controller#An_overview_of_pins_used

// display layout design (for now, simple mode only)
// 0123456789012345		0123456789012345
// 12:34:56  -12.34		12:34:56   55.67
// +-* TARGET-15.00		+ *      HEAT ON

// TODO:
// - write UI for real incubator programs (for example heat 3 days at 30C, then switch to cooling at 4C indefinite)
// - write UI for preset programs (ferment tempeh; kimchi fridge; brew ginger beer etc)
// - figure out what optional second temp sensor should change in logic
// - take out float code from temp lib

// libraries
#include <LiquidCrystal_I2C.h>				// must be the new one for backlight control!!!
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <EEPROM.h>

// DS18B20 temperature sensor
#define ONE_WIRE_PIN 11					// the pin where the temperature sensor is connected
OneWire ourWire( ONE_WIRE_PIN );		// init oneWire instance
DallasTemperature sensors(&ourWire);	// init Dallas Temperature Library

#define OUT1		2					// switch on the heat function on digital output 2
#define OUT2		3					// switch on the cool function on digital output 3
#define FAN_CONTROL	5					// switch on the fans on digital output 13

#define MODE_OFF	0
#define MODE_FAN	1
#define MODE_HEAT	2
#define MODE_COOL	3
#define MODE_TEMP_H	4
#define MODE_TEMP_C	5
#define MODE_TEMP_T	6

#define MODE_MIN	MODE_OFF
#define MODE_MAX	MODE_TEMP_T

#define DEFAULT_TEMP	2500		// 25C = default choice for temp related programs
#define TEMP_INTERVAL	50			// +-0.5C hysteresis
#define TEMP_STEP		100			// button up and down changes target how much?
#define TIME_INTERVAL	5			// minimum on or off time (important for PC power supplies)
#define FAN_TIME		30			// how long to keep fans on after elements are turned off

// config magic and version to be sure is for our code, otherwise fall back to defaults
#define MAGIC1	0x0F	//oodhacking
#define MAGIC2	0x0B	//ase
#define VERSION	0x02	//v0.2

typedef struct {
	uint8_t uMagic1;
	uint8_t uMagic2;
	uint8_t uVersion;
	int8_t iMode;
	int iTargetTemp;
} Config;

Config cfg;

void read_config()
{
	int i;
	uint8_t* tp= (uint8_t*)&cfg;
	for( i= 0; i<sizeof( cfg ); i++ )
		*tp++= EEPROM.read( i );
	if( cfg.uMagic1!=MAGIC1 || cfg.uMagic2!=MAGIC2 || cfg.uVersion!=VERSION ) {
		// magic or version mismatch, reset to default values
		cfg.uMagic1= MAGIC1;
		cfg.uMagic2= MAGIC2;
		cfg.uVersion= VERSION;
		cfg.iMode= MODE_OFF;
		cfg.iTargetTemp= DEFAULT_TEMP;
	}	
}

void save_config()
{
	int i;
	uint8_t* tp= (uint8_t*)&cfg;
	for( i= 0; i<sizeof( cfg ); i++ )
		EEPROM.write( i, *tp++ );
}

// LCD panel
// for PCF8574A based I2C displays
// LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
// for PCF8574T based I2C displays
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3 , POSITIVE);
// define some values used by the LCD panel and Keys
#define btnLLEFT  1
#define btnLEFT   2
#define btnUP     3
#define btnDOWN   4
#define btnRIGHT  5
#define btnRRIGHT 6
#define btnNONE   0

// read the keys
int8_t read_LCD_keys()
{
	int adc_key_in;

	adc_key_in= analogRead( 0 );				// read the value from the sensor
	if( adc_key_in>1000 )
		return btnNONE;							// We make this the 1st option for speed reasons since it will be the most likely result
	// For V1.1 us this threshold
	// if (adc_key_in < 50)   return btnRIGHT;
	// if (adc_key_in < 250)  return btnUP;
	// if (adc_key_in < 450)  return btnDOWN;
	// if (adc_key_in < 650)  return btnLEFT;
	// if (adc_key_in < 850)  return btnSELECT;

	// For V1.0 comment the other threshold and use the one below:
	if( adc_key_in<50 )
		return btnLLEFT;
	if( adc_key_in<195 )
		return btnLEFT;
	if( adc_key_in<380 )
		return btnUP;
	if( adc_key_in<555 )
		return btnRIGHT;
	if( adc_key_in<790 )
		return btnDOWN;
	if( adc_key_in<950 )
		return btnRRIGHT;

	return btnNONE;							// when all others fail, return this...
}

void splash()
{
	lcd.setCursor( 0, 0 );
	lcd.print( "FoodHackingBase" );
	lcd.setCursor( 0, 1 );
	lcd.print( "Incubator V" );
	lcd.print( (char)('0'+(VERSION>>4)) );
	lcd.print( '.' );
	lcd.print( (char)('0'+(VERSION&0xF)) );
	delay( 1500 );
	lcd.clear();
}

void print2digits( short number, char fill ) {
	if( number>=0 && number<10 )
		lcd.print( fill );
	else
		lcd.print( (char)('0'+(char)(number/10)) );
	lcd.print( (char)('0'+(char)(number%10)) );
}

void print_temp( int t )
{
	if( t<0 ) {
		lcd.print( '-' );
		t= -t;
	}
	else
		lcd.print( ' ' );
	print2digits( t/100, ' ' );
	lcd.print( '.' );
	print2digits( t%100, '0' );
}

time_t tSample;
time_t tLast;
time_t tNow;
time_t tFan;

int8_t iLastKey= btnNONE;

boolean active_low= TRUE;

#define FAN_OFF		fan_onoff( LOW )
#define FAN_ON		fan_onoff( HIGH )

#define TURN_OFF	turn_onoff( LOW, LOW )
#define TURN_HEAT	turn_onoff( HIGH, LOW )
#define TURN_COOL	turn_onoff( LOW, HIGH )

boolean fan_on= FALSE;
void fan_onoff( uint8_t p )
{
	if( active_low ) {
		digitalWrite( FAN_CONTROL, p==HIGH?LOW:HIGH );
	} else {
		digitalWrite( FAN_CONTROL, p );
	}
	tFan= tNow;
	fan_on= (p==HIGH);
	lcd.setCursor( 2, 1 );
	lcd.print( fan_on?'*':' ' );
}

boolean need_fan= FALSE;
void turn_onoff( uint8_t p1, uint8_t p2 )
{
	if( (p1==HIGH) || (p2==HIGH) ) {
		FAN_ON;
		need_fan= TRUE;
		delay(1000);			// let's be nice to our PSU and not switch everything on at once
	} else
		need_fan= FALSE;

	if( active_low ) {
		digitalWrite( OUT1, p1==HIGH?LOW:HIGH );
		digitalWrite( OUT2, p2==HIGH?LOW:HIGH );
	} else {
		digitalWrite( OUT1, p1 );
		digitalWrite( OUT2, p2 );
	}

	lcd.setCursor( 0, 1 );
	lcd.print( p1==HIGH?'+':' ' );
	lcd.print( p2==HIGH?'-':' ' );
	tSample= tNow;
}

int8_t iDir= 0;			// if we can cool and heat, we need to know what we were doing
#define UP		1
#define DOWN	-1

void set_mode()
{
	switch( cfg.iMode )
	{
	case MODE_OFF:
		TURN_OFF;
		FAN_OFF;
		break;
	case MODE_FAN:
		TURN_OFF;
		FAN_ON;
		break;
	case MODE_HEAT:
		TURN_HEAT;
		break;
	case MODE_COOL:
		TURN_COOL;
		break;
	case MODE_TEMP_T:
		iDir= 0;
		break;
	default:
		// let our timer decide on action
		break;
	}
}

void print_mode( int8_t i )
{
	lcd.setCursor( 3, 1 );
	switch( i )
	{
	case MODE_OFF:
		lcd.print( "          OFF" );
		break;
	case MODE_FAN:
		lcd.print( "       FAN ON" );
		break;
	case MODE_HEAT:
		lcd.print( "      HEAT ON" );
		break;
	case MODE_COOL:
		lcd.print( "      COOL ON" );
		break;
	case MODE_TEMP_H:
		lcd.print( "HEAT TO" );
		goto more_temp;							// every C program should have a goto statement, just because you can!
	case MODE_TEMP_C:
		lcd.print( "COOL TO" );
		goto more_temp;
	case MODE_TEMP_T:
		lcd.print( " TARGET" );
	more_temp:
		print_temp( cfg.iTargetTemp );
		break;
	default:
		;
	}
}

void setup() // is executed once at the start
{
	tmElements_t tm;

	pinMode( OUT1, OUTPUT );				// heat
	pinMode( OUT2, OUTPUT );				// cool
	pinMode( FAN_CONTROL, OUTPUT );			// fan

	FAN_OFF;
	TURN_OFF;

	lcd.begin( 16, 2 );						// start the LCD library
	//lcd.setBacklightPin ( 10, POSITIVE );
	lcd.setBacklight( 128 );				// set the contrast to 50%
	splash();

	read_config();

	sensors.begin();						// init Dallas Temperature library

	if( RTC.chipPresent() ) {
		if( !RTC.read( tm ) ) {
			RTC.set( 0 );						// we have a chip but it needs kicking in the butt
			RTC.read( tm );
		}
		tFan= tLast= tSample= tNow= makeTime( tm );
	} else
		tFan= tLast= tSample= tNow= 0;

	set_mode();
	print_mode( cfg.iMode );
}

int iLastTemp= 0;
tmElements_t oldTm= {99,99,99,99,99,99,99};

void loop() // is executed in a loop
{
	tmElements_t tm;
	int8_t iNewKey;
	int iTemp;
	float fTemp;

	sensors.requestTemperatures();				// request temperature
	fTemp= sensors.getTempCByIndex( 0 );		// get temperature
	iTemp= (int) (fTemp*100);					// convert float to int
	if( iTemp!=iLastTemp ) {
		iLastTemp= iTemp;
		lcd.setCursor( 10, 0 );
		print_temp( iTemp );					// display temperature
	}

	if( RTC.read( tm ) ) {
		tNow= makeTime( tm );

		if( fan_on && !need_fan ) {				// check why that fan is still running
			if( tNow-tFan>=FAN_TIME && cfg.iMode!=MODE_FAN ) {
				FAN_OFF;
			}
		}

		if( tNow!=tLast ) {						// only update time on display if time has changed
			if( tm.Hour!=oldTm.Hour ) {
				lcd.setCursor( 0, 0 );
				print2digits( tm.Hour, '0' );
				lcd.print( ':' );
			}
			if( tm.Minute!=oldTm.Minute ) {
				lcd.setCursor( 3, 0 );
				print2digits( tm.Minute, '0' );
				lcd.print( ':' );
			}
			lcd.setCursor( 6, 0 );
			print2digits( tm.Second, '0' );
			tLast= tNow;
			oldTm= tm;
		}
	} else {									// time failed to read
		lcd.setCursor( 0, 0 );
		if( RTC.chipPresent() ) {
			lcd.print( "stopped " );
		} else {
			lcd.print( "error   " );
		}
		delay( 2000 );
		tLast= tNow= now();
	}

	iNewKey= read_LCD_keys();				// read the keys

	if( iNewKey!=iLastKey ) {
		iLastKey= iNewKey;
		switch( iNewKey )					// depending on which key was pushed, we perform an action
		{
		case btnRIGHT:
			// save the current config to eeprom
			save_config();
			break;
		case btnLEFT:
			cfg.iMode++;
			if( cfg.iMode>MODE_MAX )
				cfg.iMode= MODE_MIN;
			set_mode();
			print_mode( cfg.iMode );
			break;
		case btnUP:
			cfg.iTargetTemp+= TEMP_STEP;
			print_mode( cfg.iMode );
			break;
		case btnDOWN:
			cfg.iTargetTemp-= TEMP_STEP;
			print_mode( cfg.iMode );
			break;
		case btnLLEFT:
			RTC.set(0);						// reset time
			tNow= tLast= tSample= tFan= 0;
			break;
		case btnNONE:
			print_mode( MODE_MAX+1 );
			break;
		}
	}

	if( tNow-tSample>=TIME_INTERVAL ) {
		if( cfg.iMode==MODE_TEMP_H ) {
			if( iTemp<=cfg.iTargetTemp-TEMP_INTERVAL )
				TURN_HEAT;
			else if( iTemp>=cfg.iTargetTemp+TEMP_INTERVAL )
				TURN_OFF;
			else
				;
		} else if( cfg.iMode==MODE_TEMP_C ) {
			if( iTemp>=cfg.iTargetTemp+TEMP_INTERVAL )
				TURN_COOL;
			else if( iTemp<=cfg.iTargetTemp-TEMP_INTERVAL )
				TURN_OFF;
			else
				;
		} else if( cfg.iMode==MODE_TEMP_T ) {
			if( iDir!=DOWN ) {
				if( iTemp<=cfg.iTargetTemp-TEMP_INTERVAL ) {
					TURN_HEAT;
					iDir= UP;
				} else if( iTemp>=cfg.iTargetTemp+2*TEMP_INTERVAL ) {
					// too much overshoot, switch direction
					TURN_COOL;
					iDir= DOWN;
				} else if( iTemp>=cfg.iTargetTemp+TEMP_INTERVAL ) {
					TURN_OFF;
				} else
					;
			} else if( iDir!=UP ) {
				if( iTemp>=cfg.iTargetTemp+TEMP_INTERVAL ) {
					TURN_COOL;
					iDir= DOWN;
				} else if( iTemp<=cfg.iTargetTemp-2*TEMP_INTERVAL ) {
					// too much overshoot, switch direction
					TURN_HEAT;
					iDir= UP;
				} else if( iTemp<=cfg.iTargetTemp-TEMP_INTERVAL ) {
					TURN_OFF;
				} else
					;
			}
		}
	}
}
