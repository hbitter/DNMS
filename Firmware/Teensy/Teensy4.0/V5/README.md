# [English text below](#dnms-firmware-teensy4.0-v5-english)

# DNMS Firmware Teensy4.0 V5

## V5 realisiert neben der Funktionalität von V4 zusätzlich die Berechnung und Ausgabe von Z-Werten

### aktuelle Versionen:

- DNMS_V5.0.4 - für das ICS-43434 Mikrofon
- DNMS_V5.0.6 - für das IM72D128 Mikrofon

Bitte beachten: Werden nicht zusammengehörige Versionen von Mikrofon und Teensy Firmware benutzt, so ergeben sich falsch Lärmwerte!

### Voraussetzungen

- Benutzung der Firmware Versionen NRZ-2020-134-DNMS-5.2 auf der NodeMCU damit die Z-Werte abgefragt werden bzw. konfiguriert werden. Für den Raspberry Pi ist eine entsprechende Version in Vorbereitung. Die Werte des 2. Messintervalls, die Terzwerte und alle Z-Werte werden nicht an Sensor.Community übertragen.
- Benutzung einer InfluxDB oder eigenen Anwendung um die Werte des 2. Messintervalls, die Terzwerte und die Z-Werte abzuspeichern.



------------------------------------------------------------------------


# DNMS Firmware Teensy4.0 V5 English

## V5 realizes besides the functionality of V4 the calculation and output of Z-values 

### Current versions:

- DNMS_V5.0.4 - for ICS-43434 Mikrofon
- DNMS_V5.0.6 - for IM72D128 Mikrofon

Note: Using inappropriate versions of microphone and Teensy firmware will result in erroneous noise values!

### Prerequisites

- Usage of firmware versions NRZ-2020-134-DNMS-5.2-en for NodeMCU to get  Z-values read out of Teensy4.0. For Raspberry Pi there is a version with these features under preparationl. Values from the 2nd interval, 1/3 octave values and Z-values are not transmitted to Sensor.Community.
- Usage of an InfluxDB or an own application for storing the 2nd interval values, 1/3 octaves values and Z-values is necessary.

