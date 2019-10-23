# DNMS


DNMS (Digital Noise Measurement Sensor) ist ein Sensor-Modul zur Schallpegelmessung. Es wird neben dem LAeq-Wert über einem Zeitintervall auch der min. und max. LA-Wert in diesem Zeitintervall festgehalten. Das zentrale Element des DNMS ist ein digitales MEMS-Mikrofon (InvenSense ICS-43434). Ein Mikrocontroller-Board (Teensy 3.6 / Teensy 4.0) übernimmt die digitalen Daten vom Mikrofon über die I²S Schnittstelle als  16-Bit, 44,1 kHz PCM Daten.  Die Audiodaten werden über einen digitalen A-Filter bewertet, dann wird der Effektivwert bestimmt und daraus der Schalldruckpegel entsprechend der Mikrofonsensitivität berechnet. Die einzelnen Schalldruckpegelwerte werden zu einem fortlaufenden LAeq-Wert weitergerechnet. 

Die Werte werden dann über eine I²C-Schnittstelle an einen abfragenden Mikrocontroller zur Weiterverarbeitung übertragen. Das Abfrage-Intervall kann 1 Sekunde betragen oder auch bis zu 3600 Sekunden. Bei der weiteren Rechnung mit den LAeq-Werten ist natürlich zu beachten, dass äquidistante Zeitintervalle Voraussetzung sind, um einen gesamt LAeq-Wert über einen längeren Zeitraum (z.B. Stunde oder Tag) zu bilden. 

