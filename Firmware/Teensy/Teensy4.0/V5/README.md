# [English text below](#dnms-firmware-teensy40-v5-english)

# DNMS Firmware Teensy4.0 V5

## V5 realisiert neben der Funktionalität von V4 zusätzlich die Berechnung und Ausgabe von Z-Werten

### Versions Historie:

- DNMS_V5.3.x - für ICS-43434 und IM72D128 Mikrofon
	 - Zusammenführung der bisher getrennten Versionen für die beiden Mikrofone ICS-43434 und IM72D128 in einer Teensy4.0 Firmware. Die Auswahl des Mikrofons erfolgt über die Konfiguration in der NodeMCU bzw. im Raspberry Pi und wird mittels Umschaltbefehl an die Teensy4.0 Firmware übertragen. Wird kein Umschaltbefehl übertragen ist standardmäßig das ICS-43434 ausgewählt d.h. auch ältere NodeMCU Firmware unterstützt die Version DNMS_V5.3.x mit ICS-43434 Mikrofon.
	 - Abhänging vom ausgewählten Mikrofon erfolgt das Blinken der roten LED auf dem Teensy4.0 Board mit unterschiedlicher Frequenz:	
ICS-43434: im Wechsel 100ms an und 100ms aus	
IM72D128: im Wechsel 500ms an und 500ms aus


- DNMS_V5.2.4 - für das ICS-43434 Mikrofon
- DNMS_V5.2.6 - für das IM72D128 Mikrofon	
	
	Bitte beachten: Werden nicht zusammengehörige Versionen von Mikrofon und Teensy Firmware benutzt, so ergeben sich falsch Lärmwerte!

### Voraussetzungen

- Benutzung der Firmware Versionen NRZ-2020-134-DNMS-5.2 auf der NodeMCU damit die Z-Werte abgefragt werden bzw. konfiguriert werden. Für den Raspberry Pi ist eine entsprechende Version in Vorbereitung. Die Werte des 2. Messintervalls, die Terzwerte und alle Z-Werte werden nicht an Sensor.Community übertragen.
- Benutzung einer InfluxDB oder eigenen Anwendung um die Werte des 2. Messintervalls, die Terzwerte und die Z-Werte abzuspeichern.



------------------------------------------------------------------------


# DNMS Firmware Teensy4.0 V5 English

## V5 realizes besides the functionality of V4 the calculation and output of Z-values 

### Version history:


- DNMS_V5.3.x - for ICS-43434 and IM72D128 microphone
	 - Merging the previously separate versions for the two microphones ICS-43434 and IM72D128 into one Teensy4.0 firmware. The microphone is selected via the configuration in the NodeMCU or Raspberry Pi and is transferred to the Teensy4.0 firmware using a switching command. If no switching command is transmitted, the ICS-43434 is selected by default, i.e. older NodeMCU firmware also supports version DNMS_V5.3.x with ICS-43434 microphone.
	 - Depending on the selected microphone, the red LED on the Teensy4.0 board flashes at different frequencies:	
ICS-43434: alternating 100ms on and 100ms off	
IM72D128: alternating 500ms on and 500ms off

- DNMS_V5.2.4 - for ICS-43434 Mikrofon
- DNMS_V5.2.6 - for IM72D128 Mikrofon	

	Note: Using inappropriate versions of microphone and Teensy firmware will result in erroneous noise values!

### Prerequisites

- Usage of firmware versions NRZ-2020-134-DNMS-5.2-en for NodeMCU to get  Z-values read out of Teensy4.0. For Raspberry Pi there is a version with these features under preparationl. Values from the 2nd interval, 1/3 octave values and Z-values are not transmitted to Sensor.Community.
- Usage of an InfluxDB or an own application for storing the 2nd interval values, 1/3 octaves values and Z-values is necessary.

