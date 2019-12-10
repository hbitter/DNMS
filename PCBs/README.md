# PCBs


## Funktion der verschiedenen PCBs

### Airrohr PCB
Basis Board für eine NodeMCU V2 od. V3 mit Anschlüssen für die folgenden Sensoren:
- Feinstaubsensor SD011 über Steckverbinder
- Feinstaubsensor SPS30 (Sensirion) über Steckverbinder
- DHT22 Temperatur- Feuchtigkeitssensor über Stiftleiste
- I²C Anschlüsse mit 3,3V und 5V Versorgungsspannungs für DNMS und andere Sensoren (z.B. BMP/E280 Temperatur- Feuchtigkeitssensor) über Stiftleiste
- optionale I²C-Bus-Verlängerung mit 5V Versorgungsspannung für entfernten DNMS Sensor (bei Verbindungen > 250mm) über Stiftleiste

PCB Historie:
- Airrohr V1.1 - erste freigegebene Version
- Airrohr V1.2 - I²C-Bus-Verlängerung mit zusätzlicher RJ-12 (Modular Jack) 6P/6C Buchse

### DNMS - T3.6+NodeMCU
Kombiniertes Board für DNMS mit Teensy3.6 und NodeMCU V2 od. V3, neben der DNMS Funktion sind noch Anschlüsse für die die folgenden Sensoren vorhanden:
- Feinstaubsensor SPS30 (Sensirion) über Steckverbiner
- I²C Anschluss mit 3,3V Versorgungsspannung für z.B. BMP/E280 Temperatur- Feuchtigkeitssensor od. SHT3X Sensor über Stiftleiste

PCB Historie:
- DNMS - T3.6+NodeMCU V1.0 - erste freigegebene Version

### DNMS - T3.6
Basis Board für Teensy3.6 zum Anschluss ICS-43434 Mikrofon, die Lärmwerte werden über I²C an einen Kommunikationsprozessor (z.B. Airrohr V1.1 od. V1.2 mit NodeMCU) übertragen, für Entfernungen zum Kommunikationsprozessor > 250mm ist eine optionale I²C-Bus-Verlängerung vorhanden.

PCB Historie:
- DNMS - T3.6 V1.1 - erste freigegebene Version
- DNMS - T3.6 V1.2 - I²C-Bus-Verlängerung mit zusätzlicher RJ-12 (Modular Jack) 6P/6C Buchse (passend zum Airrohr V1.2)

### DNMS - T4+NodeMCU

### DNMS - T4


----------------------------------------------------------------------------------------------------------------

## use of different PCBs

### Airrohr PCBs

### DNMS - T3.6+NodeMCU

### DNMS - T3.6

### DNMS - T4+NodeMCU

### DNMS - T4


