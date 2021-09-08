
## English text below

## DNMS (Digital Noise Measurement Sensor)

Ein Sensor-Modul zur Schallpegelmessung. Es wird neben dem LAeq-Wert über einem Zeitintervall auch der min. und max. LA-Wert in diesem Zeitintervall festgehalten. Das zentrale Element des DNMS ist ein digitales MEMS-Mikrofon (InvenSense ICS-43434). Ein Mikrocontroller-Board (Teensy3.6 / Teensy4.0) übernimmt die digitalen Daten vom Mikrofon über die I²S Schnittstelle als  24-Bit, 44,1 kHz PCM Daten (ab Firmware Version 2 - zuvor nur 16-Bit).  Die Audiodaten werden über einen digitalen A-Filter bewertet, dann wird der Effektivwert bestimmt und daraus der Schalldruckpegel entsprechend der Mikrofonsensitivität berechnet. Die einzelnen Schalldruckpegelwerte werden zu einem fortlaufenden LAeq-Wert weitergerechnet. Ebenso werden der max. und min. Schalldruckpegel (LAmax, LAmin) aus dem Intervall abgespeichert.

Die Werte werden dann über eine I²C-Schnittstelle an einen abfragenden Mikrocontroller zur Weiterverarbeitung übertragen. Das Abfrage-Intervall kann 1 Sekunde betragen oder auch bis zu 3600 Sekunden.

Geplante Erweiterungen sind zusätzlich zur A-Bewertung eine C-Bewertung d.h. Bildung von LCeq, LCmax und LCmin, sowie eine FFT zur weiteren Analyse im Frequenzbereich.

## PCBs und Teile
-  Wer bei dem Lärmprojekt mitmachen will und Hilfe bei der Beschaffung benötigt, kann bei Franz Hoefle <franz.hoefle (at) buergerforum-gladbeck.de> anfragen, ob Platinen (auch bestückt) bzw. fertige Teile verfügbar sind  - s.a. ebay-Kleinanzeigen Suchwort <DNMS>.
 
 
## Neuigkeiten

### Achtung
Bei den Versionen DNMS-T4.0+NodeMCU-V1.x führt ein gleichzeitger Betrieb von DNMS und dem Feinstaubsensor SPS30 z.Zt. zu Problemen. Es gibt fehlerhafte DNMS Lärm Werte oder der gemeinsame Betrieb ist überhaupt nicht möglich. Die Ursache muss noch geklärt werden. Soll ein SPS30 eingesetzt werden ist der Workaround bis zur Klärung: Ein getrenntes Airrohr PCB und DNMS-T4.0 PCB benutzen.
Bitte in diesen Fällen Firmware Version 2.1.4 oder 2.1.5 probieren. Eine Rückmeldung, ob die Probleme damit behoben sind, wäre schön.

### September 2021
- Die Beta Firmware Version 2.1.4 und 2.1.5 versucht das I²C Kommunikation bei den Kombi Boards DNMS-T4.0+NodeMCU-V1.x zu beheben. Bitte probieren, wenn ein solches Problem aufgetreten ist. Eine Rückmeldung, ob es dann funktioniert oder nicht, wäre schön.

### Mai 2021
- Die Firmware Version 2 ist nun zusammen mit der HFT (Hochschule für Technik, Stuttgart) im Akustik Labor überprüft worden mit gutem Ergebnis. Der detaillierte Bericht darüber ist unter Firmware zu finden.

### März 2021
- Erstes Release der Firmware Version 2 basierend auf 32-Bit Floating Point Verarbeitung für Teensy4.0 und Teensy3.6. Der untere Grenze des LAeq Messbereichs liegt mit dieser Version bei ca. 30 dBA beim Teensy4.0 und ca. 32 dBA beim Teensy3.6. Dies ist eine Verbesserung von 5 - 6 dBA im Vergleich zur Firmware Version 1. Einfach die entsprechende Firmware auf Teensy3.6 oder Teensy4.0 flashen. Die endgültigen Messungen und Auswertungen zu dieser Version erfolgen noch im Laufe des April. Dann werden wir auch die Messwerte veröffentlichen.

### Januar 2021
- Neues Layout DNMS-T4.0+NodeMCU-V1.5 - Masseflächen an Ober- und Unterseite des Boards um die Abschirmung zwischen Teensy4.0 und dem WLAN der NodeMCU zu verbessern

### Dezember 2020
- Als Grundlage für die weitere Entwicklung erfolgt zur Zeit eine Umstellung von 16-Bit Integer Verarbeitung auf 32-Bit Floating Point. Teensy3.6 /4.0 sind dafür leistungsfähig genug, da sie über eine HW Floating Point Einheit verfügen, Teensy 4.0 sogar für 64-Bit Floating Point. Die volle 24-Bit Auflösung des Mikrofons wird dann auch ausgenutzt, was einen unteren Messbereich für den LAeq von ca. 31 dBA ermöglicht. Für Teensy3.6 ist diese Umstellung schon lauffähig. Die Filter für die A-Bewertung und Frequenzgangkorrektur des Mikrofons müssen aufgrund der Umstellung noch angepasst werden. Dies soll im Zeitraum Januar/Februar 2021 erfolgen.
- Auf der neuen Basis wird danach das Thema Frequenzanalyse mittels FFT angegangen, was einige Zeit dauern wird.

### Oktober 2020
- Kleine Korrekturen und Ergänzungen im Manual, Stand nun V2.2
- Neues Manual V2.0 verfügbar mit detaillierten Beschreibungen der V1.4 Boards
- Stücklisten/BOM für die V1.4 Boards (Airrohr-PCB-V1.4, DNMS-T4.0-V1.4, DNMS-T4.0+NodeMCU-V1.4)

### August 2020: Neue PCBs verfügbar
- Airrohr-PCB-V1.4
- DNMS-T3.6-V1.4
- DNMS-T4.0-V1.4
- DNMS-T4.0+NodeMCU-V1.4

### Juni 2020: Teensy4.0 läuft stabil und wird nun empfohlen
- Teensy4.0 ist preiswerter als Teensy3.6
- Teensy4.0 ist leistungsfähiger als Teensy3.6
- Teensy3.6 wird weiter unterstützt




----------------------------------------------------------------------------------------------------------------
## DNMS (Digital Noise Measurement Sensor)

A sensor modul for SPL measurement. Over a period of time the LAeq value is calculated, beside that the minimal and the maximal LA value is covered over the time period. The central element of DNMS is a digital MEMS-Microphone (InvenSense ICS-43434). The digital data is fed from the microphone via a I²S connection as 24-bit, 44,1 kHz PCM data (from firmware version 2 on - before it was only 16-Bit) to a microcontroller board (Teensy3.6 / Teensy4.0). The data is filtered by an A-filter, then the RMS value is calculated. Further the sound pressure level (SPL) is calculated based on microphone sensivity. The SPL values are integrated over time to give the LAeq. Also the maximum level in the measurement period LAmax and the minimum level LAmin will be kept.

All measurements will be transfered to a further microcontroller via I²C bus. The intervall time to pick up measurements can reach from 1 second up to 3600 seconds.

Planned extensions are an additional C-filter to get LCeq, LCmax and LCmin and further FFT for analysis in the frequency domain.

## PCBs and parts
- Anybody who will participate and need some help for buying parts can contact Franz Hoefle <franz.hoefle (at) buergerforum-gladbeck.de> to get information about available PCBs (assembled or not) and ready made parts of the system - have a look at ebay classifieds for <DNMS>.


## News

### Attention
With PCB versions DNMS-T4.0+NodeMCU-V1.x there is at the moment no simultaneous operation of DNMS and PM sensor SPS30 possible. Either wrong DNMS noise measurements readings occur or the simultaneous operation of DNMS and SPS30 is not possible at all. The cause has to be investigated. If you like to use a SPS30 PM sensor the workaround is: Use separated Airrohr PCB and DNMS-T4.0 PCB instead.
Please try the beta firmware 2.1.4 or 2.1.5 in these cases. Feedback about the result would be appreciated.

### September 2021
- Beta firmware versions 2.1.4 and 2.1.5 tries to fix this I²C communication problem if you are using a combined board DNMS-T4.0+NodeMCU-V1.x. Please try the new beta firmware, if you had such problems. Feedback about the result would be appreciated.

### May 2021
- Firmware version 2 was checked togehter with the HFT (Hochschule für Technik - University of Applied Sciences, Stuttgart) in the acoustic laboratories of the Centre for Building Physics (ZFB). You will find a detailed description about the tests under firmware.

### March 2021
- First release of firmware version 2, which is based on 32-Bit floating point processing. It's available for Teensy3.6 and Teensy4.0. The lower LAeq measurement limit for version 2 is araund 30 dBA for Teensy4.0 and around 32 dBA for Teensy3.6. This is an improvement of about 5 - 6 dBA compared to firmware version 1. To upgrade to version 2 you only need to flash the appropriate firmware on Teensy3.6 or Teensy4.0. The definite test and reporting will be done over April an the results will be published then. 

### January 2021
- New layout of DNMS-T4.0+NodeMCU-V1.5 - ground-planes on both sides of the board to improve shielding between Teensy4.0 and WLAN of NodeMCU

### December 2020
- As a new basis processing of the audio signal will be changed from 16-Bit integer to 32-Bit floating point. Teensy 3.6 /4.0 are very capable, their processors have 32-Bit floating point in HW, Teensy 4.0 even 64-Bit. The full 24-Bit resolution of the microphone will be used as well, this will result in a lower limit of the measurement range for LAeq of around 31 dBA. For Teensy3.6 these changes are done. Still the filters for A-weighting and correction of the microphone frequency response needs some adaption. This work should happen in january/february 2021.
- On the new basis the next step will be frequency analysis based on FFT, which will take some more time.

### October 2020
- Small corrections and additions, now it is V2.2
- New Manual V2.0 (right now only in german) with detailed description of V1.4 boards
- BOM for V1.4 Boards (Airrohr-PCB-V1.4, DNMS-T4.0-V1.4, DNMS-T4.0+NodeMCU-V1.4)

### August 2020: New PCBs are available
- Airrohr-PCB-V1.4
- DNMS-T3.6-V1.4
- DNMS-T4.0-V1.4
- DNMS-T4.0+NodeMCU-V1.4

### June 2020: Teensy4.0 is stable and now recommended
- Teensy4.0 is cheaper than Teensy3.6
- Teensy4.0 has a lot more power than Teensy3.6
- Teensy3.6 will be further supported




