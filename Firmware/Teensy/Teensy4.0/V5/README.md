# [English text below](#dnms-firmware-teensy40-v5-english)

# DNMS Firmware Teensy4.0 V5

## V5 realisiert neben der Funktionalität von V4 zusätzlich die Berechnung und Ausgabe von Z-Werten

### Versions Historie:

Bitte beachten: Wird das IM72D128 Mikrofon eingesetzt, muss auf dem Teensy4.0 Board die Firmware DNMS_V5.2.6 oder DNMS_V5.5.x und höher geflasht sein. Die Firmware DNMS_V5.5.x kann mit dem IM72D128 benutzt werden, wenn auf dem Kommunikationsprozessor (NodeMCU oder Raspiberry Pi) eine Firmware vorhanden ist, die die Umschaltung zwischen den Mikrofonen ICS-43434 und IM72D128 erlaubt. Wird z.B. auf der NodeMCU eine ältere Firmware oder die Standard Firmware von Sensor.Community eingesetzt, so muss auf dem Teensy4.0 Board die DNMS_V5.2.6 Firmware geflasht sein.


- DNMS_V6.0.x für IM72D128 und ICS-43434 Mikrofone.   
 	+ Komplettes Refactoring der Firmware mit Optimierung der Verwendung von Double (nur noch wenn absolut notwendig), dadurch erfordert die Berechnung der Leq Werte deutlich weniger Ressourcen.
 	+  Fehlerbeseitigung einer Race-Condition zwischen 1. und 2. Messintervall.
 	+  Die Parameterwerte zur Frequenzgangkorrektur sind nicht mehr in der Firmware fest abgespeichert. Es werden für jeden Mikrofontyp 31 Korrekturwerte vom Kommunikationsprozessor an die Teensy Firmware beim Start übertragen, daraus berechnet die Teensy Firmware die Korrekturwerte für den gewählten Mikrofontyp für die FFT Bins.
 	+  Änderung der Filterparameter für die I²C Kommunikation zwischen Teensy und Kommunikationsprozessor, dadurch stabilere I²C Übertragung. Beim Kompilieren der Firmware ist es deshalb notwendig die angepasste teensy4_i2c-master Library zu benutzen (zu finden unter Firmware/Teensy/Teensy4.0/).
 
  	<mark>Achtung: Um diese Firmware zu nutzen, ist die Firmware Version airrohr-DNMS-6.0.0 (oder höher) beim NodeMCU Kommunikationsprozessor oder beim Raspberry Pi  die Version dnms-0.9.27 bzw. dnms-0.9.28 (oder höher) notwendig. Nur diese Versionen haben die Parameter zur Frequenzgangkorrektur abgespeichert und übertragen diese Werte beim Start.<mark>
 

- DNMS_V5.9.x  für IM72D128 und ICS-43434 Mikrofone mit dem Unterschied zur Version DNMS_V5.8.x, dass zusätzlich C-Werte berechnet und abgerufen werden können. Wenn keine C-Werte benötigt werden, ist die Version  DNMS_V5.8.x weiterhin aktuell. Eine Version mit Abfrage der C-Werte ist die Raspberry Pi Version dnms-0.9.25. Eine Abfrage der C-Werte für die NodeMCU Firmware ist z.Zt. nicht geplant.

- DNMS_V5.8.x  für IM72D128 und ICS-43434 Mikrofone mit:
	+ zusätzlicher Auswahl der Frequenzgangkorrektur für das DLR Gehäuse mit IM72D128 Mikrofon, sowie Auswahl ohne Frequenzgangkorrektur für IM72D128 und ICS-43434 Mikrofon.
	+ Kleine Korrekturen im Source Code, so dass in der Arduino IDE Version 2.3.8 die Übersetzung ohne Warnungen erfolgt. Dies betrifft auch die Library dnms_audio_lib-master.zip, deshalb beim Übersetzen die veränderte Library einbinden.
	+ Korrekturen an der I²C Übertragung, Änderung der Werte für die Glitch Filter von SDA und SCL im Slave Modus. Diese Korrekturen befinden sich in der Library  teensy4_i2c-master.zip, deshalb beim Übersetzen die veränderte Library einbinden.


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

Please note: If the IM72D128 microphone is used, the firmware DNMS_V5.2.6 or DNMS_V5.5.x and higher must be flashed on the Teensy4.0 board. The firmware DNMS_V5.5.x can be used with the IM72D128 if there is firmware on the communications processor (NodeMCU or Raspiberry Pi) that allows switching between the ICS-43434 and IM72D128 microphones. For example, if older firmware or the standard firmware from Sensor.Community is used on the NodeMCU, the DNMS_V5.2.6 firmware must be flashed on the Teensy4.0 board.

- DNMS_V6.0.x for IM72D128 and ICS-43434 microphones.   
 	+ Complete refactoring of the firmware, optimising the use of `double` (now only used when absolutely necessary), meaning that the calculation of Leq values requires significantly fewer resources.
 	+  Resolving a race condition between the first and second measurement intervals.
 	+  The parameter values for frequency response correction are no longer hard-coded in the firmware. For each microphone type, 31 correction values are transferred from the communication processor to the Teensy firmware at start-up; the Teensy firmware then uses these to calculate the correction values for the selected microphone type for the FFT bins.
 	+  Changes to the filter parameters for I²C communication between the Teensy and the communication processor, resulting in more stable I²C transmission. When compiling the firmware, it is therefore necessary to use the modified teensy4_i2c-master library (located in Firmware/Teensy/Teensy4.0/).
 
 	<mark>Please note: To use this firmware, the NodeMCU communication processor must be running firmware version airrohr-DNMS-6.0.0 (or later), whilst the Raspberry Pi must be running version dnms-0.9.27 or dnms-0.9.28 (or later). Only these versions have the frequency response correction parameters stored and transmit these values on start-up.<mark>

- DNMS_V5.9.x  for IM72D128 and ICS-43434 microphones; the difference from version DNMS_V5.8.x is that C-values can now also be calculated and retrieved. If C-values are not required, version  DNMS_V5.8.x remains the current version. One version that retrieves C-values is the Raspberry Pi version dnms-0.9.25. There are currently no plans to retrieve C-values for the NodeMCU firmware.

- DNMS_V5.8.x  for IM72D128 and ICS-43434 microphones with:
	+ additional selection of frequency response correction for the DLR housing with IM72D128 microphone, as well as a selection without frequency response correction for IM72D128 and ICS-43434 microphones
	+ minor corrections have been made to the source code so that the code compiles without warnings in Arduino IDE version 2.3.8. This also applies to the dnms_audio_lib-master.zip library, so please include the updated library when compiling.
	+ corrections to the I²C transmission; changes to the values for the SDA and SCL glitch filters in slave mode. These corrections are contained in the library teensy4_i2c-master.zip, so be sure to include the updated library when compiling.

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

