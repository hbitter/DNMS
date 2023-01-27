
## English text below

## DNMS (Digital Noise Measurement Sensor)

Ein Sensor-Modul zur Schallpegelmessung. Es wird neben dem LAeq-Wert über einem Zeitintervall auch der min. und max. LA-Wert in diesem Zeitintervall festgehalten. Das zentrale Element des DNMS ist ein digitales MEMS-Mikrofon (InvenSense ICS-43434). Ein Mikrocontroller-Board (Teensy3.6 / Teensy4.0) übernimmt die digitalen Daten vom Mikrofon über die I²S Schnittstelle als  24-Bit, 44,1 kHz PCM Daten (ab Firmware Version 2 - zuvor nur 16-Bit).  Die Audiodaten werden über einen digitalen A-Filter bewertet, dann wird der Effektivwert bestimmt und daraus der Schalldruckpegel entsprechend der Mikrofonsensitivität berechnet. Die einzelnen Schalldruckpegelwerte werden zu einem fortlaufenden LAeq-Wert weitergerechnet. Ebenso werden der max. und min. Schalldruckpegel (LAmax, LAmin) aus dem Intervall abgespeichert.

Die Werte werden dann über eine I²C-Schnittstelle an einen abfragenden Mikrocontroller zur Weiterverarbeitung übertragen. Das Abfrage-Intervall kann 1 Sekunde betragen oder auch bis zu 3600 Sekunden.

Eine Version mit der Möglichkeit das Frequenzspektrum zu betrachten ist seit einiger Zeit verfügbar. Terzwerte von 20Hz - 20000Hz können ausgegeben werden. Terzwerte werden z.Zt. nicht von Sensor.Community unterstützt, deshalb gibt es spezielle Versionen der NodeMCU Firmware um Terzwerte auf der lokalen Webseite der NodeMCU auszugeben und an weitere APIs wie z.B. zu einer InfluxDB zu senden.

Weitere Entwicklungen wie die Berechnung und Ausgabe von Z-Werten und C-Werten sind in der Überlegung.
 
## Neuigkeiten

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
## DNMS (Digital Noise Measurement Sensor)

A sensor modul for SPL measurement. Over a period of time the LAeq value is calculated, beside that the minimal and the maximal LA value is covered over the time period. The central element of DNMS is a digital MEMS-Microphone (InvenSense ICS-43434). The digital data is fed from the microphone via a I²S connection as 24-bit, 44,1 kHz PCM data (from firmware version 2 on - before it was only 16-Bit) to a microcontroller board (Teensy3.6 / Teensy4.0). The data is filtered by an A-filter, then the RMS value is calculated. Further the sound pressure level (SPL) is calculated based on microphone sensivity. The SPL values are integrated over time to give the LAeq. Also the maximum level in the measurement period LAmax and the minimum level LAmin will be kept.

All measurements will be transfered to a further microcontroller via I²C bus. The intervall time to pick up measurements can reach from 1 second up to 3600 seconds.

A version with support of frequency spectrum is available for some time. 1/3 octave values from 20Hz - 20000Hz are calculated. 1/3 octave values are not supported by Sensor.Community so far. Therefore a special version of the NodeMCU firmware is available, showing the 1/3 octave values on the local webpage of the NodeMCU as well as transmitting the 1/3 octave values to an InfluxDB.

Further development for generating Z-values and C-values are under consideration.

## News

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






