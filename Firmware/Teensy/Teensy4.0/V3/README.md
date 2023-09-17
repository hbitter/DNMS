# [English text below](#dnms-firmware-teensy40-v3-english)

# DNMS Firmware Teensy4.0 V3

## Neben der Berechnung von LAeq, LAmin und LAmax erfolgt zusätzlich die Berechnung der Terzwerte von 20Hz - 20000Hz mittels FFT

### aktuelle Version:

- DNMS_V3.4.4 - ohne USB-Audio Ausgabe


### Voraussetzungen

- Benutzung der Firmware Versionen NRZ-2020-134-DNMS-3.2 oder NRZ-2020-134-DNMS-4.2 auf der NodeMCU damit die Terzwerte abgefragt werden. Die Anwendung dnms-0.9.5 für den Raspberry Pi bietet ebenfalls die Möglichkeit die Terzwerte abzufragen und an eine InfluxDB weiterzugeben.
- Die Terzwerte werden z.Zt. nicht bei Sensor.Community gespeichert, sollen sie gespeichert werden ist der Einsatz einer InfluxDB oder einer eigen Anwendung notwendig.


------------------------------------------------------------------------


# DNMS Firmware Teensy4.0 V3 English

## Besides the calculation of LAeq, LAmin and LAmax there is a calculation of 1/3 octave values from 20Hz to 20000Hz based on FFT

### Current version:

- DNMS_V3.4.4 - without USB-Audio output

### Prerequisites

- Usage of firmware versions NRZ-2020-134-DNMS-3.2 or NRZ-2020-134-DNMS-4.2for NodeMCU to get the 1/3 octave values read out of Teensy4.0. The application dnms-0.9.5 for the Rspberry Pi reads the 1/3 octave values as well and transmits the values to an InfluxDB.
- 1/3 octave values are not kept by Sensor.Community. It is necessary to use an InfluxDB or an own application for storing the 1/3 octave values.

