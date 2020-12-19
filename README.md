# DNMS (Digital Noise Measurement Sensor)

Ein Sensor-Modul zur Schallpegelmessung. Es wird neben dem LAeq-Wert über einem Zeitintervall auch der min. und max. LA-Wert in diesem Zeitintervall festgehalten. Das zentrale Element des DNMS ist ein digitales MEMS-Mikrofon (InvenSense ICS-43434). Ein Mikrocontroller-Board (Teensy3.6 / Teensy4.0) übernimmt die digitalen Daten vom Mikrofon über die I²S Schnittstelle als  16-Bit, 44,1 kHz PCM Daten.  Die Audiodaten werden über einen digitalen A-Filter bewertet, dann wird der Effektivwert bestimmt und daraus der Schalldruckpegel entsprechend der Mikrofonsensitivität berechnet. Die einzelnen Schalldruckpegelwerte werden zu einem fortlaufenden LAeq-Wert weitergerechnet. Ebenso werden der max. und min. Schalldruckpegel (LAmax, LAmin) aus dem Intervall abgespeichert.

Die Werte werden dann über eine I²C-Schnittstelle an einen abfragenden Mikrocontroller zur Weiterverarbeitung übertragen. Das Abfrage-Intervall kann 1 Sekunde betragen oder auch bis zu 3600 Sekunden.

Geplante Erweiterungen sind zusätzlich zur A-Bewertung eine C-Bewertung d.h. Bildung von LCeq, LCmax und LCmin, sowie eine FFT zur weiteren Analyse im Frequenzbereich.

## PCBs und Teile
-  Wer bei dem Lärmprojekt mitmachen will und Hilfe bei der Beschaffung benötigt, kann bei Franz Hoefle <franz.hoefle (at) buergerforum-gladbeck.de> anfragen, ob Platinen (auch bestückt) bzw. fertige Teile verfügbar sind  - s.a. ebay-Kleinanzeigen Suchwort <DNMS>.
 
 
## Neuigkeiten

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


A sensor modul for SPL measurement. Over a period of time the LAeq value is calculated, beside that the minimal and the maximal LA value is covered over the time period. The central element of DNMS is a digital MEMS-Microphone (InvenSense ICS-43434). The digital data is fed from the microphone via a I²S connection as 16-bit, 44,1 kHz PCM data to a microcontroller board (Teensy3.6 / Teensy4.0). The data is filtered by an A-filter, then the RMS value is calculated. Further the sound pressure level (SPL) is calculated based on microphone sensivity. The SPL values are integrated over time to give the LAeq. Also the maximum level in the measurement period LAmax and the minimum level LAmin will be kept.

All measurements will be transfered to a further microcontroller via I²C bus. The intervall time to pick up measurements can reach from 1 second up to 3600 seconds.

Planned extensions are an additional C-filter to get LCeq, LCmax and LCmin and further FFT for analysis in the frequency domain.

## PCBs and parts
- Anybody who will participate and need some help for buying parts can contact Franz Hoefle <franz.hoefle (at) buergerforum-gladbeck.de> to get information about available PCBs (assembled or not) and ready made parts of the system - have a look at ebay classifieds for <DNMS>.


## News

### December 2020
- As a new basis processing of the audio signal will be changed from 16-Bit integer to 32-Bit floating point. Teensy 3.6 /4.0 are very capable, their processors have 32-Bit floating point in HW, Teensy 4.0 even 64-Bit. The full 24-Bit resolution of the microphone will be used as well, this will result in a lower limit of the measurement range for LAeq of around 31 dBA. For Teensy3.6 these changes are done. Still the filters for A-weighting and correction of the microphone frequency response needs some adaption. This work should happen in the january/february 2021.
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





