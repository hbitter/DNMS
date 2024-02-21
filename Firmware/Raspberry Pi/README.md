# [English text below](#raspberry-pi-application-for-dnms)

## Raspberry Pi Anwendung für DNMS

  dnms-0.9.12 erste veröffentliche Version der DNMS Kommunikations Anwendung für den Raspberry Pi (ab Pi Zero möglich, Pi Zero 2 aufwärts empfohlen).
  
###  Funktionalität:
  -	Übertragung der DNMS Daten zu Sensor.Community.
  -	Übertragung der DNMS Daten zu einer InfluxDB (V1.8) für unabhängige 2 Messintervalle.
  -	Das zweite Messintervall übertragt die Daten an die InfluxDB ab einem konfigurierbaren Schwellenwert (LAeq). Die Anzahl der Übertragungen (Messintervalle), die nach Überschreitung des Schwellenwertes übertragen werden, kann ebenfalls  konfiguriert werden.
  -	Konfiguration der Werte, die zur InfluxDB übertragen werden, wie LAeq, LZeq und/oder die jeweiligen Terzwerte.
  -	Minimales Messintervall von 125ms konfigurierbar. Sollen Terzwerte übertragen werden, ist dies ab einem Messintervall von 500ms aufwärts möglich.
  -	Konfiguration der Eigenschaften und der Übertragung der Messwerte an Sensor.Community und/oder einer InfluxDB mittels Konfigurationsdatei (dnms.conf).
  - Installation der Anwendung als Service d.h. die DNMS Anwendung läuft im Hintergrund und startet nach einem Neustart des Raspberry Pi automatisch.
  
### Voraussetzungen:
- Aktuelles Raspberry Pi OS installiert (Bookworm)
- ssh Zugriff auf den Raspberry Pi
- I²C Interface aktiviert (raspi-config) 

### Installation:
 - ssh Verbindung zum Raspberry Pi herstellen.
 - Einen neuen Ordner für die Anwendung unterhalb des aktuellen Benutzers einrichten (z.B. dnms): 
  
        mkdir dnms
 - In den angelegten Ordner wechseln:
  
        cd dnms
 - Die .zip Datei (aktuell dnms-0.9.12.zip) vom PC auf den Raspberry Pi in den neu angelegten Ordner übertragen. Dafür eignet sich z.B. das kostenlose Programm FileZilla sehr gut (Win, Linux, Mac).
 - Überprüfen, ob die .zip Datei (aktuell dnms-0.9.12.zip) nun auf dem Raspberry Pi vorhanden ist mit ls:
 
	 ![](images/pic1.jpg)

 - Entpacken der .zip Datei
 
		unzip dnms.zip

	mit folgendem Ergebnis:
	
	![](images/pic2.jpg)

- Die Datei dnms_install.sh ausführbar machen:
       
       chmod +x dnms_install.sh
- Das Installations-Script starten:
       
       sudo ./dnms_install.sh
	Das Installations-Script kann einige Minuten benötigen, bis es komplett durchgelaufen ist, da auch ein kompletter update/upgrade Zyklus durchlaufen wird.
	
	![](images/pic3.jpg)
 
  Das Installations-Script sollte folgendermaßen enden:
  
	![](images/pic4.jpg)

 - Überprüfung des Status von dnms.service:

       sudo systemctl status dnms.service

	![](images/pic5.jpg)

- Stoppen von dnms.service um zuerst die Konfiguration zu bearbeiten:

		sudo systemctl stop dnms.service

- Editieren der Datei dnms.conf, die die Konfiguration enthält z.B. mit nano:

		nano dnms.conf

	![](images/pic6.jpg)

- Sind alle Einstellungen angepasst in der Datei dnms.conf insbesondere Adresse, Passwort, Datenbankname usw. für den Zugang zu einer InfluxDB, kann dnms.service wieder gestartet werden:

		sudo systemctl restart dnms.service

- Achtung: Nach jeder Änderung der Konfiguration in der Datei dnms.conf, muss dnms.service wieder mit dem obigen Befehl gestartet werden.
- Konnte die Konfiguration erfolgreich gestartet werden, so wird ein Logfile mit dem Namen dnms.log im Installationsordner angelegt, Ausgabe z.B. mit:
 
	  cat dnms.log

	![](images/pic7.jpg)

- Die Messwerte können auch in die Datei dnms.log geschrieben werden. Dies muss konfiguriert werden (Output to Terminal - Umleitung der Terminalausgabe in die Datei). Es ist aber keine komplette Datenlogger Funktionalität realisiert. Diese rudimentäre Funktionalität dient zur Einrichtung und zum Test der Konfiguration und zur Aufzeichnung von Fehlern. Es lassen sich auch die Übertragungszeiten zur InfluxDB loggen. 

	![](images/pic8.jpg)


------------------------------------------------------------------------

## Raspberry Pi application for DNMS


dnms-0.9.12 First released version of the DNMS communication application for Raspberry Pi (possible on Pi Zero, recommended from Pi Zero 2 on).
  
###  Functionality:
  -	Transfer of DNMS data to Sensor.Community.
  -	Transfer of DNMS data to an InfluxDB (V1.8) for 2 independent measurement intervals.
  -	The second measurement interval transmits data to the InfluxDB when a configurable threshold (LAeq) is exceeded. The number of transmissions (measurement intervals) to be sent when the threshold is exceeded can also be configured.
  -Configuration of the values to be transferred to InfluxDB, such as LAeq, LZeq and/or their respective 1/3-octave values.
  -	Minimum measuring interval configurable from 125ms. If 1/3-octave values are to be transmitted, this is possible from a measuring interval of 500ms upwards.
  -Configuration of properties and transmission of readings to Sensor.Community and/or an InfluxDB using a configuration file (dnms.conf).
  - Installation of the application as a service, i.e. the DNMS application runs in the background and starts automatically when the Raspberry Pi is rebooted.
  
### Prerequisite:
- Current Raspberry Pi OS installed (Bookworm)
- ssh access to the Raspberry Pi
- I²C interface enabled (raspi-config)

### Installation:
- Make an ssh connection to the Raspberry Pi.
 - Create a new folder for the application under the current user (e.g. dnms):
  
        mkdir dnms
- Switch to the created folder:
  
        cd dnms
- Copy the .zip file (currently dnms-0.9.12.zip) from your PC to the newly created folder on the Raspberry Pi. You can use the free program FileZilla (Win, Linux, Mac) to do this.
 - Use ls to check if the .zip file (currently dnms-0.9.12.zip) is now present on the Raspberry Pi:
 
	 ![](images/pic1.jpg)

- Unzip the .zip file
 
		unzip dnms.zip

	with the following result:
	
	![](images/pic2.jpg)

- Make the dnms_install.sh file executable:
       
       chmod +x dnms_install.sh
- Launch the installation script:
       
       sudo ./dnms_install.sh

	The installation script may take a few minutes to complete as it will also perform a full update/upgrade cycle.
	
	![](images/pic3.jpg)
 
	The installation script should end as follows:  

	![](images/pic4.jpg)

- Check the status of dnms.service:

       sudo systemctl status dnms.service

	![](images/pic5.jpg)

- Stop the dnms.service to edit the configuration first:

		sudo systemctl stop dnms.service

- Edit the dnms.conf file that contains the configuration, e.g. with nano:

		nano dnms.conf

	![](images/pic6.jpg)

- Once all the settings in the dnms.conf file have been adjusted, in particular the address, password, database name, etc. for accessing an InfluxDB, the dnms.service can be restarted:

		sudo systemctl restart dnms.service

- Attention: After each change of the configuration in the dnms.conf file, the dnms.service must be restarted with the above command.
- If the configuration was started successfully, a log file with the name dnms.log is created in the installation folder:
 
		cat dnms.log

	![](images/pic7.jpg)

- The readings can also be written to the dnms.log file. This must be configured (output to terminal - redirect terminal output to the file). However, no complete data logger functionality is implemented. This rudimentary functionality is used to set up and test the configuration and to record errors. It is also possible to log the transfer times to InfluxDB.

	![](images/pic8.jpg)