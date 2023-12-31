// app.h

// written by: Oliver Cordes 2023-09-18
// changed by: Oliver Cordes 2023-11-12

#ifndef __APP_H__
#define __APP_H__

#include <Arduino.h>
#include <ArduinoJson.h>

#include "appInterface.h"

#include "defines.h"

#include "mqtt/mqtt_client.h"
#include "utils/scheduler.h"
#include "web/RestApi.h"
#include "web/web.h"
#include "wifi/basicwifi.h"

class System {

};


typedef Web<System> WebType;

class app : public IApp, public ei::Scheduler 
{
public:
    app();
    ~app() {}

    void setup();
    void loop();

    void onNetwork(bool gotIp);

private:
    void resetSystem(void);

    bool getProtection(AsyncWebServerRequest *request);
    void scanAvailNetworks();
    bool getAvailNetworks(JsonObject obj);
    bool getMqttIsConnected();
    bool getSavePending();
    bool getLastSaveSucceed();
    bool getShouldReboot();
    uint32_t getTimestamp();
    String getTimeStr(uint32_t offset = 0);
    uint32_t getTimezoneOffset();
    uint32_t getUptime();
    const char *getVersion();
    void regularTickers(void);
    bool saveSettings(bool reboot);
    void setRebootFlag();
    void setTimestamp(uint32_t newTime);
    void tickReboot(void);
    void tickSave(void);

    char mVersion[12];
    uint32_t mTimestamp;
    settings mSettings;
    settings_t *mConfig;
    bool mSavePending;
    bool mSaveReboot;
    bool mShowRebootRequest;

    basicwifi mWifi;
    WebType mWeb;
    RestApi mApi;

    // mqtt
    MqttEnClient mMqtt;
    //bool mMqttReconnect;
    bool mMqttEnabled;
};

#endif