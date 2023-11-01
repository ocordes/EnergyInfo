// config/settings.h

// written by: Oliver Cordes 2023-09-18
// changed by: Oliver Cordes 2023-09-18

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#undef F
#define F(sl) (sl)

/**
 * More info:
 * https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#flash-layout
 * */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "../defines.h"
#include "../utils/dbg.h"
#include "../utils/helper.h"

#define MAX_ALLOWED_BUF_SIZE ESP.getMaxAllocHeap() - 1024

typedef struct
{
    uint8_t ip[4];      // ip address
    uint8_t mask[4];    // sub mask
    uint8_t dns1[4];    // dns 1
    uint8_t dns2[4];    // dns 2
    uint8_t gateway[4]; // standard gateway
} cfgIp_t;


typedef struct
{
    char deviceName[DEVNAME_LEN];
    char adminPwd[PWD_LEN];
    uint16_t protectionMask;
    bool darkMode;
    bool schedReboot;
    // wifi
    char stationSsid[SSID_LEN];
    char stationPwd[PWD_LEN];
    char apPwd[PWD_LEN];
    bool isHidden;

    cfgIp_t ip;
} cfgSys_t;

typedef struct
{
    char addr[NTP_ADDR_LEN];
    uint16_t port;
    uint16_t interval; // in minutes
} cfgNtp_t;


typedef struct
{
    cfgSys_t sys;
    //cfgNrf24_t nrf;
    //cfgCmt_t cmt;
    cfgNtp_t ntp;
    //cfgSun_t sun;
    //cfgSerial_t serial;
    //cfgMqtt_t mqtt;
    //cfgLed_t led;
    //cfgInst_t inst;
    //plugins_t plugin;
    bool valid;
} settings_t;

class
    settings
{
public:
    settings()
    {
        mLastSaveSucceed = false;
    }

    void setup()
    {
        DPRINTLN(DBG_INFO, F("Initializing FS .."));

        mCfg.valid = false;

        #define LITTLFS_TRUE true
        #define LITTLFS_FALSE false

        if (!LittleFS.begin(LITTLFS_FALSE))
        {
            DPRINTLN(DBG_INFO, F(".. format .."));
            LittleFS.format();
            if (LittleFS.begin(LITTLFS_TRUE))
            {
                DPRINTLN(DBG_INFO, F(".. success"));
            }
            else
            {
                DPRINTLN(DBG_INFO, F(".. failed"));
            }
        }
        else
            DPRINTLN(DBG_INFO, F(" .. done"));

        readSettings("/settings.json");
    }

    // should be used before OTA
    void stop()
    {
        LittleFS.end();
        DPRINTLN(DBG_INFO, F("FS stopped"));
    }

    void getPtr(settings_t *&cfg)
    {
        cfg = &mCfg;
    }

    bool getValid(void)
    {
        return mCfg.valid;
    }

    inline bool getLastSaveSucceed()
    {
        return mLastSaveSucceed;
    }

    void getInfo(uint32_t *used, uint32_t *size)
    {
        DPRINTLN(DBG_WARN, F("not supported by ESP32"));
    }

    bool readSettings(const char *path)
    {
        loadDefaults();
        File fp = LittleFS.open(path, "r");
        if (!fp)
            DPRINTLN(DBG_WARN, F("failed to load json, using default config"));
        else
        {
            // DPRINTLN(DBG_INFO, fp.readString());
            // fp.seek(0, SeekSet);
            DynamicJsonDocument root(MAX_ALLOWED_BUF_SIZE);
            DeserializationError err = deserializeJson(root, fp);
            root.shrinkToFit();
            if (!err && (root.size() > 0))
            {
                mCfg.valid = true;
                if (root.containsKey(F("wifi")))
                    jsonNetwork(root[F("wifi")]);
                // if (root.containsKey(F("nrf")))
                //     jsonNrf(root[F("nrf")]);
                // if (root.containsKey(F("cmt")))
                //     jsonCmt(root[F("cmt")]);
                // if (root.containsKey(F("ntp")))
                //     jsonNtp(root[F("ntp")]);
                // if (root.containsKey(F("sun")))
                //     jsonSun(root[F("sun")]);
                // if (root.containsKey(F("serial")))
                //     jsonSerial(root[F("serial")]);
                // if (root.containsKey(F("mqtt")))
                //     jsonMqtt(root[F("mqtt")]);
                // if (root.containsKey(F("led")))
                //     jsonLed(root[F("led")]);
                // if (root.containsKey(F("plugin")))
                //     jsonPlugin(root[F("plugin")]);
                // if (root.containsKey(F("inst")))
                //     jsonInst(root[F("inst")]);
            }
            else
            {
                    Serial.println(F("failed to parse json, using default config"));
            }

            fp.close();
        }

        return mCfg.valid;
    }

    bool saveSettings()
    {
        DPRINTLN(DBG_DEBUG, F("save settings"));

        DynamicJsonDocument json(MAX_ALLOWED_BUF_SIZE);
        JsonObject root = json.to<JsonObject>();
        jsonNetwork(root.createNestedObject(F("wifi")), true);
        //jsonNrf(root.createNestedObject(F("nrf")), true);
        //jsonCmt(root.createNestedObject(F("cmt")), true);
        //jsonNtp(root.createNestedObject(F("ntp")), true);
        //jsonSun(root.createNestedObject(F("sun")), true);
        //jsonSerial(root.createNestedObject(F("serial")), true);
        //jsonMqtt(root.createNestedObject(F("mqtt")), true);
        //jsonLed(root.createNestedObject(F("led")), true);
        //jsonPlugin(root.createNestedObject(F("plugin")), true);
        //jsonInst(root.createNestedObject(F("inst")), true);

        DPRINT(DBG_INFO, F("memory usage: "));
        DBGPRINTLN(String(json.memoryUsage()));
        DPRINT(DBG_INFO, F("capacity: "));
        DBGPRINTLN(String(json.capacity()));
        DPRINT(DBG_INFO, F("max alloc: "));
        DBGPRINTLN(String(MAX_ALLOWED_BUF_SIZE));

        if (json.overflowed())
        {
            DPRINTLN(DBG_ERROR, F("buffer too small!"));
            mLastSaveSucceed = false;
            return false;
        }

        File fp = LittleFS.open("/settings.json", "w");
        if (!fp)
        {
            DPRINTLN(DBG_ERROR, F("can't open settings file!"));
            mLastSaveSucceed = false;
            return false;
        }

        if (0 == serializeJson(root, fp))
        {
            DPRINTLN(DBG_ERROR, F("can't write settings file!"));
            mLastSaveSucceed = false;
            return false;
        }
        fp.close();

        DPRINTLN(DBG_INFO, F("settings saved"));
        mLastSaveSucceed = true;
        return true;
    }

    bool eraseSettings(bool eraseWifi = false)
    {
        if (true == eraseWifi)
            return LittleFS.format();
        loadDefaults(!eraseWifi);
        return saveSettings();
    }

private:
    void loadDefaults(bool keepWifi = false)
    {
        DPRINTLN(DBG_VERBOSE, F("loadDefaults"));

        cfgSys_t tmp;
        if (keepWifi)
        {
            // copy contents which should not be deleted
            memset(&tmp.adminPwd, 0, PWD_LEN);
            memcpy(&tmp, &mCfg.sys, sizeof(cfgSys_t));
        }
        // erase all settings and reset to default
        memset(&mCfg, 0, sizeof(settings_t));
        mCfg.sys.protectionMask = DEF_PROT_INDEX | DEF_PROT_LIVE | DEF_PROT_SERIAL | DEF_PROT_SETUP | DEF_PROT_UPDATE | DEF_PROT_SYSTEM | DEF_PROT_API | DEF_PROT_MQTT;
        mCfg.sys.darkMode = false;
        mCfg.sys.schedReboot = false;
        // restore temp settings
        if (keepWifi)
            memcpy(&mCfg.sys, &tmp, sizeof(cfgSys_t));
        else
        {
            snprintf(mCfg.sys.stationSsid, SSID_LEN, FB_WIFI_SSID);
            snprintf(mCfg.sys.stationPwd, PWD_LEN, FB_WIFI_PWD);
            snprintf(mCfg.sys.apPwd, PWD_LEN, WIFI_AP_PWD);
            mCfg.sys.isHidden = false;
        }

        snprintf(mCfg.sys.deviceName, DEVNAME_LEN, DEF_DEVICE_NAME);

        // OC NTP
        // snprintf(mCfg.ntp.addr, NTP_ADDR_LEN, "%s", DEF_NTP_SERVER_NAME);
        // mCfg.ntp.port = DEF_NTP_PORT;
        // mCfg.ntp.interval = 720;


        // mCfg.serial.interval = SERIAL_INTERVAL;
        // mCfg.serial.showIv = false;
        // mCfg.serial.debug = false;

        // mCfg.mqtt.port = DEF_MQTT_PORT;
        // snprintf(mCfg.mqtt.broker, MQTT_ADDR_LEN, "%s", DEF_MQTT_BROKER);
        // snprintf(mCfg.mqtt.user, MQTT_USER_LEN, "%s", DEF_MQTT_USER);
        // snprintf(mCfg.mqtt.pwd, MQTT_PWD_LEN, "%s", DEF_MQTT_PWD);
        // snprintf(mCfg.mqtt.topic, MQTT_TOPIC_LEN, "%s", DEF_MQTT_TOPIC);
        // mCfg.mqtt.interval = 0; // off
    }

    void jsonNetwork(JsonObject obj, bool set = false)
    {
        if (set)
        {
            char buf[16];

            obj[F("ssid")] = mCfg.sys.stationSsid;
            obj[F("pwd")] = mCfg.sys.stationPwd;
            obj[F("ap_pwd")] = mCfg.sys.apPwd;
            obj[F("hidd")] = (bool)mCfg.sys.isHidden;

            obj[F("dev")] = mCfg.sys.deviceName;
            obj[F("adm")] = mCfg.sys.adminPwd;
            obj[F("prot_mask")] = mCfg.sys.protectionMask;
            obj[F("dark")] = mCfg.sys.darkMode;
            obj[F("reb")] = (bool)mCfg.sys.schedReboot;
            ah::ip2Char(mCfg.sys.ip.ip, buf);
            obj[F("ip")] = String(buf);
            ah::ip2Char(mCfg.sys.ip.mask, buf);
            obj[F("mask")] = String(buf);
            ah::ip2Char(mCfg.sys.ip.dns1, buf);
            obj[F("dns1")] = String(buf);
            ah::ip2Char(mCfg.sys.ip.dns2, buf);
            obj[F("dns2")] = String(buf);
            ah::ip2Char(mCfg.sys.ip.gateway, buf);
            obj[F("gtwy")] = String(buf);
        }
        else
        {
            getChar(obj, F("ssid"), mCfg.sys.stationSsid, SSID_LEN);
            getChar(obj, F("pwd"), mCfg.sys.stationPwd, PWD_LEN);
            getChar(obj, F("ap_pwd"), mCfg.sys.apPwd, PWD_LEN);
            getVal<bool>(obj, F("hidd"), &mCfg.sys.isHidden);
            if (obj.containsKey(F("ip")))
                ah::ip2Arr(mCfg.sys.ip.ip, obj[F("ip")].as<const char *>());
            if (obj.containsKey(F("mask")))
                ah::ip2Arr(mCfg.sys.ip.mask, obj[F("mask")].as<const char *>());
            if (obj.containsKey(F("dns1")))
                ah::ip2Arr(mCfg.sys.ip.dns1, obj[F("dns1")].as<const char *>());
            if (obj.containsKey(F("dns2")))
                ah::ip2Arr(mCfg.sys.ip.dns2, obj[F("dns2")].as<const char *>());
            if (obj.containsKey(F("gtwy")))
                ah::ip2Arr(mCfg.sys.ip.gateway, obj[F("gtwy")].as<const char *>());

            if (mCfg.sys.protectionMask == 0)
                mCfg.sys.protectionMask = DEF_PROT_INDEX | DEF_PROT_LIVE | DEF_PROT_SERIAL | DEF_PROT_SETUP | DEF_PROT_UPDATE | DEF_PROT_SYSTEM | DEF_PROT_API | DEF_PROT_MQTT;
        }
    }

    void getChar(JsonObject obj, const char *key, char *dst, int maxLen)
    {
        if (obj.containsKey(key))
            snprintf(dst, maxLen, "%s", obj[key].as<const char *>());
    }

    template <typename T = uint8_t>
    void getVal(JsonObject obj, const char *key, T *dst)
    {
        if (obj.containsKey(key))
            *dst = obj[key];
    }
 
    settings_t mCfg;
    bool mLastSaveSucceed;
};

#endif