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

#endif /*__CONFIG_H__*/