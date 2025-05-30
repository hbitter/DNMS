const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_CSS[] PROGMEM = "text/css";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM = "image/png";

const char DBG_TXT_TEMPERATURE[] PROGMEM = "Temperature (°C): ";
const char DBG_TXT_HUMIDITY[] PROGMEM = "Humidity (%): ";
const char DBG_TXT_PRESSURE[] PROGMEM = "Pressure (hPa): ";
const char DBG_TXT_CO2PPM[] PROGMEM = "CO₂ (ppm): ";
const char DBG_TXT_VOCINDEX[] PROGMEM = "VOC index : ";
const char DBG_TXT_NOX[] PROGMEM = "NOx index: ";
const char DBG_TXT_START_READING[] PROGMEM = "R/ ";
const char DBG_TXT_END_READING[] PROGMEM = "/R ";
const char DBG_TXT_CHECKSUM_IS[] PROGMEM = "Checksum is: ";
const char DBG_TXT_CHECKSUM_SHOULD[] PROGMEM = "Checksum should: ";
const char DBG_TXT_DATA_READ_FAILED[] PROGMEM = "Data read failed";
const char DBG_TXT_UPDATE[] PROGMEM = "[update] ";
const char DBG_TXT_UPDATE_FAILED[] PROGMEM = "Update failed.";
const char DBG_TXT_UPDATE_NO_UPDATE[] PROGMEM = "No update.";
const char DBG_TXT_SENDING_TO[] PROGMEM = "## Sending to ";
const char DBG_TXT_SDS011_VERSION_DATE[] PROGMEM = "SDS011 version date";
const char DBG_TXT_NPM_VERSION_DATE[] PROGMEM = "Next PM version date";
const char DBG_TXT_CONNECTING_TO[] PROGMEM = "Connecting to ";
const char DBG_TXT_FOUND[] PROGMEM = " ... found";
const char DBG_TXT_NOT_FOUND[] PROGMEM = " ... not found";
const char DBG_TXT_SEP[] PROGMEM = "----";

const char SENSORS_SDS011[] PROGMEM = "SDS011";
const char SENSORS_PPD42NS[] PROGMEM = "PPD42NS";
const char SENSORS_PMSx003[] PROGMEM = "PMSx003";
const char SENSORS_HPM[] PROGMEM = "Honeywell PM";
const char SENSORS_NPM[] PROGMEM = "Tera Sensor Next PM";
const char SENSORS_IPS[] PROGMEM = "Piera Systems IPS-7100";
const char SENSORS_SEN5X[] PROGMEM = "Sensirion SEN5X";
const char SENSORS_SEN50[] PROGMEM = "Sensirion SEN50";
const char SENSORS_SEN54[] PROGMEM = "Sensirion SEN54";
const char SENSORS_SEN55[] PROGMEM = "Sensirion SEN55";
const char SENSORS_SPS30[] PROGMEM = "Sensirion SPS30";
const char SENSORS_DHT22[] PROGMEM = "DHT22";
const char SENSORS_DS18B20[] PROGMEM = "DS18B20";
const char SENSORS_HTU21D[] PROGMEM = "HTU21D";
const char SENSORS_SHT3X[] PROGMEM = "SHT3x";
const char SENSORS_SCD30[] PROGMEM = "SCD30";
const char SENSORS_SCD4X[] PROGMEM = "SCD4X";
const char SENSORS_BMP180[] PROGMEM = "BMP180";
const char SENSORS_BME280[] PROGMEM = "BME280";
const char SENSORS_BMP280[] PROGMEM = "BMP280";
const char SENSORS_DNMS[] PROGMEM = "DNMS";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html lang='" INTL_LANG "'>\
<head>\
<meta charset='utf-8'/>\
<title>{t}</title>";

const char WEB_PAGE_STATIC_CSS[] PROGMEM = "\
body{font-family:Arial,sans-serif;margin:0}\
.content{margin:10px}\
.footer{height:48px;background:#f5f5f5;width:100%}\
.r{text-align:right}\
td{vertical-align:top}\
.v>tbody>tr:nth-child(odd){background:#efefef}\
.b{text-decoration:none;padding:10px;background:#2a9;color:#fff;display:block;width:auto;border-radius:5px;}\
.wifi{background:0 0;color:#00f;padding:5px;display:inline;border:0;}\
input[type=text]{width:100%}\
input[type=password]{width:100%}\
input[type=submit]{color:#fff;text-align:left;cursor:pointer;border-radius:5px;font-size:medium;background:#b33;padding:9px!important;width:100%;border-style:none}\
input[type=submit]:hover{background:#d44}\
.s_green{padding:9px !important;width:100%;border-style:none;background:#3ba;color:#fff;text-align:left}\
.tabs{display:flex;flex-direction:row;align-items:stretch;align-content:flex-end;justify-content:flex-start}\
.tab{padding:10px 20px;display:inline-block;color:#333}\
.panels{min-height:200px;overflow:hidden;padding:20px;border:2px solid #3ba;margin-bottom:1em;}\
.radio{display:none}.panel{display:none}\
.canvas{min-height:118px;background:#3ba;margin-bottom:20px;}\
#r5:checked~.panels>#panel5,#r4:checked~.panels>#panel4,#r1:checked~.panels>#panel1,#r3:checked~.panels>#panel3,#r2:checked~.panels>#panel2{display:block}\
#r5:checked~.tabs>#tab5,#r4:checked~.tabs>#tab4,#r1:checked~.tabs>#tab1,#r3:checked~.tabs>#tab3,#r2:checked~.tabs>#tab2{background:#3ba;color:#fff}";

#define STATIC_PREFIX "/" INTL_LANG "_s1"

const char WEB_PAGE_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>\
<meta name='theme-color' content='#3ba'>\
<link rel='stylesheet' href='" STATIC_PREFIX "?r=css'>\
</style>\
</head><body>\
<div class='canvas'>\
<a class='b' href='/' style='background:none;display:inline'>\
<img src='" STATIC_PREFIX "?r=logo' alt='" INTL_BACK_TO_HOME "' style='float:left;margin:16px' width='100' height='89'/></a>";

const char WEB_PAGE_HEADER_BODY[] PROGMEM = "<h3 style='margin:0 10px; color:#fff;'>" INTL_PM_SENSOR "</h3>\
<br/><small style='color:#fff;font-weight:700'>ID: {id} ({macid})<br/>" INTL_FIRMWARE ": " SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")<br/>\
</small></div><div class='content'><h4>" INTL_HOME " {n} {t}</h4>";

const char BR_TAG[] PROGMEM = "<br/>";
const char WEB_DIV_PANEL[] PROGMEM = "</div><div class='panel' id='panel{v}'>";
const char TABLE_TAG_OPEN[] PROGMEM = "<table>";
const char TABLE_TAG_CLOSE_BR[] PROGMEM = "</table>";
const char EMPTY_ROW[] PROGMEM = "<tr><td colspan='3'>&nbsp;</td></tr>";

const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/>"
                "<a class='b' href='/' style='display:inline;'>" INTL_BACK_TO_HOME "</a><br/><br/><br/>"
		"</div><footer class='footer'><div style='padding:16px'>"
		"<a href='https://codefor.de/stuttgart/' target='_blank' rel='noreferrer' style='color:#3ba;'>"
		"&copy; Open Knowledge Lab Stuttgart a.o. (Code for Germany)</a>&nbsp;&nbsp("
                "<a href='https://github.com/opendata-stuttgart/sensors-software/labels/bug' target='_blank' rel='noreferrer'>" INTL_REPORT_ISSUE "</a>"
		")</div></footer></body></html>\r\n";

const char WEB_ROOT_PAGE_CONTENT[] PROGMEM = "<a class='b' href='/values'>{t}</a><br/>\
<a class='b' href='/status'>{s}</a><br/>\
<a class='b' href='https://maps.sensor.community/' target='_blank' rel='noreferrer'>" INTL_ACTIVE_SENSORS_MAP "</a><br/>\
<a class='b' href='/config'>{conf}</a><br/>\
<a class='b' href='/removeConfig'>" INTL_CONFIGURATION_DELETE "</a><br/>\
<a class='b' href='/reset'>{restart}</a><br/>\
<a class='b' href='/debug'>{debug}</a><br/>";

const char WEB_CONFIG_SCRIPT[] PROGMEM = "<script>\
function setSSID(ssid){document.getElementById('wlanssid').value=ssid.innerText||ssid.textContent;document.getElementById('wlanpwd').focus();}\
function load_wifi_list(){var x=new XMLHttpRequest();x.open('GET','/wifi');x.onload = function(){if (x.status === 200) {document.getElementById('wifilist').innerHTML = x.responseText;}};x.send();}\
</script>";

const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM = "<h3>" INTL_CONFIGURATION_REALLY_DELETE "</h3>\
<table><tr><td><form method='POST' action='/removeConfig'>\
<input type='submit' class='s_red' name='submit' value='" INTL_DELETE "'/></form></td>\
<td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>";

const char WEB_RESET_CONTENT[] PROGMEM = "<h3>" INTL_REALLY_RESTART_SENSOR "</h3>" \
"<table><tr><td><form method='POST' action'/reset'>" \
"<input type='submit' class='s_red' name='submit' value='" INTL_RESTART "'/>"\
"</form></td><td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>";

const char WEB_IOS_REDIRECT[] PROGMEM = "<html><body>Redirecting...\
<script type=\"text/javascript\">\
window.location = \"http://192.168.4.1/config\";\
</script>\
</body></html>";

const char WEB_B_BR_BR[] PROGMEM = "</b><br/><br/>";
const char WEB_BRACE_BR[] PROGMEM = ")<br/>";
const char WEB_B_BR[] PROGMEM = "</b><br/>";
const char WEB_BR_BR[] PROGMEM = "<br/><br/>";
const char WEB_BR_FORM[] PROGMEM = "<br/></form>";
const char WEB_BR_LF_B[] PROGMEM = "<br/>\n<b>";
const char WEB_LF_B[] PROGMEM = "\n<b>";
const char WEB_CSV[] PROGMEM = "CSV";
const char WEB_FEINSTAUB_APP[] PROGMEM = "<a target='_blank' href='https://chillibits.com/pmapp'>Feinstaub-App</a>";
const char WEB_OPENSENSEMAP[] PROGMEM = "<a target='_blank' href='https://opensensemap.org/about'>OpenSenseMap.org</a>";
const char WEB_AIRCMS[] PROGMEM = "<a target='_blank' href='https://aircms.online/#/mission'>aircms.online</a>";
const char WEB_MADAVI[] PROGMEM = "<a target='_blank' href='https://www.madavi.de/ok-lab-stuttgart/'>Madavi.de</a>";
const char WEB_SENSORCOMMUNITY[] PROGMEM = "<a target='_blank' href='https://archive.sensor.community/00disclamer.md'>Sensor.Community</a>";
const char WEB_HTTPS[] PROGMEM = "HTTPS";
const char WEB_NBSP_NBSP_BRACE[] PROGMEM = "&nbsp;&nbsp;(";
const char WEB_REPLN_REPLV[] PROGMEM = "\"{n}\":\"{v}\",";
const char WEB_PM1[] PROGMEM = "PM1";
const char WEB_PM25[] PROGMEM = "PM2.5";
const char WEB_PM10[] PROGMEM = "PM10";
const char WEB_PM4[] PROGMEM = "PM4";
const char WEB_PM01[] PROGMEM = "PM0.1";
const char WEB_PM03[] PROGMEM = "PM0.3";
const char WEB_PM05[] PROGMEM = "PM0.5";
const char WEB_PM5[] PROGMEM = "PM5";
const char WEB_NC0k1[] PROGMEM = "NC0.1";
const char WEB_NC0k3[] PROGMEM = "NC0.3";
const char WEB_NC0k5[] PROGMEM = "NC0.5";
const char WEB_NC1k0[] PROGMEM = "NC1.0";
const char WEB_NC2k5[] PROGMEM = "NC2.5";
const char WEB_NC4k0[] PROGMEM = "NC4.0";
const char WEB_NC5k0[] PROGMEM = "NC5.0";
const char WEB_NC10[] PROGMEM = "NC10";
const char WEB_TPS[] PROGMEM = "TPS";
const char WEB_GPS[] PROGMEM = "GPS";

const char * WEB_LAeq_HZ[] PROGMEM = {"DNMS_noise_LAeq20","DNMS_noise_LAeq25","DNMS_noise_LAeq31.5","DNMS_noise_LAeq40","DNMS_noise_LAeq50",
  "DNMS_noise_LAeq63","DNMS_noise_LAeq80","DNMS_noise_LAeq100","DNMS_noise_LAeq125","DNMS_noise_LAeq160","DNMS_noise_LAeq200","DNMS_noise_LAeq250",
  "DNMS_noise_LAeq315","DNMS_noise_LAeq400","DNMS_noise_LAeq500","DNMS_noise_LAeq630","DNMS_noise_LAeq800","DNMS_noise_LAeq1000","DNMS_noise_LAeq1250",
  "DNMS_noise_LAeq1600","DNMS_noise_LAeq2000","DNMS_noise_LAeq2500","DNMS_noise_LAeq3150","DNMS_noise_LAeq4000","DNMS_noise_LAeq5000",
  "DNMS_noise_LAeq6300","DNMS_noise_LAeq8000","DNMS_noise_LAeq10000","DNMS_noise_LAeq12500","DNMS_noise_LAeq16000","DNMS_noise_LAeq20000"}; 
const char * WEB_LAeq_HZ_2[] PROGMEM = {"LAeq20: ","LAeq25: ","LAeq31.5: ","LAeq40: ","LAeq50: ","LAeq63: ","LAeq80: ","LAeq100: ","LAeq125: ","LAeq160: ",
  "LAeq200: ","LAeq250: ","LAeq315: ","LAeq400: ","LAeq500: ","LAeq630: ","LAeq800: ","LAeq1000: ","LAeq1250: ","LAeq1600: ","LAeq2000: ","LAeq2500: ","LAeq3150: ",
  "LAeq4000: ","LAeq5000: ","LAeq6300: ","LAeq8000: ","LAeq10000: ","LAeq12500: ","LAeq16000: ","LAeq20000: "};  
const char * WEB_LAeq_HZ_2nd[] PROGMEM = {"DNMS_noise_LAeq20_2nd","DNMS_noise_LAeq25_2nd","DNMS_noise_LAeq31.5_2nd","DNMS_noise_LAeq40_2nd","DNMS_noise_LAeq50_2nd",
  "DNMS_noise_LAeq63_2nd","DNMS_noise_LAeq80_2nd","DNMS_noise_LAeq100_2nd","DNMS_noise_LAeq125_2nd","DNMS_noise_LAeq160_2nd","DNMS_noise_LAeq200_2nd","DNMS_noise_LAeq250_2nd",
  "DNMS_noise_LAeq315_2nd","DNMS_noise_LAeq400_2nd","DNMS_noise_LAeq500_2nd","DNMS_noise_LAeq630_2nd","DNMS_noise_LAeq800_2nd","DNMS_noise_LAeq1000_2nd","DNMS_noise_LAeq1250_2nd",
  "DNMS_noise_LAeq1600_2nd","DNMS_noise_LAeq2000_2nd","DNMS_noise_LAeq2500_2nd","DNMS_noise_LAeq3150_2nd","DNMS_noise_LAeq4000_2nd","DNMS_noise_LAeq5000_2nd",
  "DNMS_noise_LAeq6300_2nd","DNMS_noise_LAeq8000_2nd","DNMS_noise_LAeq10000_2nd","DNMS_noise_LAeq12500_2nd","DNMS_noise_LAeq16000_2nd","DNMS_noise_LAeq20000_2nd"}; 
const char * WEB_LAeq_HZ_2_2nd[] PROGMEM = {"LAeq20_2nd: ","LAeq25_2nd: ","LAeq31.5_2nd: ","LAeq40_2nd: ","LAeq50_2nd: ","LAeq63_2nd: ","LAeq80_2nd: ","LAeq100_2nd: ","LAeq125_2nd: ","LAeq160_2nd: ",
  "LAeq200_2nd: ","LAeq250_2nd: ","LAeq315_2nd: ","LAeq400_2nd: ","LAeq500_2nd: ","LAeq630_2nd: ","LAeq800_2nd: ","LAeq1000_2nd: ","LAeq1250_2nd: ","LAeq1600_2nd: ","LAeq2000_2nd: ","LAeq2500_2nd: ","LAeq3150_2nd: ",
  "LAeq4000_2nd: ","LAeq5000_2nd: ","LAeq6300_2nd: ","LAeq8000_2nd: ","LAeq10000_2nd: ","LAeq12500_2nd: ","LAeq16000_2nd: ","LAeq20000_2nd: "}; 

const char * WEB_LZeq_HZ[] PROGMEM = {"DNMS_noise_LZeq20","DNMS_noise_LZeq25","DNMS_noise_LZeq31.5","DNMS_noise_LZeq40","DNMS_noise_LZeq50",
  "DNMS_noise_LZeq63","DNMS_noise_LZeq80","DNMS_noise_LZeq100","DNMS_noise_LZeq125","DNMS_noise_LZeq160","DNMS_noise_LZeq200","DNMS_noise_LZeq250",
  "DNMS_noise_LZeq315","DNMS_noise_LZeq400","DNMS_noise_LZeq500","DNMS_noise_LZeq630","DNMS_noise_LZeq800","DNMS_noise_LZeq1000","DNMS_noise_LZeq1250",
  "DNMS_noise_LZeq1600","DNMS_noise_LZeq2000","DNMS_noise_LZeq2500","DNMS_noise_LZeq3150","DNMS_noise_LZeq4000","DNMS_noise_LZeq5000",
  "DNMS_noise_LZeq6300","DNMS_noise_LZeq8000","DNMS_noise_LZeq10000","DNMS_noise_LZeq12500","DNMS_noise_LZeq16000","DNMS_noise_LZeq20000"}; 
const char * WEB_LZeq_HZ_2[] PROGMEM = {"LZeq20: ","LZeq25: ","LZeq31.5: ","LZeq40: ","LZeq50: ","LZeq63: ","LZeq80: ","LZeq100: ","LZeq125: ","LZeq160: ",
  "LZeq200: ","LZeq250: ","LZeq315: ","LZeq400: ","LZeq500: ","LZeq630: ","LZeq800: ","LZeq1000: ","LZeq1250: ","LZeq1600: ","LZeq2000: ","LZeq2500: ","LZeq3150: ",
  "LZeq4000: ","LZeq5000: ","LZeq6300: ","LZeq8000: ","LZeq10000: ","LZeq12500: ","LZeq16000: ","LZeq20000: "};  
const char * WEB_LZeq_HZ_2nd[] PROGMEM = {"DNMS_noise_LZeq20_2nd","DNMS_noise_LZeq25_2nd","DNMS_noise_LZeq31.5_2nd","DNMS_noise_LZeq40_2nd","DNMS_noise_LZeq50_2nd",
  "DNMS_noise_LZeq63_2nd","DNMS_noise_LZeq80_2nd","DNMS_noise_LZeq100_2nd","DNMS_noise_LZeq125_2nd","DNMS_noise_LZeq160_2nd","DNMS_noise_LZeq200_2nd","DNMS_noise_LZeq250_2nd",
  "DNMS_noise_LZeq315_2nd","DNMS_noise_LZeq400_2nd","DNMS_noise_LZeq500_2nd","DNMS_noise_LZeq630_2nd","DNMS_noise_LZeq800_2nd","DNMS_noise_LZeq1000_2nd","DNMS_noise_LZeq1250_2nd",
  "DNMS_noise_LZeq1600_2nd","DNMS_noise_LZeq2000_2nd","DNMS_noise_LZeq2500_2nd","DNMS_noise_LZeq3150_2nd","DNMS_noise_LZeq4000_2nd","DNMS_noise_LZeq5000_2nd",
  "DNMS_noise_LZeq6300_2nd","DNMS_noise_LZeq8000_2nd","DNMS_noise_LZeq10000_2nd","DNMS_noise_LZeq12500_2nd","DNMS_noise_LZeq16000_2nd","DNMS_noise_LZeq20000_2nd"}; 
const char * WEB_LZeq_HZ_2_2nd[] PROGMEM = {"LZeq20_2nd: ","LZeq25_2nd: ","LZeq31.5_2nd: ","LZeq40_2nd: ","LZeq50_2nd: ","LZeq63_2nd: ","LZeq80_2nd: ","LZeq100_2nd: ","LZeq125_2nd: ","LZeq160_2nd: ",
  "LZeq200_2nd: ","LZeq250_2nd: ","LZeq315_2nd: ","LZeq400_2nd: ","LZeq500_2nd: ","LZeq630_2nd: ","LZeq800_2nd: ","LZeq1000_2nd: ","LZeq1250_2nd: ","LZeq1600_2nd: ","LZeq2000_2nd: ","LZeq2500_2nd: ","LZeq3150_2nd: ",
  "LZeq4000_2nd: ","LZeq5000_2nd: ","LZeq6300_2nd: ","LZeq8000_2nd: ","LZeq10000_2nd: ","LZeq12500_2nd: ","LZeq16000_2nd: ","LZeq20000_2nd: "};
