// app.cpp

// written by: Oliver Cordes 2023-09-18
// changed by: Oliver Cordes 2023-09-18

#include "app.h"


#define USE_LittleFS

#include <FS.h>
#ifdef USE_LittleFS
#define SPIFFS LittleFS
#include <LittleFS.h>
#else
#include <SPIFFS.h>
#endif

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("− failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" − not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                Serial.println("------------------------------------------------");
                listDir(fs, file.path(), levels - 1);
                Serial.println("------------------------------------------------");
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

app::app()
    : ei::Scheduler{}
{

}


void app::setup()
{
    char msg[40];

    // start serial console
    Serial.begin(115200);
    while (!Serial)
        yield();

    

    resetSystem();


    // print version
    Serial.print("Welcome to EnergyInfo: ");
    Serial.println(mVersion);

        // enable settings
        mSettings.setup();
    mSettings.getPtr(mConfig);

    ei::Scheduler::setup(true);
    DPRINT(DBG_INFO, F("Settings valid: "));
    if (mSettings.getValid())
        DBGPRINTLN(F("true"));
    else
        DBGPRINTLN(F("false"));

    // startup code
    // WIFI
    //mInnerLoopCb = std::bind(&app::loopWifi, this);
    mWifi.setup(mConfig, &mTimestamp, std::bind(&app::onNetwork, this, std::placeholders::_1));
    everySec(std::bind(&basicwifi::tickWifiLoop, &mWifi), "wifiL");

    mWeb.setup(this, mConfig);
    mWeb.setProtection(strlen(mConfig->sys.adminPwd) != 0);

    mApi.setup(this, mWeb.getWebSrvPtr(), mConfig);

    mMqttEnabled = (mConfig->mqtt.broker[0] > 0);
    if (mMqttEnabled)
    {
        mMqtt.setup(&mConfig->mqtt, mConfig->sys.deviceName, mVersion, &mTimestamp, &mUptime);
        //mMqtt.setSubscriptionCb(std::bind(&app::mqttSubRxCb, this, std::placeholders::_1));
    }

        // if (!SPIFFS.begin(true))
        // {
        //     Serial.println("An Error has occurred while mounting SPIFFS");
        //     return;
        // }

        // delay(1000);

        // listDir(SPIFFS, "/", 3);

        // File file = SPIFFS.open("/test.txt", "r");
        // if (!file)
        // {
        //     Serial.println("Failed to open file for reading");
        //     Serial.flush();
        //     return;
        // }

        // Serial.println("File Content:");
        // while (file.available())
        // {
        //     Serial.write(file.read());
        // }
        // Serial.flush();
        // file.close();

        // Serial.println("Setup complete!");
    }

void app::loop()
{
    ei::Scheduler::loop();
}

//-----------------------------------------------------------------------------
void app::onNetwork(bool gotIp)
{
    DPRINTLN(DBG_DEBUG, F("onNetwork"));
    ei::Scheduler::resetTicker();
    regularTickers(); // reinstall regular tickers
    if (gotIp)
    {
    }
}

bool app::getProtection(AsyncWebServerRequest *request)
{
    return mWeb.isProtected(request);
}

void app::scanAvailNetworks()
{
    mWifi.scanAvailNetworks();
}

void app::resetSystem(void)
{
    snprintf(mVersion, 12, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

bool app::getAvailNetworks(JsonObject obj)
{
    return mWifi.getAvailNetworks(obj);
}

bool app::getMqttIsConnected()
{
    return mMqtt.isConnected();
}

bool app::getSavePending()
{
    return mSavePending;
}

bool app::getLastSaveSucceed()
{
    return mSettings.getLastSaveSucceed();
}

bool app::getShouldReboot()
{
    return mSaveReboot;
}

uint32_t app::getTimestamp()
{
    return Scheduler::getTimestamp();
    //return mTimestamp;
}

String app::getTimeStr(uint32_t offset)
{
    char str[10];
    if (0 == mTimestamp)
        sprintf(str, "n/a");
    else
        sprintf(str, "%02d:%02d:%02d ", hour(mTimestamp + offset), minute(mTimestamp + offset), second(mTimestamp + offset));
    return String(str);
}

uint32_t app::getTimezoneOffset()
{
    return mApi.getTimezoneOffset();
}

uint32_t app::getUptime()
{
    return Scheduler::getUptime();
}

const char *app::getVersion()
{
    return mVersion;
}

void app::regularTickers(void)
{
    DPRINTLN(DBG_DEBUG, F("regularTickers"));
    everySec(std::bind(&WebType::tickSecond, &mWeb), "webSc");
    // Plugins
    //OC if (mConfig->plugin.display.type != 0)
    //OC    everySec(std::bind(&DisplayType::tickerSecond, &mDisplay), "disp");
    //OC every(std::bind(&PubSerialType::tick, &mPubSerial), mConfig->serial.interval, "uart");
}

bool app::saveSettings(bool reboot)
{
    mShowRebootRequest = true; // only message on index, no reboot
    mSavePending = true;
    mSaveReboot = reboot;
    if (reboot)
    {
        onNetwork(false);
        ei::Scheduler::resetTicker();
    }
    once(std::bind(&app::tickSave, this), 3, "save");
    return true;
}

void app::setRebootFlag()
{
    once(std::bind(&app::tickReboot, this), 3, "rboot");
}

void app::setTimestamp(uint32_t newTime)
{
    DPRINT(DBG_DEBUG, F("setTimestamp: "));
    DBGPRINTLN(String(newTime));
    if (0 == newTime)
    {
#if defined(ETHERNET)
        mEth.updateNtpTime();
#else  /* defined(ETHERNET) */
        mWifi.getNtpTime();
#endif /* defined(ETHERNET) */
    }
    else
        Scheduler::setTimestamp(newTime);
}

void app::tickReboot(void)
{
    DPRINTLN(DBG_INFO, F("Rebooting..."));
    onNetwork(false);
    ei::Scheduler::resetTicker();
    WiFi.disconnect();
    delay(200);
    ESP.restart();
}

void app::tickSave(void)
{
    if (!mSettings.saveSettings())
        mSaveReboot = false;
    mSavePending = false;

    if (mSaveReboot)
        setRebootFlag();
}
