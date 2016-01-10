// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <OneWire.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <DS1307RTC.h>
#include <Time.h>

// DS18B20 temperature sensor
#define ONE_WIRE_PIN 11					// the pin where the temperature sensor is connected
OneWire ourWire(ONE_WIRE_PIN);			// init oneWire instance
DallasTemperature sensors(&ourWire);	// init Dallas Temperature Library

#define OUT1	2						// switch on the element
#define OUT2	3						// change element function

RTC_DS1307 rtc;
// for PCF8574A based I2C displays
// LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
// for PCF8574T based I2C displays
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3 , POSITIVE);

int the_out;

void setup () {
	Serial.begin(115200);
	lcd.begin(16,2);               // initialize the lcd 
	lcd.backlight();
	lcd.setBacklight( 128 );
	the_out= 2;
	pinMode( 2, OUTPUT );				// heat
	pinMode( 3, OUTPUT );				// cool
	pinMode( 4, OUTPUT );				// heat
	pinMode( 5, OUTPUT );				// cool
	digitalWrite( 2, HIGH );
	digitalWrite( 3, HIGH );
	digitalWrite( 4, HIGH );
	digitalWrite( 5, HIGH );
  
#ifdef AVR
	Wire.begin();
#else
	Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
	rtc.begin();

	if (! rtc.isrunning()) {
		Serial.println("RTC is NOT running!");
		// following line sets the RTC to the date & time this sketch was compiled
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		// This line sets the RTC with an explicit date & time, for example to set
		// January 21, 2014 at 3am you would call:
		// rtc.adjust(DateTime(2015, 8, 6, 0, 0, 0));
	}
	sensors.begin();						// init Dallas Temperature library
}

#define btnLLEFT  1
#define btnLEFT   2
#define btnUP     3
#define btnDOWN   4
#define btnRIGHT  5
#define btnRRIGHT 6
#define btnNONE   0

// read the keys
int read_LCD_keys()
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

void loop () {
    DateTime now = rtc.now();
    // int a = analogRead(0);
    int a = read_LCD_keys();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
    
    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now.unixtime() + 7 * 86400L + 30);
    
    Serial.print(" now + 7d + 30s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();
    
    Serial.println();
    lcd.home ();
    lcd.print(now.year(), DEC);
    lcd.print("-");
    lcd.print(now.month(), DEC);
    lcd.print("-");
    lcd.print(now.day(), DEC);
    lcd.setCursor(0,1);
    lcd.print(now.hour(), DEC);
    lcd.print(":");
    print2digits(now.minute(),'0');
    lcd.print(":");
    print2digits(now.second(),'0');
    lcd.print(" ");
	if(a!=btnNONE)
	   lcd.print(a, DEC);
	else
	    lcd.print(" ");

	digitalWrite( the_out, HIGH );
	the_out++;
	if( the_out>5)
		the_out=2;
	digitalWrite( the_out, LOW );

	int iTemp;
	float fTemp;

	sensors.requestTemperatures();				// request temperature

	int ii;
	for( ii=0; ii<2; ii++)
	{
		fTemp= sensors.getTempCByIndex( ii );		// get temperature
		iTemp= (int) (fTemp*100);					// convert float to int
		lcd.setCursor( 10, ii );
		print_temp( iTemp );					// display temperature
	}
    delay(500);
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
	if( t==-12700 )
		lcd.print( " XXXXX");
	else
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
}
