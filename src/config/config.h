#ifndef __CONFIG_H__
#define __CONFIG_H__

//-------------------------------------
// WIFI CONFIGURATION
//-------------------------------------

// Fallback WiFi Info
#define FB_WIFI_SSID "YOUR_WIFI_SSID"
#define FB_WIFI_PWD "YOUR_WIFI_PWD"

// Access Point Info
// In case there is no WiFi Network or Ahoy can not connect to it, it will act as an Access Point

#define WIFI_AP_SSID "MonsterAP"
#define WIFI_AP_PWD "monster"

// default device name
#define DEF_DEVICE_NAME "EnergyInfo"

#define LOGOUT_TIMEOUT (20 * 60)

// Timezone
#define TIMEZONE 1

// default NTP server uri
#define DEF_NTP_SERVER_NAME "pool.ntp.org"

// default NTP server port
#define DEF_NTP_PORT 123

// NTP refresh interval in ms (default 12h)
#define NTP_REFRESH_INTERVAL 12 * 3600 * 1000


// check if necessary
#define PROT_MASK_INDEX 0x0001
#define PROT_MASK_LIVE 0x0002
#define PROT_MASK_SERIAL 0x0004
#define PROT_MASK_SETUP 0x0008
#define PROT_MASK_UPDATE 0x0010
#define PROT_MASK_SYSTEM 0x0020
#define PROT_MASK_API 0x0040
#define PROT_MASK_MQTT 0x0080

#define DEF_PROT_INDEX 0x0001
#define DEF_PROT_LIVE 0x0000
#define DEF_PROT_SERIAL 0x0004
#define DEF_PROT_SETUP 0x0008
#define DEF_PROT_UPDATE 0x0010
#define DEF_PROT_SYSTEM 0x0020
#define DEF_PROT_API 0x0000
#define DEF_PROT_MQTT 0x0000

// default mqtt interval
#define MQTT_INTERVAL 90

// default MQTT broker uri
#define DEF_MQTT_BROKER "\0"

// default MQTT port
#define DEF_MQTT_PORT 1883

// default MQTT user
#define DEF_MQTT_USER "\0"

// default MQTT pwd
#define DEF_MQTT_PWD "\0"

// discovery prefix
#define MQTT_DISCOVERY_PREFIX "homeassistant"

// reconnect delay
#define MQTT_RECONNECT_DELAY 5000

#endif /*__CONFIG_H__*/