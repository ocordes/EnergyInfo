// defines.h

// written by: Oliver Cordes 2023-09-18
// changed by: Oliver Cordes 2023-11-12

#ifndef __DEFINES_H__
#define __DEFINES_H__

//#include "config/config.h"

//-------------------------------------
// VERSION
//-------------------------------------
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 1

//-------------------------------------
// EEPROM
//-------------------------------------
#define SSID_LEN 32
#define PWD_LEN 64
#define DEVNAME_LEN 16
#define NTP_ADDR_LEN 32 // DNS Name

#define MQTT_ADDR_LEN 64     // DNS Name
#define MQTT_CLIENTID_LEN 22 // number of chars is limited to 23 up to v3.1 of MQTT
#define MQTT_USER_LEN 65     // there is another byte necessary for \0
#define MQTT_PWD_LEN 65

#endif