Experimental Incubator
======================

## Project links:
- Wiki: http://wiki.techinc.nl/index.php/Fermentation_controller
- Food Hacking Base project page: http://foodhackingbase.org/wiki/Experimental_Incubator
- Code: https://github.com/foodhackingbase/incubator
- Programmers: larsm <post@larsm.org>, Marcel <bigmac@xs4all.nl>
- Code was used from: http://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_%28SKU:_DFR0009%29

## required libraries:
- OneWire: http://www.pjrc.com/teensy/td_libs_OneWire.html
- DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library
- Tiny RTC: http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
- Time: http://www.pjrc.com/teensy/td_libs_Time.html
- NEW LiquidCrystal_I2C: https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home

## Hardware:
- arduino uno + prototyping shield + DS18B20 temperature sensor + arduino keypad shield (6 keys and 16x2 lcd)

## Connecting everything:
- http://wiki.techinc.nl/index.php/Fermentation_controller#An_overview_of_pins_used

## display layout design (for now, simple mode only)
---
0123456789012345		0123456789012345
12:34:56  -12.34		12:34:56   55.67
+-* TARGET-15.00		+ *      HEAT ON
---

## TODO:
- write UI for real incubator programs (for example heat 3 days at 30C, then switch to cooling at 4C indefinite)
- write UI for preset programs (ferment tempeh; kimchi fridge; brew ginger beer etc)
- figure out what optional second temp sensor should change in logic
- take out float code from temp lib