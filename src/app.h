// app.h

// written by: Oliver Cordes 2023-09-18
// changed by: Oliver Cordes 2023-09-18

#ifndef __APP_H__
#define __APP_H__

#include <Arduino.h>
#include <ArduinoJson.h>

#include "appInterface.h"

#include "defines.h"

#include "web/RestApi.h"
#include "web/web.h"
#include "wifi/basicwifi.h"

class System {

};


typedef Web<System> WebType;

class app : public IApp {
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
    const char *getVersion();
    uint32_t getTimestamp();

    char mVersion[12];
    uint32_t mTimestamp;
    settings mSettings;
    settings_t *mConfig;
    basicwifi mWifi;
    WebType mWeb;
    RestApi mApi;
};

#endif