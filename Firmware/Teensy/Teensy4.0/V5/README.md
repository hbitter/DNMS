# [English text below](#dnms-firmware-teensy40-v5-english)

# DNMS Firmware Teensy4.0 V5

## V5 realisiert neben der Funktionalität von V4 zusätzlich die Berechnung und Ausgabe von Z-Werten

### Versions Historie:

Bitte beachten: Wird das IM72D128 Mikrofon eingesetzt, muss auf dem Teensy4.0 Board die Firmware DNMS_V5.2.6 oder DNMS_V5.5.x geflasht sein. Die Firmware DNMS_V5.5.x kann mit dem IM72D128 benutzt werden, wenn auf dem Kommunikationsprozessor (NodeMCU oder Raspiberry Pi) eine Firmware vorhanden ist, die die Umschaltung zwischen den Mikrofonen ICS-43434 und IM72D128 erlaubt. Wird z.B. auf der NodeMCU eine ältere Firmware oder die Standard Firmware von Sensor.Community eingesetzt, so muss auf dem Teensy4.0 Board die DNMS_V5.2.6 Firmware geflasht sein.

- DNMS_V5.5.x  für IM72D128 und ICS-43434 Mikrofone mit:
	+ Verbesserter Korrektur des Frequenzgangs für IM72D128 Mikrofon und ICS-43434 Mikrofon. Dies verbessert auch die Terzwerte des Spektrums.

- DNMS_V5.4.x - für ICS-43434 und IM72D128 Mikrofon
	+ Ersetzt DNMS_V5.3.x da in der Version DNMS_V5.3.x Fehler in der Berechnung der Terzwerte waren. DNMS_V5.3.x bitte ersetzen mit DNMS_V5.4.x 

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

Please note: If the IM72D128 microphone is used, the firmware DNMS_V5.2.6 or DNMS_V5.5.x must be flashed on the Teensy4.0 board. The firmware DNMS_V5.5.x can be used with the IM72D128 if there is firmware on the communications processor (NodeMCU or Raspiberry Pi) that allows switching between the ICS-43434 and IM72D128 microphones. For example, if older firmware or the standard firmware from Sensor.Community is used on the NodeMCU, the DNMS_V5.2.6 firmware must be flashed on the Teensy4.0 board.

- DNMS_V5.5.x  for IM72D128 and ICS-43434 microphones with:
	+ Improved frequency correction for IM72D128 microphone and ICS-43434 microphone. It also improves the 1/2 octave values of the spectrum.

- DNMS_V5.4.x - for ICS-43434 and IM72D128 microphone
	+ Replaces DNMS_V5.3.x because of an error in the calculation of the 1/3 octave values in DNMS_V5.3.x. Please replace DNMS_V5.3.x with DNMS_V5.4.x.

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

