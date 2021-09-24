# Testprogramm für Teensy 3.6 und ICS-43434

## Flashen des Teensy 3.6 mit dem Testprogramm

Für die Teensy Boards existiert ein standalone Flash-Programm - der Teensy Loader - sowohl mit grafischer Oberfläche als auch als Version für die Kommandozeile jeweils für Windows, Linux und Mac. Der Download des Teensy Loader ist unter: https://www.pjrc.com/teensy/loader.html möglich. Weiterhin ist dort im Detail der Flash Vorgang unter den verschiedenen Betriebssystemen beschrieben.

## Funktion und Ausgabe des Testprogramms

Nach dem Flashen oder nach Power Up startet das Testprogramm und berechnet aus den Werten des angeschlossenen Mikrofons den LAeq, LAmin und LAmax. Diese drei Werte werden dann alle 10 Sekunden über die USB Schnittstelle an ein Terminalprogramm ausgegeben (Serieller Monitor der Arduino IDE od. PuTTY mit 115200 baud). Die Ausgabe sollte wie auf dem folgenden Bild aussehen:

------------------------------------------------------------------------

# Test program for Teensy 3.6 and ICS-43434

## How to flash the Teensy 3.6 with the test program

For all Teensy boards a flash tool exists - the Teensy Loader. There is a version with a graphical user interface and also a commad line version for Windows, Linux and Mac. You can download the tool from: https://www.pjrc.com/teensy/loader.html. The details how to flash the Teensy board under the various operating systems are given on that page.

## Functions and output of the test program

After flashing or after power up the test program starts caculatiing LAeq, LAmin and LAmax based on the values from the connected microphone. These three values will be written over USB to a terminal program (serial monitor of the Arduino IDE or Putty with a baud rate of 115200). The following picture shows how the output on the terminal should look like:

------------------------------------------------------------------------

<img src="images/Image 1.jpg"><br>

