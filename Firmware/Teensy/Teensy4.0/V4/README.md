# [English text below](#dnms-firmware-teensy40-v4-english)

# DNMS Firmware Teensy4.0 V4

## V4 realisiert neben der Funktionalität von V3 ein 2. Messintervall 

### aktuelle Version:

- DNMS_V4.4.4 - ohne USB-Audio Ausgabe


### Voraussetzungen

- Benutzung der Firmware Versionen NRZ-2020-134-DNMS-4.2 auf der NodeMCU damit das 2. Messintervall konfiguriert und abgefragt wird. Die Anwendung dnms-0.9.5 für den Raspberry Pi bietet ebenfalls die Möglichkeit das 2. Messintervall zu konfigurieren und abzufragen und die Werte an eine InfluxDB weiterzugeben. Das 2. Messintervall liefert neben LAeq, LAmin und LAmax auch die Terzwerte des 2. Messintervalls. Die Werte des 2. Messintervalls werden nicht an Sensor.Community übertragen.
- Benutzung einer InfluxDB oder eigenen Anwendung um die Werte des 2. Messintervalls abzuspeichern.



------------------------------------------------------------------------


# DNMS Firmware Teensy4.0 V4 English

## V4 realizes a 2nd measurement interval besides the functionality of V3 

### Current version:

- DNMS_V4.4.4 - without USB-Audio output

### Prerequisites

- Usage of firmware versions NRZ-2020-134-DNMS-4.2 for NodeMCU to get the values of the 2nd interval read out of Teensy4.0. The application dnms-0.9.5 for the Rspberry Pi reads the 2nd measurement interval values as well and transmits the values to an InfluxDB. The 2nd measurement interval generates not only LAeq, LAmin and LAmax but as well the 1/3 octave values from that interval. Values from the 2nd interval are not transmitted to Sensor.Community.
- Usage of an InfluxDB or an own application for storing the 2nd interval values.

