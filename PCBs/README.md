# PCBs


## Funktion der verschiedenen PCBs

### Airrohr PCB
Basis Board für eine NodeMCU V2 od. V3 mit Anschlüssen für die folgenden Sensoren:
- Feinstaubsensor SDS011 über Steckverbinder
- Feinstaubsensor SPS30 (Sensirion) über Steckverbinder
- DHT22 Temperatur- Feuchtigkeitssensor über Stiftleiste
- I²C Anschlüsse mit 3,3V und 5V Versorgungsspannungs für DNMS und andere Sensoren (z.B. BMP/E280 Temperatur- Feuchtigkeitssensor) über Stiftleiste
- I²C-Bus-Verlängerung mit 5V Versorgungsspannung für entfernten DNMS Sensor (bei Verbindungen > 250mm) über RJ12 Buchse bzw. Stiftleiste

PCB Versions Historie:
- Airrohr-V1.1 - erste Version
- Airrohr-V1.2 - I²C-Bus Verlängerung mit zusätzlicher RJ-12 (Modular Jack) 6P/6C Buchse
- Airrohr-V1.4 - RJ12 Buchse nun für zwei unterschiedliche "Footprints" und in der Ausrichtung geändert 
			   - einige Bauelemente nach innen verschoben, damit der Einbau in ein Gehäuse (Rohr) einfacher ist 
			   - P82B715 (I²C-bus Extender IC) kann im DIP8 Gehäuse oder im SO8 Gehäuse bestückt werden

### DNMS-T3.6+NodeMCU
Kombiniertes Board für DNMS mit Teensy3.6 und NodeMCU V2 od. V3, neben der DNMS Funktion sind noch Anschlüsse für die die folgenden Sensoren vorhanden:
- Feinstaubsensor SPS30 (Sensirion) über Steckverbiner
- I²C Anschluss mit 3,3V Versorgungsspannung für z.B. BMP/E280 Temperatur- Feuchtigkeitssensor od. SHT3X Sensor über Stiftleiste

PCB Versions Historie:
- DNMS-T3.6+NodeMCU V1.0 - erste Version

### DNMS-T3.6
Basis Board für Teensy3.6 zum Anschluss ICS-43434 Mikrofon, die Lärmwerte werden über I²C an einen Kommunikationsprozessor (z.B. Airrohr-V1.4 mit NodeMCU) übertragen, für Entfernungen zum Kommunikationsprozessor > 250mm ist eine optionale I²C-Bus-Verlängerung vorhanden. Ein standard Telefonkabel mit RJ12 Verbindern (6P/6C) dient dann zur Verbindung des DNMS mit dem Airrohr Board.

PCB Versions Historie:
- DNMS-T3.6-V1.1 - erste Version
- DNMS-T3.6-V1.2 - I²C-Bus-Verlängerung mit zusätzlicher RJ-12 (Modular Jack) 6P/6C Buchse (passend zum Airrohr-V1.2)
- DNMS-T3.6-V1.3 - P82B715 (I²C-bus Extender IC) kann im DIP8 Gehäuse oder im SO8 Gehäuse bestückt werden
- DNMS-T3.6-V1.4 - RJ12 Buchse nun für zwei unterschiedliche "Footprints"

### DNMS-T4.0+NodeMCU
Kombiniertes Board für DNMS mit Teensy4.0 und NodeMCU V2 od. V3, neben der DNMS Funktion sind noch Anschlüsse für die die folgenden Sensoren vorhanden:
- Feinstaubsensor SPS30 (Sensirion) über Steckverbiner
- I²C Anschluss mit 3,3V Versorgungsspannung für z.B. BMP/E280 Temperatur- Feuchtigkeitssensor od. SHT3X Sensor über Stiftleiste

PCB Versions Historie:
- DNMS-T4.0+NodeMCU V1.2 - erste veröffentlichte Version
- DNMS-T4.0+NodeMCU V1.4 - einige Bauelemente wurden nach innen verschoben, damit der Einbau ins Gehäuse einfacher wird


### DNMS-T4.0
Basis Board für Teensy4.0 zum Anschluss ICS-43434 Mikrofon, die Lärmwerte werden über I²C an einen Kommunikationsprozessor (z.B. Airrohr-V1.4 mit NodeMCU) übertragen, für Entfernungen zum Kommunikationsprozessor > 250mm ist eine optionale I²C-Bus-Verlängerung vorhanden. Ein standard Telefonkabel mit RJ12 Verbindern (6P/6C) dient dann zur Verbindung des DNMS mit dem Airrohr Board.

PCB Versions Historie:
- DNMS-T4.0-V1.2 - erste Version
- DNMS-T4.0-V1.4 - P82B715 (I²C-bus Extender IC) kann im DIP8 Gehäuse oder im SO8 Gehäuse bestückt werden
				 - RJ12 Buchse für I²C-Bus-Verlängerung mit zwei unterschiedlichen "Footprints"

----------------------------------------------------------------------------------------------------------------

## use of different PCBs

### Airrohr PCB
Basis board for NodeMCU V2 or V3 with possible connection of the following sensors:
- PM sensor SDS011 via connector
- PM sensor SPS30 (Sensirion) via connector
- DHT22 temperatur and humidity sensor via pin header
- I²C connection with 3.3V and 5V power for DNMS and other sensors (i.e. BMP/E280 or SHT3x) via pin header
- I²C-bus-extension with 5V power for DNMS sensor with connection lenght > 250mm via RJ12 connector or pin header

PCB revision history:
- Airrohr-V1.1 - first version
- Airrohr-V1.2 - I²C-bus-extension with RJ12 connector (Modular Jack 6P/6C) or pin header
- Airrohr-V1.4 - support for two different footprints for RJ12 connector, placement of RJ12 connector to different side
			   - placement of some components more to the inner side, so that everything fits better in a case (i.e. tube)
			   - P82B715 (I²C-bus Extender IC) layout allows assembly of DIP8 or SO8 package


### DNMS-T3.6+NodeMCU
This is a combination of a NodeMCU and a Teensy3.6, besides the DNMS functionality the following sensors could be connected:
- SPS30 PM sensor via connector
- I²C connection with 3.3V power for sensors like BMP/E280 or SHT3x via pin header


PCB revision history:
- DNMS-T3.6+NodeMCU V1.0 - first version

### DNMS-T3.6
DNMS board for Teensy3.6 to connect a ICS-43434 microphone, the noise measurements are transmitted via the I²C-bus to a communication processor (i.e. Airrohr-V1.4 with NodeMCU), if the distance to the communication processor exceeds 250mm then the I²C-bus-extension with RJ12 connector (Modular Jack 6P/6C) can be used. A standard telephone cable with RJ12 jacks on both sides (6P/6C) with up to 5m length or more provides the connection of the DNMS and the Airrohr board.

PCB revision history:
- DNMS-T3.6-V1.1 - first version
- DNMS-T3.6-V1.2 - I²C-bus-extension with RJ12 connector (Modular Jack 6P/6C) (fits Airrohr-V1.2)
- DNMS-T3.6-V1.3 - P82B715 (I²C-bus Extender IC) layout allows assembly of DIP8 or SO8 package
- DNMS-T3.6-V1.4 - RJ12 connector now for two different footprints

### DNMS-T4.0+NodeMCU
This is a combination of a NodeMCU and a Teensy4.0, besides the DNMS functionality the following sensors could be connected:
- SPS30 PM sensor via connector
- I²C connection with 3.3V power for sensors like BMP/E280 or SHT3x via pin header

PCB revision history:
- DNMS-T4.0+NodeMCU V1.2 - first version
- DNMS-T4.0+NodeMCU V1.4 - some components were moved towards the inner side, so that everything fits better in a case (i.e. tube)


### DNMS-T4.0
DNMS board for Teensy4.0 to connect a ICS-43434 microphone, the noise measurements are transmitted via the I²C-bus to a communication processor (i.e. Airrohr-V1.4 with NodeMCU), if the distance to the communication processor exceeds 250mm then the I²C-bus-extension with RJ12 connector (Modular Jack 6P/6C) can be used. A standard telephone cable with RJ12 jacks on both sides (6P/6C) with up to 5m length or more provides the connection of the DNMS and the Airrohr board.

PCB revision history:
- DNMS-T4.0-V1.2 - first version
- DNMS-T4.0-V1.4 - P82B715 (I²C-bus Extender IC) layout allows assembly of DIP8 or SO8 package
				 - support for two different footprints for RJ12 connector


