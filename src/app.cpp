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

    // startup code
    // WIFI
    mWifi.setup(mConfig, &mTimestamp, std::bind(&app::onNetwork, this, std::placeholders::_1));

    mWeb.setup(this, mConfig);
    mWeb.setProtection(strlen(mConfig->sys.adminPwd) != 0);

    mApi.setup(this, mWeb.getWebSrvPtr(), mConfig);

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

}

//-----------------------------------------------------------------------------
void app::onNetwork(bool gotIp)
{
    DPRINTLN(DBG_DEBUG, F("onNetwork"));
    //ah::Scheduler::resetTicker();
    //regularTickers(); // reinstall regular tickers
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

const char* app::getVersion()
{
    return mVersion;
}

uint32_t app::getTimestamp()
{
    //return Scheduler::getTimestamp();
    return mTimestamp;
}