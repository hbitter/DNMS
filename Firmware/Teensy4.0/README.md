# DNMS Firmware for Teensy 4.0

## English text below

## Anmerkung zum Download des Hex-Files

Auch um einen einzelnen Hex-File (z.B. DNMS_V1.1.0.ino.hex) herrunterzuladen ist es am einfachsten das gesamte Repository als ZIP herunterladen und dann zu entpacken.
<img src="images/DNMS_herrunterladen.jpg"><br>



## Anmerkung zum selbst Übersetzen der DNMS Firmware 

### Version 1 und 2

Um die Firmware selbst zu übersetzen ist neben der Installation der Arduino IDE (Version 1.8.13) und der Teensyduino Erweiterung (Version 1.53)(PJRC Link: https://www.pjrc.com/teensy/teensyduino.html) noch folgendes zu beachten:
- Anstelle der Wire.h Bibliothek kommt für die I²C Kommunikation die Bibliothek teensy4_i2c zum Einsatz. Kann von GitHub herruntergeladen werden (https://github.com/Richard-Gemmell/teensy4_i2c ). Dann die i2c_t3 Bibliothek in der Arduino IDE installieren. 
- In der Teensyduino Audio Bibliothek nun alle Verweise auf die Wire.h Bibliothek durch Verweise auf i2c_driver und i2c_driver_wire ersetzen d.h alle Vorkommnisse von "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen. Der Pfad zur Teensyduino Audio Bibliothek hängt vom Betriebssystem und der Installation ab, bei Windows z.B. unter C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux z.B unter /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/.
In der Audio Bibliothek sind einige Dateien mit dem Namen "control_xxx.cpp" vorhanden. In all diesen Dateien den Verweis "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen.
- Beim Übersetzen der Version DNMS_V1.0.5 ist es notwendig in der Arduino IDE unter Werkzeuge den USB Type von "Serial" auf "Audio" oder "Serial + MIDI + Audio" oder "Serial +MIDIx16 + Audio" zu setzen.
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br>


### nur Version 2

- Den unter Firmware vorhanden Zip-File "dnms_audio_lib-master.zip" herunterladen, entpacken und in den Arduino Library Ordner als Lib einfügen.


### nur Versionen 2.1.4 und 2.1.5

- Diese Versionen unterscheiden sich nur in der I²C Initialisierung des Teensy4 von den Versionen 2.0.4 und 2.0.5. Hintergrund sind die I²C Kommunikationsprobleme bei den Kombiboards (Teensy4 und NodeMCU auf einem Board), wenn parallel zum DNMS noch ein SPS30 Feinstaubsensor mit dem I²C Bus verbunden ist - bei den getrennten Boards giebt es diese Probleme nicht. Es wird für die beiden I²C Signale zusätzlich ein "Glitch"-Filter im Teensy4 aktiviert. 
Wenn man die Firmware selbst übersetzen möchte, ist in der teensy4_i2c-master lib unter src->imx_rt1060 im File imx_rt1060_i2c_driver.cpp die Funktion IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) wie folgt zu ersetzen:

```
void IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) {
    // Make sure slave mode is disabled before configuring it.
    stop_listening();

    // Halt and reset Slave Mode if it's running
    port->SCR = (LPI2C_SCR_RST | LPI2C_SCR_RRF | LPI2C_SCR_RTF);
    port->SCR = 0;

    initialise_common(config, pad_control_config);

    // Set the Slave Address
    port->SAMR = samr;

    // Enable clock stretching
    port->SCFGR1 = (address_config | LPI2C_SCFGR1_TXDSTALL | LPI2C_SCFGR1_RXSTALL);
    port->SCFGR2 = (LPI2C_SCFGR2_FILTSDA(15) | LPI2C_SCFGR2_FILTSCL(15));

    // Set up interrupts
    attachInterruptVector(config.irq, isr);
    port->SIER = (LPI2C_SIER_RSIE | LPI2C_SIER_SDIE | LPI2C_SIER_TDIE | LPI2C_SIER_RDIE);
    NVIC_ENABLE_IRQ(config.irq);

    // Enable Slave Mode
    port->SCR = LPI2C_SCR_SEN | LPI2C_SCR_FILTEN;
}

```
------------------------------------------------------------------------
## Note how to download the hex file

Even if you want to download only a single Hex-File (i.e. DNMS_V1.1.0.ini.hex), the easiest way is to download the complete repository as a ZIP and then unpack it.
<img src="images/DNMS_herrunterladen.jpg"><br>



## Note how to compile the firmware by yourself

### version 1 and 2

To compile the firmware by yourself you have to install the Arduino IDE (Version 1.8.13) and the Teensyduino add on (Version 1.53) (PJRC Link: https://www.pjrc.com/teensy/teensyduino.html). Besides that please see below:
- Instead of Wire.h lib the I²C lib teensy4_i2c is used. Download teensy4_i2c lib from GitHub (https://github.com/Richard-Gemmell/teensy4_i2c ) and install it in the Arduino IDE.
- In the Teensyduino Audio lib you have to change all "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>". The path to the Teensyduino Audio lib depends on your Operating System and your installation. On Windows e.g. C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux e.g. /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/. In that directory you will find several files named: control_xxx.cpp. In all those files change "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>".
- If you compile the version DNMS_V1.0.5 you have to change the USB Type in the Arduino IDE under Tools from "Serial" to "Audio" or "Serial + MIDI + Audio" or "Serial +MIDIx16 + Audio".
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br> 


### only version 2

- In addition you have to download the zip-file "dnms_audio_lib-master.zip" from Firmware, unzip it and place it your Arduino library folder.


### only versions 2.1.4 and 2.1.5

- These versions differ only in I²C initialising of Teensy4 from the versions 2.0.4 and 2.0.5. Background are I²C communication problems of the combined boards (Teensy4 and NodeMCU on one board), if there is a SPS30 PM sensor in parallel connected to the I²C bus - using separated boards, there is no communication problem. For both I²C signals a "glitch" filter on the Teensy4 will be activated.
If you want to compile the firmware yourself, you have to change in the teensy4_i2c-master lib under src->imx_rt1060 in the file imx_rt1060_i2c_driver.cpp the function IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) as follows:

```
void IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) {
    // Make sure slave mode is disabled before configuring it.
    stop_listening();

    // Halt and reset Slave Mode if it's running
    port->SCR = (LPI2C_SCR_RST | LPI2C_SCR_RRF | LPI2C_SCR_RTF);
    port->SCR = 0;

    initialise_common(config, pad_control_config);

    // Set the Slave Address
    port->SAMR = samr;

    // Enable clock stretching
    port->SCFGR1 = (address_config | LPI2C_SCFGR1_TXDSTALL | LPI2C_SCFGR1_RXSTALL);
    port->SCFGR2 = (LPI2C_SCFGR2_FILTSDA(15) | LPI2C_SCFGR2_FILTSCL(15));

    // Set up interrupts
    attachInterruptVector(config.irq, isr);
    port->SIER = (LPI2C_SIER_RSIE | LPI2C_SIER_SDIE | LPI2C_SIER_TDIE | LPI2C_SIER_RDIE);
    NVIC_ENABLE_IRQ(config.irq);

    // Enable Slave Mode
    port->SCR = LPI2C_SCR_SEN | LPI2C_SCR_FILTEN;
}

```
