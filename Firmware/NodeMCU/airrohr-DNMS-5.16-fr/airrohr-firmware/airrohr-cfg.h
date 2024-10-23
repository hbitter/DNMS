
enum ConfigEntryType : unsigned short {
	Config_Type_Bool,
	Config_Type_UInt,
	Config_Type_Time,
	Config_Type_String,
	Config_Type_Password
};

struct ConfigShapeEntry {
	enum ConfigEntryType cfg_type;
	unsigned short cfg_len;
	const char* _cfg_key;
	union {
		void* as_void;
		bool* as_bool;
		unsigned int* as_uint;
		char* as_str;
	} cfg_val;
	const __FlashStringHelper* cfg_key() const { return FPSTR(_cfg_key); }
};

enum ConfigShapeId {
	Config_current_lang,
	Config_wlanssid,
  Config_with_wlanpwd,
	Config_wlanpwd,
	Config_www_username,
	Config_www_password,
	Config_fs_ssid,
	Config_fs_pwd,
	Config_www_basicauth_enabled,
  Config_start_on_full_minute,
  Config_start_on_full_hour,
  Config_sensor_cyclic_restart,
  Config_sensor_cyclic_restart_time,
	Config_dht_read,
	Config_htu21d_read,
	Config_ppd_read,
	Config_sds_read,
	Config_pms_read,
	Config_hpm_read,
	Config_npm_read,
	Config_npm_fulltime,
	Config_ips_read,
	Config_sps30_read,
 	Config_sen5x_read, 
	Config_bmp_read,
	Config_bmx280_read,
	Config_sht3x_read,
	Config_scd30_read,
  Config_scd4x_read,
  Config_scd4x_low_power,
	Config_ds18b20_read,
  Config_i2c_gpio_scl,
  Config_i2c_gpio_sda,
	Config_dnms_read,
  Config_dnms_read_Z,
	Config_dnms_read_spectrum,
	Config_dnms_read_spectrum_Z,  
  Config_dnms_2nd_interval,
  Config_dnms_2nd_interval_Z,
	Config_dnms_2nd_read_spectrum,
	Config_dnms_2nd_read_spectrum_Z,
  Config_dnms_2nd_interval_time,
  Config_dnms_2nd_threshold,
  Config_dnms_2nd_number_measurements,
  Config_dnms_2nd_switch_output,
  Config_dnms_2nd_gpio_pin,     	
  Config_dnms_correction,
  Config_dnms_ICS43434,
  Config_dnms_IM72D128,
	Config_temp_correction,
	Config_height_above_sealevel,
	Config_gps_read,
	Config_send2dusti,
	Config_ssl_dusti,
	Config_send2madavi,
	Config_ssl_madavi,
	Config_send2sensemap,
	Config_send2fsapp,
	Config_send2aircms,
	Config_send2csv,
	Config_auto_update,
	Config_use_beta,
	Config_has_display,
	Config_has_sh1106,
	Config_has_flipped_display,
	Config_has_lcd1602,
	Config_has_lcd1602_27,
	Config_has_lcd2004,
	Config_has_lcd2004_27,
	Config_display_wifi_info,
	Config_display_device_info,
	Config_static_ip,
	Config_static_subnet,
	Config_static_gateway,
	Config_static_dns,
	Config_powersave,
	Config_debug,
	Config_sending_intervall_ms,
	Config_time_for_wifi_config,
	Config_senseboxid,
	Config_send2custom,
	Config_host_custom,
	Config_url_custom,
	Config_port_custom,
	Config_user_custom,
	Config_pwd_custom,
	Config_ssl_custom,
	Config_send2influx,
	Config_host_influx,
	Config_url_influx,
	Config_port_influx,
	Config_user_influx,
	Config_pwd_influx,
	Config_measurement_name_influx,
	Config_ssl_influx,
  Config_send2mqtt,
	Config_host_mqtt,
	Config_port_mqtt,
	Config_user_mqtt,
	Config_pwd_mqtt,
  Config_measurement_name_mqtt,
  Config_mqtt_main_topic,
  Config_id_as_mqtt_sub_topic,
	Config_tls_mqtt,
  Config_ntp_1,
  Config_ntp_2,
};
static constexpr char CFG_KEY_CURRENT_LANG[] PROGMEM = "current_lang";
static constexpr char CFG_KEY_WLANSSID[] PROGMEM = "wlanssid";
static constexpr char CFG_KEY_WITH_WLANPWD[] PROGMEM = "with_wlanpwd";
static constexpr char CFG_KEY_WLANPWD[] PROGMEM = "wlanpwd";
static constexpr char CFG_KEY_WWW_USERNAME[] PROGMEM = "www_username";
static constexpr char CFG_KEY_WWW_PASSWORD[] PROGMEM = "www_password";
static constexpr char CFG_KEY_FS_SSID[] PROGMEM = "fs_ssid";
static constexpr char CFG_KEY_FS_PWD[] PROGMEM = "fs_pwd";
static constexpr char CFG_KEY_WWW_BASICAUTH_ENABLED[] PROGMEM = "www_basicauth_enabled";
static constexpr char CFG_KEY_START_ON_FULL_MINUTE[] PROGMEM = "start on full minute";
static constexpr char CFG_KEY_START_ON_FULL_HOUR[] PROGMEM = "start on full hourt";
static constexpr char CFG_KEY_SENSOR_CYCLIC_RESTART[] PROGMEM = "sensor_restart";
static constexpr char CFG_KEY_SENSOR_CYCLIC_RESTART_TIME[] PROGMEM = "sensor_restart_time";
static constexpr char CFG_KEY_DHT_READ[] PROGMEM = "dht_read";
static constexpr char CFG_KEY_HTU21D_READ[] PROGMEM = "htu21d_read";
static constexpr char CFG_KEY_PPD_READ[] PROGMEM = "ppd_read";
static constexpr char CFG_KEY_SDS_READ[] PROGMEM = "sds_read";
static constexpr char CFG_KEY_PMS_READ[] PROGMEM = "pms_read";
static constexpr char CFG_KEY_HPM_READ[] PROGMEM = "hpm_read";
static constexpr char CFG_KEY_NPM_READ[] PROGMEM = "npm_read";
static constexpr char CFG_KEY_NPM_FULLTIME[] PROGMEM = "npm_fulltime";
static constexpr char CFG_KEY_IPS_READ[] PROGMEM = "ips_read";
static constexpr char CFG_KEY_SPS30_READ[] PROGMEM = "sps30_read";
static constexpr char CFG_KEY_SEN5X_READ[] PROGMEM = "sen5x_read";
static constexpr char CFG_KEY_BMP_READ[] PROGMEM = "bmp_read";
static constexpr char CFG_KEY_BMX280_READ[] PROGMEM = "bmx280_read";
static constexpr char CFG_KEY_SHT3X_READ[] PROGMEM = "sht3x_read";
static constexpr char CFG_KEY_SCD30_READ[] PROGMEM = "scd30_read";
static constexpr char CFG_KEY_SCD4X_READ[] PROGMEM = "scd4x_read";
static constexpr char CFG_KEY_SCD4X_LOW_POWER[] PROGMEM = "scd4x_low_power";
static constexpr char CFG_KEY_DS18B20_READ[] PROGMEM = "ds18b20_read";
static constexpr char CFG_KEY_I2C_GPIO_SCL[] PROGMEM = "i2c_gpio_scl";
static constexpr char CFG_KEY_I2C_GPIO_SDA[] PROGMEM = "i2c_gpio_sda";
static constexpr char CFG_KEY_DNMS_READ[] PROGMEM = "dnms_read";
static constexpr char CFG_KEY_DNMS_READ_Z[] PROGMEM = "dnms_read_Z";
static constexpr char CFG_KEY_DNMS_READ_SPECTRUM[] PROGMEM = "dnms_read_spectrum";
static constexpr char CFG_KEY_DNMS_READ_SPECTRUM_Z[] PROGMEM = "dnms_read_spectrum_Z";
static constexpr char CFG_KEY_DNMS_2nd_INTERVAL[] PROGMEM = "dnms_2nd_interval";
static constexpr char CFG_KEY_DNMS_2nd_INTERVAL_Z[] PROGMEM = "dnms_2nd_interval_Z";
static constexpr char CFG_KEY_DNMS_2nd_READ_SPECTRUM[] PROGMEM = "dnms_2nd_read_spectrum";
static constexpr char CFG_KEY_DNMS_2nd_READ_SPECTRUM_Z[] PROGMEM = "dnms_2nd_read_spectrum_Z";
static constexpr char CFG_KEY_DNMS_2nd_INTERVAL_TIME[] PROGMEM = "dnms_2nd_interval_time";
static constexpr char CFG_KEY_DNMS_2nd_THRESHOLD[] PROGMEM = "dnms_2nd_threshold";
static constexpr char CFG_KEY_DNMS_2nd_NUMBER_MEASUREMENTS[] PROGMEM = "dnms_2nd_number_measurements";
static constexpr char CFG_KEY_DNMS_2nd_SWITCH_OUTPUT[] PROGMEM = "dnms_2nd_switch_output";
static constexpr char CFG_KEY_DNMS_2nd_GPIO_PIN[] PROGMEM = "dnms_2nd_gpio_pin";
static constexpr char CFG_KEY_DNMS_CORRECTION[] PROGMEM = "dnms_correction";
static constexpr char CFG_KEY_DNMS_ICS43434[] PROGMEM = "ICS-43434";
static constexpr char CFG_KEY_DNMS_IM72D128[] PROGMEM = "IM72D128";
static constexpr char CFG_KEY_TEMP_CORRECTION[] PROGMEM = "temp_correction";
static constexpr char CFG_KEY_HEIGHT_ABOVE_SEALEVEL[] PROGMEM = "height_above_sealevel";
static constexpr char CFG_KEY_GPS_READ[] PROGMEM = "gps_read";
static constexpr char CFG_KEY_SEND2DUSTI[] PROGMEM = "send2dusti";
static constexpr char CFG_KEY_SSL_DUSTI[] PROGMEM = "ssl_dusti";
static constexpr char CFG_KEY_SEND2MADAVI[] PROGMEM = "send2madavi";
static constexpr char CFG_KEY_SSL_MADAVI[] PROGMEM = "ssl_madavi";
static constexpr char CFG_KEY_SEND2SENSEMAP[] PROGMEM = "send2sensemap";
static constexpr char CFG_KEY_SEND2FSAPP[] PROGMEM = "send2fsapp";
static constexpr char CFG_KEY_SEND2AIRCMS[] PROGMEM = "send2aircms";
static constexpr char CFG_KEY_SEND2CSV[] PROGMEM = "send2csv";
static constexpr char CFG_KEY_AUTO_UPDATE[] PROGMEM = "auto_update";
static constexpr char CFG_KEY_USE_BETA[] PROGMEM = "use_beta";
static constexpr char CFG_KEY_HAS_DISPLAY[] PROGMEM = "has_display";
static constexpr char CFG_KEY_HAS_SH1106[] PROGMEM = "has_sh1106";
static constexpr char CFG_KEY_HAS_FLIPPED_DISPLAY[] PROGMEM = "has_flipped_display";
static constexpr char CFG_KEY_HAS_LCD1602[] PROGMEM = "has_lcd1602";
static constexpr char CFG_KEY_HAS_LCD1602_27[] PROGMEM = "has_lcd1602_27";
static constexpr char CFG_KEY_HAS_LCD2004[] PROGMEM = "has_lcd2004";
static constexpr char CFG_KEY_HAS_LCD2004_27[] PROGMEM = "has_lcd2004_27";
static constexpr char CFG_KEY_DISPLAY_WIFI_INFO[] PROGMEM = "display_wifi_info";
static constexpr char CFG_KEY_DISPLAY_DEVICE_INFO[] PROGMEM = "display_device_info";
static constexpr char CFG_KEY_STATIC_IP[] PROGMEM = "static_ip";
static constexpr char CFG_KEY_STATIC_SUBNET[] PROGMEM = "static_subnet";
static constexpr char CFG_KEY_STATIC_GATEWAY[] PROGMEM = "static_gateway";
static constexpr char CFG_KEY_STATIC_DNS[] PROGMEM = "static_dns";
static constexpr char CFG_KEY_POWERSAVE[] PROGMEM = "powersave";
static constexpr char CFG_KEY_DEBUG[] PROGMEM = "debug";
static constexpr char CFG_KEY_SENDING_INTERVALL_MS[] PROGMEM = "sending_intervall_ms";
static constexpr char CFG_KEY_TIME_FOR_WIFI_CONFIG[] PROGMEM = "time_for_wifi_config";
static constexpr char CFG_KEY_SENSEBOXID[] PROGMEM = "senseboxid";
static constexpr char CFG_KEY_SEND2CUSTOM[] PROGMEM = "send2custom";
static constexpr char CFG_KEY_HOST_CUSTOM[] PROGMEM = "host_custom";
static constexpr char CFG_KEY_URL_CUSTOM[] PROGMEM = "url_custom";
static constexpr char CFG_KEY_PORT_CUSTOM[] PROGMEM = "port_custom";
static constexpr char CFG_KEY_USER_CUSTOM[] PROGMEM = "user_custom";
static constexpr char CFG_KEY_PWD_CUSTOM[] PROGMEM = "pwd_custom";
static constexpr char CFG_KEY_SSL_CUSTOM[] PROGMEM = "ssl_custom";
static constexpr char CFG_KEY_SEND2INFLUX[] PROGMEM = "send2influx";
static constexpr char CFG_KEY_HOST_INFLUX[] PROGMEM = "host_influx";
static constexpr char CFG_KEY_URL_INFLUX[] PROGMEM = "url_influx";
static constexpr char CFG_KEY_PORT_INFLUX[] PROGMEM = "port_influx";
static constexpr char CFG_KEY_USER_INFLUX[] PROGMEM = "user_influx";
static constexpr char CFG_KEY_PWD_INFLUX[] PROGMEM = "pwd_influx";
static constexpr char CFG_KEY_MEASUREMENT_NAME_INFLUX[] PROGMEM = "measurement_name_influx";
static constexpr char CFG_KEY_SSL_INFLUX[] PROGMEM = "ssl_influx";
static constexpr char CFG_KEY_SEND2MQTT[] PROGMEM = "send2mqtt";
static constexpr char CFG_KEY_HOST_MQTT[] PROGMEM = "host_mqtt";
static constexpr char CFG_KEY_PORT_MQTT[] PROGMEM = "port_mqtt";
static constexpr char CFG_KEY_USER_MQTT[] PROGMEM = "user_mqtt";
static constexpr char CFG_KEY_PWD_MQTT[] PROGMEM = "pwd_mqtt";
static constexpr char CFG_KEY_MEASUREMENT_NAME_MQTT[] PROGMEM = "measurement_name_mqtt";
static constexpr char CFG_KEY_MQTT_MAIN_TOPIC[] PROGMEM = "mqtt_main_topic";
static constexpr char CFG_KEY_ID_AS_MQTT_SUB_TOPIC[] PROGMEM = "id_as_mqtt_sub_topic";
static constexpr char CFG_KEY_TLS_MQTT[] PROGMEM = "tls_mqtt";
static constexpr char CFG_KEY_NTP_SERVER_1[] PROGMEM = "ntp_1";
static constexpr char CFG_KEY_NTP_SERVER_2[] PROGMEM = "ntp_2";

static constexpr ConfigShapeEntry configShape[] PROGMEM = {
	{ Config_Type_String, sizeof(cfg::current_lang)-1, CFG_KEY_CURRENT_LANG, cfg::current_lang },
	{ Config_Type_String, sizeof(cfg::wlanssid)-1, CFG_KEY_WLANSSID, cfg::wlanssid },
	{ Config_Type_Bool, 0, CFG_KEY_WITH_WLANPWD, &cfg::with_wlanpwd },  
	{ Config_Type_Password, sizeof(cfg::wlanpwd)-1, CFG_KEY_WLANPWD, cfg::wlanpwd },
	{ Config_Type_String, sizeof(cfg::www_username)-1, CFG_KEY_WWW_USERNAME, cfg::www_username },
	{ Config_Type_Password, sizeof(cfg::www_password)-1, CFG_KEY_WWW_PASSWORD, cfg::www_password },
	{ Config_Type_String, sizeof(cfg::fs_ssid)-1, CFG_KEY_FS_SSID, cfg::fs_ssid },
	{ Config_Type_Password, sizeof(cfg::fs_pwd)-1, CFG_KEY_FS_PWD, cfg::fs_pwd },
	{ Config_Type_Bool, 0, CFG_KEY_WWW_BASICAUTH_ENABLED, &cfg::www_basicauth_enabled },
	{ Config_Type_Bool, 0, CFG_KEY_START_ON_FULL_MINUTE, &cfg::start_on_full_minute },
	{ Config_Type_Bool, 0, CFG_KEY_START_ON_FULL_HOUR, &cfg::start_on_full_hour },
	{ Config_Type_Bool, 0, CFG_KEY_SENSOR_CYCLIC_RESTART, &cfg::sensor_cyclic_restart },
	{ Config_Type_UInt, 0, CFG_KEY_SENSOR_CYCLIC_RESTART_TIME, &cfg::sensor_cyclic_restart_time }, 
	{ Config_Type_Bool, 0, CFG_KEY_DHT_READ, &cfg::dht_read },
	{ Config_Type_Bool, 0, CFG_KEY_HTU21D_READ, &cfg::htu21d_read },
	{ Config_Type_Bool, 0, CFG_KEY_PPD_READ, &cfg::ppd_read },
	{ Config_Type_Bool, 0, CFG_KEY_SDS_READ, &cfg::sds_read },
	{ Config_Type_Bool, 0, CFG_KEY_PMS_READ, &cfg::pms_read },
	{ Config_Type_Bool, 0, CFG_KEY_HPM_READ, &cfg::hpm_read },
	{ Config_Type_Bool, 0, CFG_KEY_NPM_READ, &cfg::npm_read },
	{ Config_Type_Bool, 0, CFG_KEY_NPM_FULLTIME, &cfg::npm_fulltime },
	{ Config_Type_Bool, 0, CFG_KEY_IPS_READ, &cfg::ips_read },
	{ Config_Type_Bool, 0, CFG_KEY_SPS30_READ, &cfg::sps30_read },
	{ Config_Type_Bool, 0, CFG_KEY_SEN5X_READ, &cfg::sen5x_read }, 
	{ Config_Type_Bool, 0, CFG_KEY_BMP_READ, &cfg::bmp_read },
	{ Config_Type_Bool, 0, CFG_KEY_BMX280_READ, &cfg::bmx280_read },
	{ Config_Type_Bool, 0, CFG_KEY_SHT3X_READ, &cfg::sht3x_read },
	{ Config_Type_Bool, 0, CFG_KEY_SCD30_READ, &cfg::scd30_read },
	{ Config_Type_Bool, 0, CFG_KEY_SCD4X_READ, &cfg::scd4x_read },
 	{ Config_Type_Bool, 0, CFG_KEY_SCD4X_LOW_POWER, &cfg::scd4x_low_power },      
	{ Config_Type_Bool, 0, CFG_KEY_DS18B20_READ, &cfg::ds18b20_read },
	{ Config_Type_UInt, 0, CFG_KEY_I2C_GPIO_SCL, &cfg::i2c_gpio_scl },
	{ Config_Type_UInt, 0, CFG_KEY_I2C_GPIO_SDA, &cfg::i2c_gpio_sda },
	{ Config_Type_Bool, 0, CFG_KEY_DNMS_READ, &cfg::dnms_read },
	{ Config_Type_Bool, 0, CFG_KEY_DNMS_READ_Z, &cfg::dnms_read_Z },  
	{ Config_Type_Bool, 0, CFG_KEY_DNMS_READ_SPECTRUM, &cfg::dnms_read_spectrum },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_READ_SPECTRUM_Z, &cfg::dnms_read_spectrum_Z },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_2nd_INTERVAL, &cfg::dnms_2nd_interval },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_2nd_INTERVAL_Z, &cfg::dnms_2nd_interval_Z },
	{ Config_Type_Bool, 0, CFG_KEY_DNMS_2nd_READ_SPECTRUM, &cfg::dnms_2nd_read_spectrum },  
 	{ Config_Type_Bool, 0, CFG_KEY_DNMS_2nd_READ_SPECTRUM_Z, &cfg::dnms_2nd_read_spectrum_Z },    
  { Config_Type_Time, 0, CFG_KEY_DNMS_2nd_INTERVAL_TIME, &cfg::dnms_2nd_interval_time },
	{ Config_Type_UInt, 0, CFG_KEY_DNMS_2nd_THRESHOLD, &cfg::dnms_2nd_threshold },  
  { Config_Type_UInt, 0, CFG_KEY_DNMS_2nd_NUMBER_MEASUREMENTS, &cfg::dnms_2nd_number_measurements },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_2nd_SWITCH_OUTPUT, &cfg::dnms_2nd_switch_output },
  { Config_Type_UInt, 0, CFG_KEY_DNMS_2nd_GPIO_PIN, &cfg::dnms_2nd_gpio_pin },
	{ Config_Type_String, sizeof(cfg::dnms_correction)-1, CFG_KEY_DNMS_CORRECTION, cfg::dnms_correction },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_ICS43434, &cfg::dnms_ICS43434 },
  { Config_Type_Bool, 0, CFG_KEY_DNMS_IM72D128, &cfg::dnms_IM72D128 },
	{ Config_Type_String, sizeof(cfg::temp_correction)-1, CFG_KEY_TEMP_CORRECTION, cfg::temp_correction },
	{ Config_Type_String, sizeof(cfg::height_above_sealevel)-1, CFG_KEY_HEIGHT_ABOVE_SEALEVEL, cfg::height_above_sealevel },
	{ Config_Type_Bool, 0, CFG_KEY_GPS_READ, &cfg::gps_read },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2DUSTI, &cfg::send2dusti },
	{ Config_Type_Bool, 0, CFG_KEY_SSL_DUSTI, &cfg::ssl_dusti },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2MADAVI, &cfg::send2madavi },
	{ Config_Type_Bool, 0, CFG_KEY_SSL_MADAVI, &cfg::ssl_madavi },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2SENSEMAP, &cfg::send2sensemap },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2FSAPP, &cfg::send2fsapp },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2AIRCMS, &cfg::send2aircms },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2CSV, &cfg::send2csv },
	{ Config_Type_Bool, 0, CFG_KEY_AUTO_UPDATE, &cfg::auto_update },
	{ Config_Type_Bool, 0, CFG_KEY_USE_BETA, &cfg::use_beta },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_DISPLAY, &cfg::has_display },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_SH1106, &cfg::has_sh1106 },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_FLIPPED_DISPLAY, &cfg::has_flipped_display },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_LCD1602, &cfg::has_lcd1602 },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_LCD1602_27, &cfg::has_lcd1602_27 },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_LCD2004, &cfg::has_lcd2004 },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_LCD2004_27, &cfg::has_lcd2004_27 },
	{ Config_Type_Bool, 0, CFG_KEY_DISPLAY_WIFI_INFO, &cfg::display_wifi_info },
	{ Config_Type_Bool, 0, CFG_KEY_DISPLAY_DEVICE_INFO, &cfg::display_device_info },
	{ Config_Type_String, sizeof(cfg::static_ip)-1, CFG_KEY_STATIC_IP, cfg::static_ip },
	{ Config_Type_String, sizeof(cfg::static_subnet)-1, CFG_KEY_STATIC_SUBNET, cfg::static_subnet },
	{ Config_Type_String, sizeof(cfg::static_gateway)-1, CFG_KEY_STATIC_GATEWAY, cfg::static_gateway },
	{ Config_Type_String, sizeof(cfg::static_dns)-1, CFG_KEY_STATIC_DNS, cfg::static_dns },
	{ Config_Type_Bool, 0, CFG_KEY_POWERSAVE, &cfg::powersave },
	{ Config_Type_UInt, 0, CFG_KEY_DEBUG, &cfg::debug },
	{ Config_Type_Time, 0, CFG_KEY_SENDING_INTERVALL_MS, &cfg::sending_intervall_ms },
	{ Config_Type_Time, 0, CFG_KEY_TIME_FOR_WIFI_CONFIG, &cfg::time_for_wifi_config },
	{ Config_Type_String, sizeof(cfg::senseboxid)-1, CFG_KEY_SENSEBOXID, cfg::senseboxid },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2CUSTOM, &cfg::send2custom },
	{ Config_Type_String, sizeof(cfg::host_custom)-1, CFG_KEY_HOST_CUSTOM, cfg::host_custom },
	{ Config_Type_String, sizeof(cfg::url_custom)-1, CFG_KEY_URL_CUSTOM, cfg::url_custom },
	{ Config_Type_UInt, 0, CFG_KEY_PORT_CUSTOM, &cfg::port_custom },
	{ Config_Type_String, sizeof(cfg::user_custom)-1, CFG_KEY_USER_CUSTOM, cfg::user_custom },
	{ Config_Type_Password, sizeof(cfg::pwd_custom)-1, CFG_KEY_PWD_CUSTOM, cfg::pwd_custom },
	{ Config_Type_Bool, 0, CFG_KEY_SSL_CUSTOM, &cfg::ssl_custom },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2INFLUX, &cfg::send2influx },
	{ Config_Type_String, sizeof(cfg::host_influx)-1, CFG_KEY_HOST_INFLUX, cfg::host_influx },
	{ Config_Type_String, sizeof(cfg::url_influx)-1, CFG_KEY_URL_INFLUX, cfg::url_influx },
	{ Config_Type_UInt, 0, CFG_KEY_PORT_INFLUX, &cfg::port_influx },
	{ Config_Type_String, sizeof(cfg::user_influx)-1, CFG_KEY_USER_INFLUX, cfg::user_influx },
	{ Config_Type_Password, sizeof(cfg::pwd_influx)-1, CFG_KEY_PWD_INFLUX, cfg::pwd_influx },
	{ Config_Type_String, sizeof(cfg::measurement_name_influx)-1, CFG_KEY_MEASUREMENT_NAME_INFLUX, cfg::measurement_name_influx },
  { Config_Type_Bool, 0, CFG_KEY_SSL_INFLUX, &cfg::ssl_influx },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2MQTT, &cfg::send2mqtt },
	{ Config_Type_String, sizeof(cfg::host_mqtt)-1, CFG_KEY_HOST_MQTT, cfg::host_mqtt },
	{ Config_Type_UInt, 0, CFG_KEY_PORT_MQTT, &cfg::port_mqtt },
	{ Config_Type_String, sizeof(cfg::user_mqtt)-1, CFG_KEY_USER_MQTT, cfg::user_mqtt },
	{ Config_Type_Password, sizeof(cfg::pwd_mqtt)-1, CFG_KEY_PWD_MQTT, cfg::pwd_mqtt },
  { Config_Type_String, sizeof(cfg::measurement_name_mqtt)-1, CFG_KEY_MEASUREMENT_NAME_MQTT, cfg::measurement_name_mqtt },
  { Config_Type_String, sizeof(cfg::mqtt_main_topic)-1, CFG_KEY_MQTT_MAIN_TOPIC, cfg::mqtt_main_topic },
 	{ Config_Type_Bool, 0, CFG_KEY_ID_AS_MQTT_SUB_TOPIC, &cfg::id_as_mqtt_sub_topic },   
  { Config_Type_Bool, 0, CFG_KEY_TLS_MQTT, &cfg::tls_mqtt },
  { Config_Type_String, sizeof(cfg::ntp_1)-1, CFG_KEY_NTP_SERVER_1, cfg::ntp_1 },
  { Config_Type_String, sizeof(cfg::ntp_2)-1, CFG_KEY_NTP_SERVER_2, cfg::ntp_2 },
};
