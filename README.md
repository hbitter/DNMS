
## [English text below](#dnms-digital-noise-measurement-sensor-english)

## DNMS (Digital Noise Measurement Sensor)

Ein Sensor-Modul zur Schallpegelmessung. Es wird neben dem LAeq-Wert über einem Zeitintervall auch der min. und max. LA-Wert in diesem Zeitintervall festgehalten. Das zentrale Element des DNMS ist ein digitales MEMS-Mikrofon (InvenSense ICS-43434). Ein Mikrocontroller-Board (Teensy3.6 / Teensy4.0) übernimmt die digitalen Daten vom Mikrofon über die I²S Schnittstelle als  24-Bit, 44,1 kHz PCM Daten (ab Firmware Version 2 - zuvor nur 16-Bit).  Die Audiodaten werden über einen digitalen A-Filter bewertet, dann wird der Effektivwert bestimmt und daraus der Schalldruckpegel entsprechend der Mikrofonsensitivität berechnet. Die einzelnen Schalldruckpegelwerte werden zu einem fortlaufenden LAeq-Wert weitergerechnet. Ebenso werden der max. und min. Schalldruckpegel (LAmax, LAmin) aus dem Intervall abgespeichert.

Die Werte werden dann über eine I²C-Schnittstelle an einen abfragenden Mikrocontroller zur Weiterverarbeitung übertragen. Das Abfrage-Intervall kann 1 Sekunde betragen oder auch bis zu 3600 Sekunden.

Eine Version mit der Möglichkeit das Frequenzspektrum zu betrachten ist seit einiger Zeit verfügbar. Terzwerte von 20Hz - 20000Hz können ausgegeben werden. Terzwerte werden z.Zt. nicht von Sensor.Community unterstützt, deshalb gibt es spezielle Versionen der NodeMCU Firmware um Terzwerte auf der lokalen Webseite der NodeMCU auszugeben und an weitere APIs wie z.B. zu einer InfluxDB zu senden.

Mit der Firmware Version 5 für Teensy4.0 werden nun auch Z-Werte (LZeq, LZmin, LZmax sowie Z-Terzwerte) unterstützt.

### Bei der Firma Watterott electronic GmbH kann nun das Mikrofon IM72D128 von Infineon fertig in einem Kunststoffrohr eingegossen im Shop bestellt werden (https://shop.watterott.com/Mikrofon-IM72D128V01-im-127mm-Rundrohr). Das IM72D128 Mikrofon auf einer Platine und mit Anschlusskabel aber nicht  im Kunststoffrohr eingegossen ist ebenfalls verfügbar (https://shop.watterott.com/Mikrofon-IM72D128V01-Platine-mit-Anschlussleitung).  Das DNMS Teensy4.0 Board wird demnächst verfügbar sein.
 
## Neuigkeiten

### Mai 2025

- Neues Testprogramm für Teensy4.0 Board und angeschlossenem Mikrofon ICS-43434:  DNMS_Vt.8.4 und für Mikrofon IM72D128: DNMS_Vt.8.6, mit verbesserter Frequenzgangkorrektur

- Neue Teensy 4.0 Firmware DNMS_V5.5.x  für IM72D128 und ICS-43434 Mikrofone mit:
	+ Verbesserte Korrektur des Frequenzgangs für IM72D128 Mikrofon und ICS-43434 Mikrofon. Dies verbessert auch die Terzwerte des Spektrums.

 - Neue Raspberry Pi Firmware dnms-0.9.23 ersetzt dnms-0.9.21 aufgrund von Fehlern:
	 + Bei der Umwandlung der MAC-Adresse in die ID
	 + Entfernung Testausgabe der Daten auf Konsole bei Sensor.Community Übertragung
	 + Beseitigung Pufferüberlauf für InfluxDB/MQTT Übertragung, wenn keine Übertragung konfiguriert war

### Oktober 2024

- Neue NodeMCU Firmware AIRROHR-DNMS-5.16 mit:
	+ Neue Konfigurationsmöglichkeit die WLAN Verbindung ohne Passwort auszuwählen aufgrund einer Änderung in der Wifi Lib. Dies ist notwendig  z.B. für Freifunk WLAN Verbindungen.
	+ Verbesserte Fehlerbehandlung bei der Datenübertragung und entsprechende Fehlerausgaben.
	+ Fehlerbeseitigung bei der Übertragung des Zeitstempels zur InfluxDB und zu einem MQTT Broker, wenn der DNMS Sensor nicht konfiguriert ist.

### September 2024

 - Raspberry Pi dnms-0.9.21 ersetzt Version dnms-0.9.20 aufgrund von Fehlern.
 	+ Fehler in der dnms.conf beim Eintrag 'mqtt_transmit'.
 	+ Fehlerhaft gesetzte Prioritäten für die Threads in der dnms.conf ('prio_1st_timer', ... usw).
 	+ Neben der InfluxDB HTTP Übertragung kann nun parallel eine Übertragung der Werte zu einem MQTT Broker erfolgen.
 		
 - Raspberry Pi dnms-0.9.20 Version mit folgenden neuen Funktionen:
	+ Data Logging der Messwerte auf der SD-Karte des Raspberry Pi
	+ Externer Start und Stop der Messwertübertragung, Ausgabe und Speicherung mittels einer named pipe
	+ Änderung der Eingabe von Bool-Werten in der Konfigurationsdatei dnms.conf: Für false eine '0' und für true eine '1' eingeben.

 - Möglichkeit die NodeMCU Firmware direkt über den Webbrowser auf einen ESP8266  zu flashen. Dies ist möglich mit allen Webbrowsern, die das Web Serial API unterstützen (Chrome, Edge, Brave, Opera aber nicht Firefox ). Um die Funktion zu benutzen,  den entsprechenden Link aufrufen abhängig von der gewünschten Sprachversion :
	+ Deutsche Firmware: https://hbitter.github.io/DNMS/flash-de.html
	+ Englische Firmware: https://hbitter.github.io/DNMS/flash-en.html
	+ Französische Firmware: https://hbitter.github.io/DNMS/flash-fr.html

### Juli 2024

- Neue Firmware für Teensy4.0 DNMS_V5.4.x - für ICS-43434 und IM72D128 Mikrofon
	+ Ersetzt DNMS_V5.3.x da in der Version DNMS_V5.3.x Fehler in der Berechnung der Terzwerte waren. DNMS_V5.3.x bitte ersetzen mit DNMS_V5.4.x 
	
- Neue NodeMCU Firmware AIRROHR-DNMS-5.12 mit:
	+ NTP Server können nun in der Konfiguration gesetzt werden unter "Weitere Einstellungen"
	+ Konfigurationsänderung für die Übertragung zum MQTT Broker, eigenes Feld für die Konfiguration der MQTT Messung. Dies wird im InfluxDB Line Protocol als Measurement weitergegeben

 - Raspberry Pi dnms-0.9.18 Fehlerkorrektur und neue Funktion:
	 - Korrektur eines Fehlers: Wenn nur die Ausgabe auf dem Terminal konfiguriert war, wurden die Werte nicht richtig auf dem Terminal ausgegeben.
 	- Neue Möglichkeit der Weitergabe von Messwerten mittels einer named pipe (oder auch fifo) zu anderen Anwendungen auf dem Raspberry Pi.



### Juni 2024

 - Raspberry Pi dnms-0.9.16 Korrektur der Versionsnummer:
	 - Die Versionsnummer wurde falsch (0.9.15 statt 0.9.16) angezeigt
 - Neue NodeMCU Firmware AIRROHR-DNMS-5.8 mit:
	 - MQTT Übertragung (z.Zt. kein TLS)
	 - Unterstützung der neuen Teensy4.0 Firmware DNMS_V5.3.x (Mikrofone ICS-43434 und IM72D128 in einer Teensy4 Firmware)
	 - Möglichkeit nach Restart mit den Messungen zur vollen Minute oder zur vollen Stunde zu beginnen<br>
	 
	 Weitere Details dazu im README.md unter Firmware/NodeMCU.

 - Erweiterte Raspberry Pi Anwendung dnms-0.9.16 mit:
	 - MQTT Übertragung zu einem MQTT-Broker (z.Zt. kein TLS)
	 - Unterstützung der neuen Teensy4.0 Firmware DNMS_V5.3.x (Mikrofone ICS-43434 und IM72D128 in einer Teensy4 Firmware)
	 - Einführung eines DNMS Korrekturwertes für die Lärmwerte (nicht für Frequenzspektren) entsprechend der NodeMCU Firmware  <br>
	 
	 Weitere Details dazu im README.md unter Firmware/Raspberry Pi.
	 
 - Neue Teensy4.0 Firmware DNMS_V5.3.x mit:
	 - Zusammenführung der bisher getrennten Versionen für die beiden Mikrofone ICS-43434 und IM72D128 in einer Teensy4.0 Firmware. Die Auswahl des Mikrofons erfolgt über die Konfiguration in der NodeMCU bzw. im Raspberry Pi und wird mittels Umschaltbefehl an die Teensy4.0 Firmware übertragen. Wird kein Umschaltbefehl übertragen ist als standardmäßig das ICS-43434 ausgewählt d.h. auch ältere NodeMCU Firmware unterstützt die Version DNMS_V5.3.x mit ICS-43434 Mikrofon. <br>

	 Weitere Details dazu im README.md unter Firmware/Teensy4.0/V5.	 

### April 2024
 - Neue NodeMCU Firmware NRZ-2020-134-DNMS-5.6 mit:
	 - Fehlerbeseitigung für DNMS Korrekturwert (Issue #54)
	 - LED Statusanzeige
	 - geänderte Speicheraufteilung<br>
	 
	 Weitere Details dazu im README.md unter Firmware/NodeMCU.

### Februar 2024
   - Verbesserte Teensy4.0 Firmware für die Mikrofone ICS-43434 und IM72D128 mit verbessertem internen Zeitverhalten (Upgrade empfohlen): 
     - DNMS_V5.2.4 - für ICS43434 mit Teensy4.0
     - DNMS_V5.2.6 - für IM72D128 mit Teensy4.0
 	
  - Standalone Testprogramme für Teensy4.0 und die Mikrofone ICS-43434 und IM72D128 mit verbessertem internen Zeitverhalten	und geänderter Ein- und Ausgabe beim Start, es können Messintervall ab 125ms eingestellt werden, die Ausgabe von Terzwerten ist ab 500ms möglich:
 	- DNMS_Vt.7.4 - für ICS43434 mit Teensy4.0
 	- DNMS_Vt.7.6 - für IM72D128 mit Teensy4.0
  
  - dnms-0.9.12 erste Version der Raspberry Pi Anwendung um den DNMS Sensor auszulesen und Daten an Sensor.Community und/oder zu einer InfluxDB zu senden als Alternative zu einer NodeMCU (Airrohr-Board). Die Raspberry Pi Anwenung unterstützt zur Übertragung an eine InfluxDB zwei zeitlich unterschiedliche Messintervalle mit unterschiedlich konfigurierbaren Messwerten. Die Messwerte des 2. Messintervalls werden erst ab der Überschreitung eines Schwellwertes (LAeq) übertragen.  
  - KiCad Layouts an KiCad Version 7 angepasst für die folgenden PCBs (im Layout selbst keine Änderung):
	  + DNMS-T4.0-V1.7
	  + DNMS-RPI-HAT-V1.0
	  + IM72D128-PCB-V1.0
	  + IM72D128-Tube-PCB-V1.0
  

### September 2023
  - Neue NodeMCU Firmware NRZ-2020-134-DNMS-5.4 auch in englischer und französischer Version mit:
    - Verbesserungen bei der Fehlerausgabe von Übertragungen zu Sensor.Community und zu einer InfluxDB
    - Vergrößerung des Übertagungspuffers, so dass neben sämtlichen DNMS Werten auch die Werte weiterer Sensoren zu einer InfluxDB übertragen werden können.
    - Link zur Meldung von Problemen geändert auf https://github.com/hbitter/DNMS/issues 

### August 2023
 - Standalone Testprogramme für Teensy4.0 und die Mikrofone ICS-43434 und IM72D128:
 	- DNMS_Vt.5.4 - für ICS43434 mit Teensy4.0
 	- DNMS_Vt.5.6 - für IM72D128 mit Teensy4.0
 
 	Diese Testprogramme basieren auf der Teensy4.0 Firmware Version 5. Neben einem einfachen Test, ob Mikrofon und Teensy4.0 funktionieren, können die Messwerte mittels einem geeigneten  Terminalprogramm auch aufgezeichnet und weiter ausgewertet werden. Das Messintervall kann eingegeben werden.
 	
 - Airrohr-ESP32-PCB-V1.1 PCB gelöscht - obsolet, da keine Firmware Unterstützung.	
 
### Juli 2023
 - Unterstützung des Mikrofons IM72D128 von Infineon. Das IM72D128 bietet zwei wesentliche Vorteile:
   - IP57 Schutz gegen Staub und Feuchtigkeit im Mikrofon integriert (trotzdem ist ein zusätzlicher Wind-/Wetterschutz sinnvoll).
   - hoher Signal to noise ratio (SNR) von 72 dB(A), dadurch wird ein unterer Messbereich von ca. 23 dB(A) erreicht.

  - PCBs für das IM72D128 Mikrofon. Ein rundes Mikrofon Board und ein längliches Board mit einer Schaltung zur Umsetzung der PDM Schnittstelle des Mikrofons auf die I²S Schnittstelle, dadurch ist die Anschlusskompatibilität zum bisherigen ICS-43434 Mikrofon gegeben.

 - Teensy4.0 Firmware V5, die zusätzlich die Berechnung und Ausgabe von Z-Werten unterstützt (LZeq, LZmin, LZmax sowie die LZeq Werte der Terzen von 20Hz - 20.000Hz). Für das bisherige Mikrofon ICS-43434 und das neue Mikrofon IM72D128 gibt es zwei unterschiedliche Versionen:
    - DNMS_V5.0.4 für das bisherige Mikrofon ICS-43434
    - DNMS_V5.0.6 für das neu unterstützte Mikrofon IM72D128
 
 - Neue NodeMCU Firmware Versionen NRZ-2020-134-DNMS-5.2 und NRZ-2020-134-DNMS-5.2-en zur Unterstützung der Z-Werte. Diese Versionen sind auch kompatibel mit allen Teensy Firmware Versionen (V2, V3, V4 und V5) und ersetzen damit die älteren NodeMCU Versionen.
 
   Weitere Details im README.md unter NodeMCU<br><br>

### März 2023

 - Neue NodeMCU Firmware Versionen NRZ-2020-134-DNMS-4.4 und NRZ-2020-134-DNMS-4.4-en (ersetzt auch die Versionen NRZ-2020-134-DNMS-3.3 und NRZ-2020-134-DNMS-3.3-en):
 
   - Instabiles Verhalten des lokalen NodeMCU Webservers beseitigt
   - SDS011 mit dieser Version wieder nutzbar
   - Erweiterte Statusausgaben bei einigen Sensoren (DNMS, SEN5X, SCD30, SCD40)
   - Unterstützung weiterer Sensoren (SEN5X, SCD30, SCD40)
   
   
   Weitere Details im README.md unter NodeMCU<br><br>

### Januar 2023
- Fehlerkorrektur Teensy3.6 Firmware, die neuen Versionen DNMS_V2.5.0 und DNMS_V2.5.1 beseitigen eine Fehler bei der Initialisierung.
- Korrektur der NodeMCU Firmware, die neuen Versionen NRZ-2020-134-DNMS-3.3, NRZ-2020-134-DNMS-3.3-en, NRZ-2020-134-DNMS-4.3 und NRZ-2020-134-DNMS-4.3-en korrigieren die I²C Takt Frequenz, jetzt näher bei 100kHz.<br><br>

- Es gibt nun drei aktuelle Versionen der Teensy Firmware, die sich im Funktionsumfang unterscheiden:
	- V2 - Funktionsumfang wie bisher d.h. LAeq, LAmin und LAmax werden berechnet und ausgegeben.
	- V3 (nur für Teensy4.0) - zusätzlich zum Funktionsumfang von V2 werden zur Auswertung des Frequenzspektrums die Terzwerte (A-bewertet) von 20Hz - 20000Hz ausgegeben. Die Terzwerte werden nicht zu Sensor.Community übertragen sondern nur auf die lokale Webseite der NodeMCU und zu weiteren APIs übertragen (z.B. zu einer InfluxDB). Zur Nutzung dieser Teensy Version V3 muss die korrespondierende Firmware Version für die NodeMCU (z.Zt. NRZ-2020-134-DNMS-3.2 bzw. die englische Version NRZ-2020-134-DNMS-3.2/EN) benutzt werden. 
	- V4 (nur für Teensy4.0) - zusätzlich zum Funktionsumfang von V3 wird ein zweites Messintervall unterstützt um z.B. in einem kürzeren Messintervall als das 150 Sekunden Intervall von Sensor.Community Werte zu einer eigenen InfluxDB zu senden. Die Übertragung der Messwerte des 2. Messintervalls kann auch erst ab Überschreitung eines konfigurierbaren Schwellwertes gestartet werden. Zur Nutzung dieser Teensy Version V4 muss die korrespondierende Firmware Version für die NodeMCU (z.Zt. NRZ-2020-134-DNMS-4.2 bzw. die englische Version NRZ-2020-134-DNMS-4.2/EN) benutzt werden.

- Änderung der Zeitbewertung in allen aktuellen Teensy Firmware Versionen von "Impuls" auf "Fast" entsprechend 125ms.  Dies führt zu etwas niedrigeren LAmax und LAeq Werten und zu etwas höheren LAmin Werten aber entspricht besser der gebräuchlichen "Fast" Zeitbewertung.

- Die Teensy Firmware Versionen V2, V3 und V4 sind abwärtskompatibel d.h. ein Teensy4.0 mit einer Firmware V4 kann zusammen mit einer NodeMCU, die nur den Funktionsumfang V2 unterstützt, betrieben werden, es ist dann nur der geringere Funktionsumfang verfügbar. Umgekehrt kann an einer NodeMCU z.B. mit der aktuellen Firmware NRZ-2020-134-DNMS-4.2 auch ein Teensy mit der Firmware Version V2 betrieben werden, es ist dann ebenfalls nur der geringere Funktionsumfang verfügbar.

- Falls Probleme mit der aktuellen Teensy Firmware beim Datentransfer mit der standard NodeMCU Firmware von Sensor.Community auftreten z.B. DNMS wird nicht erkannt, bitte die NodeMCU Firmware NRZ-2020-134-DNMS-3.2 oder NRZ-2020-134-DNMS-4.2 von hier benutzen.

- geänderte PCBs:
	- ICS-43434-PCB-V1.4 - Durchmesser des akustischen Ports von 0,6mm auf 0,7mm geändert
	- DNMS-T4.0-V1.7 - zusätzliche Steckverbinder für die Verbindung zum Mikrofon

- Software um einen Raspberry Pi als Kommunikationsprozessor für DNMS mit Übertragung der Werte zu Sensor.Community und zu einer InfluxDB zu nutzen:
	- dnms-0.9.5 erste Version für Raspi Pi Zero W, Raspi Pi Zero 2 W und andere



----------------------------------------------------------------------------------------------------------------
## DNMS (Digital Noise Measurement Sensor) English

A sensor modul for SPL measurement. Over a period of time the LAeq value is calculated, beside that the minimal and the maximal LA value is covered over the time period. The central element of DNMS is a digital MEMS-Microphone (InvenSense ICS-43434). The digital data is fed from the microphone via a I²S connection as 24-bit, 44,1 kHz PCM data (from firmware version 2 on - before it was only 16-Bit) to a microcontroller board (Teensy3.6 / Teensy4.0). The data is filtered by an A-filter, then the RMS value is calculated. Further the sound pressure level (SPL) is calculated based on microphone sensivity. The SPL values are integrated over time to give the LAeq. Also the maximum level in the measurement period LAmax and the minimum level LAmin will be kept.

All measurements will be transfered to a further microcontroller via I²C bus. The intervall time to pick up measurements can reach from 1 second up to 3600 seconds.

A version with support of frequency spectrum is available for some time. 1/3 octave values from 20Hz - 20000Hz are calculated. 1/3 octave values are not supported by Sensor.Community so far. Therefore a special version of the NodeMCU firmware is available, showing the 1/3 octave values on the local webpage of the NodeMCU as well as transmitting the 1/3 octave values to an InfluxDB.

Firmware version 5 for Teensy4.0 now supports Z-values (LZeq, LZmin, LZmax as well as 1/3 octave Z-values).

### The IM72D128 microphone from Infineon can now be ordered ready-molded in a plastic tube from Watterott electronic GmbH in the shop (https://shop.watterott.com/Mikrofon-IM72D128V01-im-127mm-Rundrohr). The IM72D128 microphone on a circuit board and with a connection cable but not cast in the plastic tube is also available (https://shop.watterott.com/Mikrofon-IM72D128V01-Platine-mit-Anschlussleitung).  The DNSMS Teensy4.0 board will be available soon.
 
## News

### May 2025

- New test program for Teensy 4.0 board and connected ICS-43434 microphone: DNMS_Vt.8.4 and
test program for IM72D128 microphone: DNMS_Vt.8.6, both with improved microphone frequency correction

- New Teensy 4.0 firmware DNMS_V5.5.x for IM72D128 and ICS-43434 microphones with:
	+ Improved frequency correction for IM72D128 microphone and ICS-43434 microphone. It also improves the 1/2 octave values of the spectrum

 - New Raspberry Pi firmware dnms-0.9.23 replaces version dnms-0.9.21 due to bugs:
	 + MAC address to ID conversion was incorrect in some circumstances
	 + Removal of test output on console for transmission to Sensor.Community
	 + Fix for buffer overflow on InfluxDB/MQTT transfers when one transfer is configured

### Oktober 2024

- New NodeMCU firmware AIRROHR-DNMS-5.16 with:
	+ New configuration option to select the WLAN connection without a password due to a change in the Wifi Lib. This is necessary, for example, for Freifunk WLAN connections.
	+ Improved error handling during data transfer and corresponding error output.
	+ Bug fix when transferring the timestamp to InfluxDB and an MQTT broker when the DNMS sensor is not configured.

### September 2024

 - Raspberry Pi dnms-0.9.21 replaces version dnms-0.9.20 due to bugs.
 	+ Error in dnms.conf with entry 'mqtt_transmit'.
 	+ Incorrectly set priorities for the threads in dnms.conf ('prio_1st_timer', ... etc).
 	+ In addition to the InfluxDB HTTP transfer, the values ​​can now be transferred to an MQTT broker in parallel.

 - Raspberry Pi dnms-0.9.20 release with the following new features:
	+ Data logging of measurements to Raspberry Pi SD card
	+ External start and stop of transmission of measurements, output and storage using a named pipe
	+ Changed entry of bool values in the dnms.conf configuration file: Enter '0' for false and '1' for true.

- Ability to flash NodeMCU firmware to an ESP8266 directly from a web browser. This is possible with all web browsers that support the Web Serial API (Chrome, Edge, Brave, Opera but not Firefox). To use this feature, click on the appropriate link for your language version:
	+ German firmware: https://hbitter.github.io/DNMS/flash-de.html
	+ English firmware: https://hbitter.github.io/DNMS/flash-en.html
	+ French firmware: https://hbitter.github.io/DNMS/flash-fr.html

### July 2024

- New NodeMCU firmware AIRROHR-DNMS-5.12 with:
	+ NTP servers can now be set in the configuration under "More settings".
	+ Configuration change for MQTT Broker transmission, separate field for configuring the MQTT measurement. This will be reported as measurement in the InfluxDB line protocol.

- New firmware for Teensy4.0 DNMS_V5.4.x - for ICS-43434 and IM72D128 microphone
	+ Replaces DNMS_V5.3.x because of an error in the calculation of the 1/3 octave values in DNMS_V5.3.x. Please replace DNMS_V5.3.x with DNMS_V5.4.x.
	
- Raspberry Pi dnms-0.9.18  bug fix and new feature:
	 - Bug fix: If only output to terminal was configured, the values were not output correctly to the terminal.
  	- New way to pass measurements to other applications on the Raspberry Pi using a named pipe (or fifo).

### June 2024

 - Raspberry Pi dnms-0.9.16 Fix for version number output:
	 - Version number was displayed as 0.9.15 instead of 0.9.16
 - New NodeMCU firmware AIRROHR-DNMS-5.8 with:
	 - MQTT transmission (currently no TLS)
	 - Support of new Teensy4.0 firmware DNMS_V5.3.x (microphones ICS-43434 and IM72D128 supported in one single Teensy4.0 firmware)
	 - Possibility to start at full minute or full hour after restart<br>
	 
	 See README.md under Firmware/NodeMCU for details.

 - Enhanced Raspberry Pi application dnms-0.9.16 with:
	 - MQTT transmission (currently no TLS)
	 - Support of new Teensy4.0 firmware DNMS_V5.3.x (microphones ICS-43434 and IM72D128 supported in one single Teensy4.0 firmware)
	 - Implementation of DNMS correction for noise values (not for 1/3 octave values) according to the NodeMCU firmware  <br>
	 
	See README.md under Firmware/Raspberry Pi for details.
	 
 - New Teensy4.0 firmware DNMS_V5.3.x with:
	 - Merging the previously separate versions for the two microphones ICS-43434 and IM72D128 in one Teensy4.0 firmware. The microphone is selected via the configuration in the NodeMCU or Raspberry Pi and is transferred to the Teensy4.0 firmware using a switching command. If no switching command is transmitted, the ICS-43434 is selected as default, i.e. older NodeMCU firmware also supports version DNMS_V5.3.x with ICS-43434 microphone. <br>

	 See README.md under Firmware/Teensy4.0/V5 for details.

### April 2024
 - New NodeMCU firmware NRZ-2020-134-DNMS-5.6 with:
	 - DNMS correction value bug fix (issue #54)
	 - LED status display
	 - modified memory organisation<br>
	 
	See README.md under Firmware/NodeMCU for details.
	

### February 2024
- Improved Teensy 4.0 firmware for ICS-43434 and IM72D128 microphones with improved internal timing (upgrade recommended):
     - DNMS_V5.2.4 - for ICS43434 with Teensy4.0
     - DNMS_V5.2.6 - for IM72D128 with Teensy4.0
 	
- Standalone testprograms for Teensy4.0 and microphones ICS-43434 and IM72D128 with improved internal timing and changed input and output at startup, measuring interval can be set from 125ms upwards, output of 1/3 octave values is possible from 500ms upwards:
 	- DNMS_Vt.7.4 - for ICS43434 with Teensy4.0
 	- DNMS_Vt.7.6 - for IM72D128 with Teensy4.0
  
- dnms-0.9.12 First version of the Raspberry Pi application to read out the DNMS sensor and send data to Sensor.Community and/or an InfluxDB as an alternative to a NodeMCU (Airrohr board). The Raspberry Pi application supports two different measurement intervals with different configurable measurement values for sending to an InfluxDB. The values of the 2nd measurement interval are only transmitted when a limit value (LAeq) is exceeded.
- KiCad layouts adapted to KiCad version 7 for the following PCBs (no change to the layout itself):

  + DNMS-T4.0-V1.7
  + DNMS-RPI-HAT-V1.0
  + IM72D128-PCB-V1.0
  + IM72D128-Tube-PCB-V1.0

### September 2023
- New NodeMCU firmware NRZ-2020-134-DNMS-5.4, also available as English and French version:
    - Improved output of transmission errors for communication with Sensor.Community and an InfluxDB.
    - Larger buffer for transmissions, so in addition to transmitting all DNMS readings, you can also transmit readings from other sensors.
    - Changed issue reporting link to https://github.com/hbitter/DNMS/issues

### August 2023
 - Standalone test programs for Teensy4.0 and microphones ICS-43434 and IM72D128:
 	- DNMS_Vt.5.4 - for ICS43434 and Teensy4.0
 	- DNMS_Vt.5.6 - for IM72D128 and Teensy4.0
 
	These test programs are based on Teensy4.0 firmware version 5. In addition to a simple test to see if the microphone and Teensy4.0 are working, a suitable terminal program can be used to store the measured values for future use. It is possible to input the measurement interval.
 	
 - Airrohr-ESP32-PCB-V1.1 PCB deleted - obsolete, because no firmware support.	

### July 2023
- Support of IM72D128 microphone from Infineon. IM72D128 microphone does have two significant advantages:
   - IP57 water and dust resistant on component level (however a good windshield is recommended).

   - Signal to noise ratio of 72 dB(A) SNR, this lowers the measurement range down to about 23 dB(A)

   - PCBs for IM72D128 microphone. A round PCB for the microphone itself and a longish PCB for the additional circuitry to convert the PDM interface of the IM72D128 to I²S. Therfore it is compatible to the I²S interface of the ICS-43434 used up to now.

 - Teensy4.0 firmware V5, which does now in addition calculation and output of Z-values (LZeq, LZmin, LZmax as well as LZeq values of 1/3 octaves from 20Hz - 20.000Hz). For the ICS-43434 microphone and the newly supported IMD128 microphone there are two different firmware versions:
    - DNMS_V5.0.4 for the ICS-43434 microphone
    - DNMS_V5.0.6 for the IM72D128 microphone
 
 - New NodeMCU firmware versions NRZ-2020-134-DNMS-5.2 and NRZ-2020-134-DNMS-5.2-en supporting Z-values. These versions are compatible with all Teensy firmware versions (V2, V3, V4 and V5) and replaces the older NodeMCU firmware versions.
 
   README.md under NodeMCU gives more details<br><br>



### March 2023

 - New NodeMCU firmware versions NRZ-2020-134-DNMS-4.4 and NRZ-2020-134-DNMS-4.4-en (replaces as well versions NRZ-2020-134-DNMS-3.3 and NRZ-2020-134-DNMS-3.3-en):
 
   - Instabilities of local NodeMCU webserver settled
   - SDS011 can be used again with this versions
   - Extended status output of some sensors (DNMS, SEN5X, SCD30, SCD40)
   - Support of further sensors (SEN5X, SCD30, SCD40)
 
 
 	README.md under NodeMCU gives more details
   <br><br>

### January 2023

- Correction of a bug in Teensy3.6 firmware, the new versions DNMS_V2.5.0 and DNMS_V2.5.1 correcting a wrong initialization at startup.
- Correction of NodeMCU firmware, the new versions NRZ-2020-134-DNMS-3.3, NRZ-2020-134-DNMS-3.3-en, NRZ-2020-134-DNMS-4.3 and NRZ-2020-134-DNMS-4.3-en correcting the I²C clock frequency, it is now closer to 100kHz.<br><br>

- There are now three different versions of the Teensy firmware, which differ in functionality:
	- V2 - with the functionality as up to now e.g. calculation of LAeq, LAmin and LAmax
	- V3 (only for Teensy4.0) - in addition to V2 supports frequency spectrum and calculates 1/3 octave values from 20Hz - 20000Hz. 1/3 octave values are not transmitted to Sensor.Community. They are available on the local webpage of the NodeMCU and can be transmitted to an InfluxDB. To use these features of Teensy V3 version a corresponding firmware version for the NodeMCU has to be used (at the moment that is NRZ-2020-134-DNMS-3.2 or the english version NRZ-2020-134-DNMS-3.2/EN).
	- V4 (only for Teensy4.0) - in addition to V3 functionality a 2nd measurement interval is realized. The 2nd measurement interval is independent from the 1st measurement interval and can be anything from one second on. The values from the 2nd measurement interval are not transmitted to Sensor.Community but to other APIs as e.g. an InfluxDB. A threshold can be configured. Exceeding the threshold will start transmission of values from the 2nd measurement for a configurable amount of time to an InfluxDB. To use these features of Teensy V4 version a corresponding firmware version for the NodeMCU has to be used (at the moment that is NRZ-2020-134-DNMS-4.2 or the english version NRZ-2020-134-DNMS-4.2/EN).

- Change of time weighting in all current Teensy firmware versions from  I (Impuls) to F (Fast - 125ms). This will result in a bit lower LAmax and LAeq values and a bit higher LAmin values. It compares better to the standard F (Fast) time weighting.

- Teensy firmware versions V2, V3 and V4 are downward compatible i.e. Tennsy4.0 with firmware V4 can operate together with a NodeMCU running a firmware supporting only functionality V2, only functionality from V2 is available then. The other way round is possible as well. NodeMCU with e.g. firmware NRZ-2020-134-DNMS-4.2 can operate together with a Teensy running firmware V2. Only functionality from V2 this supported then.

- If there are communication problems between Teensy and standard NodeMCU firmware from Sensor.Community, please use the current NodeMCU firmware (e.g. NRZ-2020-134-DNMS-3.2 or NRZ-2020-134-DNMS-4.2) from this site.

- changed PCBs:
	- ICS-43434-PCB-V1.4 - diameter of acoustic port changed from 0,6mm to 0,7mm
	
	- DNMS-T4.0-V1.7 - additional connectors for microphone connection
	
- Software for using a Raspberry Pi as communication processor to transmit DNMS data to Sensor.Community and/or to an InfluxDB:
	- 1st version dnms-0.9.5 for Raspi Pi Zero W, Raspi Pi Zero 2 W and others






