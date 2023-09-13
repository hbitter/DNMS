# DNMS Firmware Teensy 4.0

# [English text below](#english text)

## Anmerkung zum selbst Übersetzen der DNMS Firmware 

### alle Versionen

Um die Firmware selbst zu übersetzen ist neben der Installation der Arduino IDE (Version 1.8.13 oder höher) und der Teensyduino Erweiterung (Version 1.53 oder höher) (PJRC Link: https://www.pjrc.com/teensy/teensyduino.html) noch folgendes zu beachten:
- Anstelle der Wire.h Bibliothek kommt für die I²C Kommunikation die Bibliothek teensy4_i2c zum Einsatz. Diese kann von GitHub herruntergeladen werden (https://github.com/Richard-Gemmell/teensy4_i2c ). Dann die teensy4_i2c Bibliothek in der Arduino IDE installieren. Unter Firmware/Teensy/Teensy4.0/ ist die benötigte Bibliothek als File teensy4_i2c-master.zip vorhanden. In der hier vorhandenen Version sind die nachfolgend beschriebenen Ergänzungen, um die Glitch-Filter für die I²C Kommunikation zu aktivieren, schon eingefügt.
- I²C Glitch-Filter aktivieren in der teensy4_i2c Lib -- dies ist unbedingt notwendig!  <br>
Bei der hier vorhandenen Version sind diese Änderungen schon eingefügt! <br> <br>
In der teensy4_i2c-master lib unter src->imx_rt1060 im File imx_rt1060_i2c_driver.cpp die Funktion IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) wie folgt zu ersetzen:
	
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
- In der Teensyduino Audio Bibliothek nun alle Verweise auf die Wire.h Bibliothek durch Verweise auf i2c_driver und i2c_driver_wire ersetzen d.h alle Vorkommnisse von "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen. Der Pfad zur Teensyduino Audio Bibliothek hängt vom Betriebssystem und der Installation ab, bei Windows z.B. unter C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux z.B unter /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/.
In der Audio Bibliothek sind einige Dateien mit dem Namen "control_xxx.cpp" vorhanden. In all diesen Dateien den Verweis "#include Wire.h" durch "#include <i2c_driver.h>" und "#include <i2c_driver_wire.h>" ersetzen.
- Beim Übersetzen der Versionen DNMS_V1.x.5 und DNMS_V2.x.5 ist es notwendig in der Arduino IDE unter Werkzeuge den USB Type von "Serial" auf "Audio" oder "Serial + MIDI + Audio" oder "Serial +MIDIx16 + Audio" zu setzen.
- 
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br>


### ab Version 2

- Den unter Firmware vorhanden Zip-File "dnms_audio_lib-master.zip" herunterladen, entpacken und in den Arduino Library Ordner als Lib einfügen.



------------------------------------------------------------------------


# <a name="english text"></a>DNMS Firmware Teensy 4.0

## Note how to compile the firmware yourself

### all versions

To compile the firmware by yourself you have to install the Arduino IDE (Version 1.8.13 and above) and the Teensyduino add on (Version 1.53 and above) (PJRC Link: https://www.pjrc.com/teensy/teensyduino.html). Besides that please see below:
- Instead of Wire.h lib the I²C lib teensy4_i2c is used. Download teensy4_i2c lib from GitHub (https://github.com/Richard-Gemmell/teensy4_i2c ) and install it in the Arduino IDE. Under Firmware/Teensy/Teensy4.0/ you will find the lib as file teensy4_i2c-master.zip. In this version here the necessary changes described later have been done to activate a glitch filter for I²C communication.
- Activation of I²C glitch filter in teensy4_i2c lib -- this is absolutely necessary! <br>
These changes are done in the version here! <br><br>
You have to change in the teensy4_i2c-master lib under src->imx_rt1060 in the file imx_rt1060_i2c_driver.cpp the function IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) as follows:
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

- In the Teensyduino Audio lib you have to change all "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>". The path to the Teensyduino Audio lib depends on your Operating System and your installation. On Windows e.g. C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio oder Linux e.g. /home/user/Arduino-SW/arduino-1.8.3/hardware/teensy/avr/libraries/Audio/. In that directory you will find several files named: control_xxx.cpp. In all those files change "#include Wire.h" to "#include <i2c_driver.h>" and "#include <i2c_driver_wire.h>".
- If you compile versions DNMS_V1.x.5 or DNMS_V2.x.5 you have to change the USB Type in the Arduino IDE under Tools from "Serial" to "Audio" or "Serial + MIDI + Audio" or "Serial +MIDIx16 + Audio".
<img src="images/Arduino IDE Teensy4.0 USB.jpg"><br> 


### from version 2 on

- In addition you have to download the zip-file "dnms_audio_lib-master.zip" from Firmware, unzip it and place it your Arduino library folder.

