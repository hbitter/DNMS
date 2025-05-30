/*
 *	airRohr firmware
 *	Copyright (C) 2016-2018 Code for Stuttgart a.o.
 *
 *  German translations
 *
 *	Texts should be as short as possible
 */

#define INTL_LANG "DE"
#define INTL_PM_SENSOR "DNMS Lärmsensor"
const char INTL_CONFIGURATION[] PROGMEM = "Konfiguration";
#define INTL_WIFI_SETTINGS "WLAN Daten"
#define INTL_WIFI_NETWORKS "Lade WLAN Netzwerke ..."
#define INTL_LANGUAGE "Sprache"
const char INTL_NO_NETWORKS[] PROGMEM =  "Keine Netzwerke gefunden";
const char INTL_NETWORKS_FOUND[] PROGMEM = "Netzwerke gefunden: ";
const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM = "Ab hier nur ändern, wenn Sie wirklich wissen, was Sie tun";
const char INTL_CAREFUL_ATTENTION[] = "Änderungen der I²C Pins bitte sorgfältig überprüfen!";
const char INTL_SAVE[] PROGMEM = "Speichern";
const char INTL_SENSORS[] PROGMEM = "Sensoren";
const char INTL_START[] PROGMEM = "Nach einem Neustart:";
const char INTL_START_ZWEI[] PROGMEM = "Achtung! Während der Wartezeit ist kein Zugriff auf den lokalen Webserver möglich!";
const char INTL_START_ON_FULL_MINUTE[] PROGMEM = "Beginn der Messungen zur vollen Minute";
const char INTL_START_ON_FULL_HOUR[] PROGMEM = "Beginn der Messungen zur vollen Stunde";
const char INTL_SENSOR_CYCLIC_RESTART[] PROGMEM = "Zyklischer Restart des Sensors";
const char INTL_SENSOR_CYCLIC_RESTART_TIME[] PROGMEM = "Zykluszeit für den Restart in h";
const char INTL_DNMS_SENSOR[] PROGMEM = "DNMS Lärm Sensor";
const char INTL_MORE_SENSORS[] PROGMEM = "Weitere Sensoren";
const char INTL_SDS011[] PROGMEM = "SDS011 ({pm})";
const char INTL_PMS[] PROGMEM = "Plantower PMS(1,3,5,6,7)003 ({pm})";
const char INTL_HPM[] PROGMEM = "Honeywell PM ({pm})";
const char INTL_NPM[] PROGMEM = "Tera Sensor Next PM ({pm})";
const char INTL_NPM_FULLTIME[] PROGMEM = "Next PM fulltime";
const char INTL_IPS[] PROGMEM = "IPS-7100 PM ({pm})";
const char INTL_SEN5X[] PROGMEM = "Sensirion SEN5X ({pm}, {t}, {h}, {voc}, {nox})";
const char INTL_SPS30[] PROGMEM = "Sensirion SPS30 ({pm})";
const char INTL_PPD42NS[] PROGMEM = "PPD42NS ({pm})";
const char INTL_DHT22[] PROGMEM = "DHT22 ({t}, {h})";
const char INTL_HTU21D[] PROGMEM = "HTU21D ({t}, {h})";
const char INTL_BMP180[] PROGMEM = "BMP180 ({t}, {p})";
const char INTL_BMX280[] PROGMEM = "BME280 ({t}, {h}, {p}), BMP280 ({t}, {p})";
const char INTL_SHT3X[] PROGMEM = "SHT3X ({t}, {h})";
const char INTL_SCD30[] PROGMEM = "SCD30 ({t}, {h}, CO₂)";
const char INTL_SCD4X[] PROGMEM = "SCD4X ({t}, {h}, CO₂)";
const char INTL_SCD4X_LOW_POWER[] PROGMEM = "SCD4X Messungen mit geringerem Stromverbrauch";
const char INTL_DS18B20[] PROGMEM = "DS18B20 ({t})";
const char INTL_I2C_GPIO_SCL[] PROGMEM = "I²C SCL GPIO Pin";
const char INTL_I2C_GPIO_SDA[] PROGMEM = "I²C SDA GPIO Pin";
const char INTL_DNMS[] PROGMEM = "DNMS ({l_a})";
const char INTL_DNMS_Z[] PROGMEM = "DNMS ({l_z})";
const char INTL_DNMS_SPECTRUM[] PROGMEM = "DNMS Frequenzspektrum A-Bewertung";
const char INTL_DNMS_SPECTRUM_Z[] PROGMEM = "DNMS Frequenzspektrum Z-Bewertung";
const char INTL_DNMS_2nd_INTERVAL[] PROGMEM = "DNMS 2. Messintervall (LAeq)";
const char INTL_DNMS_2nd_INTERVAL_Z[] PROGMEM = "DNMS 2. Messintervall (LZeq)";
const char INTL_DNMS_2nd_SPECTRUM[] PROGMEM = "DNMS 2. Messintervall Frequenzspektrum A-Bewertung";
const char INTL_DNMS_2nd_SPECTRUM_Z[] PROGMEM = "DNMS 2. Messintervall Frequenzspektrum Z-Bewertung";
const char INTL_DNMS_2nd_INTERVAL_TIME[] PROGMEM = "DNMS 2. Messintervall in Sekunden";
const char INTL_DNMS_2nd_THRESHOLD[] PROGMEM = "DNMS 2. Messintervall LAeq Schwellwert&nbsp;in&nbsp;dB(A)";
const char INTL_DNMS_2nd_NUMBER_MEASUREMENTS[] PROGMEM = "DNMS 2. Messintervall # Messungen nach Überschreitung";
const char INTL_DNMS_2nd_SWITCH_OUTPUT[] PROGMEM = "DNMS 2. Messintervall Ausgang schalten bei Überschreitung";
const char INTL_DNMS_2nd_GPIO_PIN[] PROGMEM = "DNMS 2. Messintervall GPIO Pin der geschaltet wird";
const char INTL_DNMS_CORRECTION[] PROGMEM = "DNMS Korrekturwert&nbsp;in&nbsp;dB(A)";
const char INTL_DNMS_MICROPHONE[] PROGMEM = "Welches Mikrofon ist am DNMS Sensor angeschlossen? (bitte nur eines auswählen)";
const char INTL_DNMS_ICS43434[] PROGMEM = "ICS-43434";
const char INTL_DNMS_IM72D128[] PROGMEM = "IM72D128";
const char INTL_TEMP_CORRECTION[] PROGMEM = "Korrekturwert in °C";
const char INTL_HEIGHT_ABOVE_SEALEVEL[] PROGMEM = "Höhe über Meeresspiegel (m)";
const char INTL_PRESSURE_AT_SEALEVEL[] PROGMEM = "Luftdruck auf Meereshöhe";
const char INTL_NEO6M[] PROGMEM = "GPS (NEO 6M)";
const char INTL_BASICAUTH[] PROGMEM = "BasicAuth aktivieren";
#define INTL_REPORT_ISSUE "Ein Problem melden"

const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM = "Sensor WLAN Name im Konfigurationsmodus";
const char INTL_FS_WIFI_NAME[] PROGMEM = "Name";
const char INTL_MORE_SETTINGS[] PROGMEM = "Weitere Einstellungen";
const char INTL_AUTO_UPDATE[] PROGMEM = "Auto Update";
const char INTL_USE_BETA[] PROGMEM = "Lade Beta Versionen";
const char INTL_DISPLAY[] PROGMEM = "OLED SSD1306";
const char INTL_SH1106[] PROGMEM = "OLED SH1106";
const char INTL_FLIP_DISPLAY[] PROGMEM = "OLED Display um 180° drehen";
const char INTL_LCD1602_27[] PROGMEM = "LCD 1602 (I2C: 0x27)";
const char INTL_LCD1602_3F[] PROGMEM = "LCD 1602 (I2C: 0x3F)";
const char INTL_LCD2004_27[] PROGMEM = "LCD 2004 (I2C: 0x27)";
const char INTL_LCD2004_3F[] PROGMEM = "LCD 2004 (I2C: 0x3F)";
const char INTL_DISPLAY_WIFI_INFO[] PROGMEM = "Zeige WiFi Info";
const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM = "Zeige Geräteinfo";
const char INTL_POWERSAVE [] PROGMEM ="Powersave?";

#define INTL_STATIC_IP_TEXT "Konfiguration statische IP Adresse (alle Felder müssen ausgefüllt sein)"
const char INTL_STATIC_IP[] PROGMEM = "IP-Adresse";
const char INTL_STATIC_SUBNET[] PROGMEM = "Subnet";
const char INTL_STATIC_GATEWAY[] PROGMEM = "Gateway";
const char INTL_STATIC_DNS[] PROGMEM = "DNS Server";

const char INTL_DEBUG_LEVEL[] PROGMEM = "Debug&nbsp;Level";
const char INTL_MEASUREMENT_INTERVAL[] PROGMEM = "Messintervall";
const char INTL_DURATION_ROUTER_MODE[] PROGMEM = "Dauer&nbsp;Routermodus";
const char INTL_MORE_APIS[] PROGMEM = "Weitere APIs";
const char INTL_SEND_TO_OWN_API[] PROGMEM = "An eigene API senden";
const char INTL_SERVER[] PROGMEM = "Server";
const char INTL_PATH[] PROGMEM = "Pfad";
const char INTL_PORT[] PROGMEM = "Port";
const char INTL_USER[] PROGMEM = "Benutzer";
const char INTL_PASSWORD[] PROGMEM = "Passwort";
const char INTL_DB[] PROGMEM = "DB/Bucket";
const char INTL_MQTT_MEASUREMENT_TEXT[] PROGMEM = "Falls die Daten vom MQTT Broker weiter an eine InfluxDB übertragen werden, bitte die InfluxDB 'Messung' unter InfluxDB eingeben oder ändern";
const char INTL_MQTT_MAIN_TOPIC[] PROGMEM = "MQTT main topic";
const char INTL_ID_AS_MQTT_SUB_TOPIC[] PROGMEM = "ID als MQTT Sub Topic";
const char INTL_MEASUREMENT[] PROGMEM = "Messung";
const char INTL_SEND_TO[] PROGMEM = "Senden an {v}";
const char INTL_READ_FROM[] PROGMEM = "Lese {v}";
const char INTL_SENSOR_IS_REBOOTING[] PROGMEM = "Sensor wird neu gestartet.";
const char INTL_RESTART_DEVICE[] PROGMEM = "Gerät neu starten";
const char INTL_DELETE_CONFIG[] PROGMEM = "Config.json löschen";
const char INTL_RESTART_SENSOR[] PROGMEM = "Sensor neu starten";
#define INTL_HOME "Übersicht"
#define INTL_BACK_TO_HOME "Zurück zur Startseite"
const char INTL_CURRENT_DATA[] PROGMEM = "Aktuelle Werte";
const char INTL_DEVICE_STATUS[] PROGMEM = "Gerätestatus";
#define INTL_ACTIVE_SENSORS_MAP "Karte der aktiven Sensoren (externer Link)"
#define INTL_CONFIGURATION_DELETE "Konfiguration löschen"
#define INTL_CONFIGURATION_REALLY_DELETE "Konfiguration wirklich löschen?"
#define INTL_DELETE "Löschen"
#define INTL_CANCEL "Abbrechen"
#define INTL_REALLY_RESTART_SENSOR "Sensor wirklich neu starten?"
#define INTL_RESTART "Neu starten"
const char INTL_SAVE_AND_RESTART[] PROGMEM = "Speichern und neu starten";
#define INTL_FIRMWARE "Firmware"
const char INTL_DEBUG_SETTING_TO[] PROGMEM = "Setze Debug auf";
#define INTL_NONE "Keine"
#define INTL_ERROR "Fehler"
#define INTL_WARNING "Warnungen"
#define INTL_MIN_INFO "min. Info"
#define INTL_MED_INFO "mttl. Info"
#define INTL_MAX_INFO "max. Info"
#define INTL_CONFIG_DELETED "Config.json gelöscht"
#define INTL_CONFIG_CAN_NOT_BE_DELETED "Config.json konnte nicht gelöscht werden"
#define INTL_CONFIG_NOT_FOUND "Config.json nicht gefunden"
const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM = "Noch {v} Sekunden bis zur ersten Messung.";
const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM = " Sekunden seit der letzten Messung.";
const char INTL_PARTICLES_PER_LITER[] PROGMEM = "Partikel/Liter";
const char INTL_PARTICULATE_MATTER[] PROGMEM = "Feinstaub";
const char INTL_TEMPERATURE[] PROGMEM = "Temperatur";
const char INTL_HUMIDITY[] PROGMEM = "rel. Luftfeuchte";
const char INTL_PRESSURE[] PROGMEM = "Luftdruck";
const char INTL_VOC[] PROGMEM = "flüchtige organische Stoffe VOC";
const char INTL_NOX[] PROGMEM = "Stickoxide NOx";
const char INTL_DEW_POINT[] PROGMEM = "Taupunkt";
const char INTL_CO2_PPM[] PROGMEM = "ppm CO₂";
const char INTL_LEQ_A[] PROGMEM = "LAeq";
const char INTL_LA_MIN[] PROGMEM = "LA min";
const char INTL_LA_MAX[] PROGMEM = "LA max";
const char INTL_LEQ_Z[] PROGMEM = "LZeq";
const char INTL_LZ_MIN[] PROGMEM = "LZ min";
const char INTL_LZ_MAX[] PROGMEM = "LZ max";
const char INTL_LEQ_A_2nd[] PROGMEM = "LAeq_2nd";
const char INTL_LA_MIN_2nd[] PROGMEM = "LA min_2nd";
const char INTL_LA_MAX_2nd[] PROGMEM = "LA max_2nd";
const char INTL_LEQ_Z_2nd[] PROGMEM = "LZeq_2nd";
const char INTL_LZ_MIN_2nd[] PROGMEM = "LZ min_2nd";
const char INTL_LZ_MAX_2nd[] PROGMEM = "LZ max_2nd";
const char INTL_LATITUDE[] PROGMEM = "Breite";
const char INTL_LONGITUDE[] PROGMEM = "Länge";
const char INTL_ALTITUDE[] PROGMEM = "Höhe";
const char INTL_TIME_UTC[] PROGMEM = "Zeit (UTC) ";
const char INTL_SIGNAL_STRENGTH[] PROGMEM = "Signal";
const char INTL_SIGNAL_QUALITY[] PROGMEM = "Qualität";
#define INTL_NUMBER_OF_MEASUREMENTS "Anzahl Messungen"
#define INTL_TIME_SENDING_MS "Dauer Messübertragung"
#define INTL_SENSOR "Sensor"
#define INTL_PARAMETER "Parameter"
#define INTL_VALUE "Wert"

#include "./airrohr-logo-common.h"
