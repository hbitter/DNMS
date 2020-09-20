# DNMS Firmware for Teensy 4.0


## Anmerkung zum Download des Hex-Files

Auch um einen einzelnen Hex-File (z.B. DNMS_V1.1.0.ino.hex) herrunterzuladen ist es am einfachsten das gesamte Repository als ZIP herunterladen und dann zu entpacken.
<img src="images/DNMS_herrunterladen.jpg"><br>



## Anmerkung zum selbst Übersetzen der DNMS Firmware

Um die Firmware selbst zu übersetzen ist neben der Installation der Arduino IDE (Version 1.8.13) und der Teensyduino Erweiterung (Version 1.53)(PJRC Link: https://www.pjrc.com/teensy/teensyduino.html) noch folgendes zu beachten:
- Anstelle der Wire.h Bibliothek kommt für die I²C Kommunikation die Bibliothek teensy4_i2c zum Einsatz. Kann von GitHub herruntergeladen werden (https://github.com/Richard-Gemmell/teensy4_i2c ). Dann die i2c_t3 Bibliothek in der Arduino IDE installieren. 
- In der Teensyduino Audio Bibliothek nun alle Verweise auf die Wire.h Bibliothek durch Verweise auf i2c_driver und i2c_driver_wire ersetzen d.h alle Vorkommnisse von "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen. Der Pfad zur Teensyduino Audio Bibliothek hängt vom Betriebssystem und der Installation ab, bei Windows z.B. unter C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux z.B unter /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/.
In der Audio Bibliothek sind einige Dateien mit dem Namen "control_xxx.cpp" vorhanden. In all diesen Dateien den Verweis "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen.
- Beim Übersetzen der Version DNMS_V1.0.5 ist es notwendig in der Arduino IDE unter Werkzeuge den USB Type von "Serial" auf "Audio" oder "Serial + MIDI + Audio" oder "Serial +MIDIx16 + Audio" zu setzen.
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br>


------------------------------------------------------------------------
## Note how to download the hex file

Even if you want to download only a single Hex-File (i.e. DNMS_V1.1.0.ini.hex), the easiest way is to download the complete repository as a ZIP and then unpack it.
<img src="images/DNMS_herrunterladen.jpg"><br>



## Note how to compile the firmware by yourself

To compile the firmware by yourself you have to install the Arduino IDE (Version 1.8.13) and the Teensyduino add on (Version 1.53) (PJRC Link: https://www.pjrc.com/teensy/teensyduino.html). Besides that please see below:
- Instead of Wire.h lib the I²C lib teensy4_i2c is used. Download teensy4_i2c lib from GitHub (https://github.com/Richard-Gemmell/teensy4_i2c ) and install it in the Arduino IDE.
- In the Teensyduino Audio lib you have to change all "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>". The path to the Teensyduino Audio lib depends on your Operating System and your installation. On Windows e.g. C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux e.g. /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/. In that directory you will find several files named: control_xxx.cpp. In all those files change "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>".
- If you compile the version DNMS_V1.0.5 you have to change the USB Type in the Arduino IDE under Tools from "Serial" to "Audio" or "Serial + MIDI + Audio" or "Serial +MIDIx16 + Audio".
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br> 

