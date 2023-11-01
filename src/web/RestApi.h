//-----------------------------------------------------------------------------
// 2023 Ahoy, https://ahoydtu.de
// Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//-----------------------------------------------------------------------------

#ifndef __WEB_API_H__
#define __WEB_API_H__

#include "../config/config.h"
#include "../config/settings.h"
#include "../utils/dbg.h"
#include "AsyncTCP.h"
#include "../appInterface.h"
#include "../utils/helper.h"
#include "AsyncJson.h"
#include "ESPAsyncWebServer.h"


#undef F
#define F(sl) (sl)

//const uint8_t acList[] = {FLD_UAC, FLD_IAC, FLD_PAC, FLD_F, FLD_PF, FLD_T, FLD_YT, FLD_YD, FLD_PDC, FLD_EFF, FLD_Q, FLD_MP};
//const uint8_t acListHmt[] = {FLD_UAC_1N, FLD_IAC_1, FLD_PAC, FLD_F, FLD_PF, FLD_T, FLD_YT, FLD_YD, FLD_PDC, FLD_EFF, FLD_Q, FLD_MP};
//const uint8_t dcList[] = {FLD_UDC, FLD_IDC, FLD_PDC, FLD_YD, FLD_YT, FLD_IRR, FLD_MP};

class RestApi {
    public:
        RestApi() {
            mTimezoneOffset = 0;
            mHeapFree = 0;
            mHeapFreeBlk = 0;
            mHeapFrag = 0;
            nr = 0;
        }

        void setup(IApp *app, AsyncWebServer *srv, settings_t *config) {
            mApp     = app;
            mSrv     = srv;
            mConfig  = config;
            mSrv->on("/api", HTTP_GET,  std::bind(&RestApi::onApi,         this, std::placeholders::_1));
            mSrv->on("/api", HTTP_POST, std::bind(&RestApi::onApiPost,     this, std::placeholders::_1)).onBody(
                                        std::bind(&RestApi::onApiPostBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

            mSrv->on("/get_setup", HTTP_GET,  std::bind(&RestApi::onDwnldSetup, this, std::placeholders::_1));
        }

        uint32_t getTimezoneOffset(void) {
            return mTimezoneOffset;
        }

        void ctrlRequest(JsonObject obj) {
            /*char out[128];
            serializeJson(obj, out, 128);
            DPRINTLN(DBG_INFO, "RestApi: " + String(out));*/
            DynamicJsonDocument json(128);
            JsonObject dummy = json.as<JsonObject>();
            if(obj[F("path")] == "ctrl")
                setCtrl(obj, dummy);
            else if(obj[F("path")] == "setup")
                setSetup(obj, dummy);
        }

    private:
        void onApi(AsyncWebServerRequest *request) {
            mHeapFree = ESP.getFreeHeap();
            
            AsyncJsonResponse* response = new AsyncJsonResponse(false, 6000);
            JsonObject root = response->getRoot();

            String path = request->url().substring(5);
            if(path == "html/system")         getHtmlSystem(request, root);
            else if(path == "html/logout")    getHtmlLogout(request, root);
            else if(path == "html/reboot")    getHtmlReboot(request, root);
            else if(path == "html/save")      getHtmlSave(request, root);
            else if(path == "system")         getSysInfo(request, root);
            else if(path == "generic")        getGeneric(request, root);
            else if(path == "reboot")         getReboot(request, root);
            else if(path == "index")          getIndex(request, root);
            else if(path == "setup")          getSetup(request, root);
            else if(path == "setup/networks") getNetworks(root);
            else if(path == "live")           getLive(request,root);
            else {
                    getNotFound(root, F("http://") + request->host() + F("/api/"));
            }

            //DPRINTLN(DBG_INFO, "API mem usage: " + String(root.memoryUsage()));
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Headers", "content-type");
            response->setLength();
            request->send(response);
        }

        void onApiPost(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, "onApiPost");
        }

        void onApiPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DPRINTLN(DBG_VERBOSE, "onApiPostBody");
            DynamicJsonDocument json(200);
            AsyncJsonResponse* response = new AsyncJsonResponse(false, 200);
            JsonObject root = response->getRoot();

            DeserializationError err = deserializeJson(json, (const char *)data, len);
            JsonObject obj = json.as<JsonObject>();
            root[F("success")] = (err) ? false : true;
            if(!err) {
                String path = request->url().substring(5);
                if(path == "ctrl")
                    root[F("success")] = setCtrl(obj, root);
                else if(path == "setup")
                    root[F("success")] = setSetup(obj, root);
                else {
                    root[F("success")] = false;
                    root[F("error")]   = "Path not found: " + path;
                }
            }
            else {
                switch (err.code()) {
                    case DeserializationError::Ok: break;
                    case DeserializationError::InvalidInput: root[F("error")] = F("Invalid input");          break;
                    case DeserializationError::NoMemory:     root[F("error")] = F("Not enough memory");      break;
                    default:                                 root[F("error")] = F("Deserialization failed"); break;
                }
            }

            response->setLength();
            request->send(response);
        }

        void getNotFound(JsonObject obj, String url) {
            JsonObject ep = obj.createNestedObject("avail_endpoints");
            ep[F("generic")]          = url + F("generic");
            ep[F("index")]            = url + F("index");
            ep[F("setup")]            = url + F("setup");
            ep[F("system")]           = url + F("system");
            ep[F("live")]             = url + F("live");
        }


        void onDwnldSetup(AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response;

            File fp = LittleFS.open("/settings.json", "r");
            if(!fp) {
                DPRINTLN(DBG_ERROR, F("failed to load settings"));
                response = request->beginResponse(200, F("application/json; charset=utf-8"), "{}");
            }
            else {
                String tmp = fp.readString();
                int i = 0;
                // remove all passwords
                while (i != -1) {
                    i = tmp.indexOf("\"pwd\":", i);
                    if(-1 != i) {
                        i+=7;
                        tmp.remove(i, tmp.indexOf("\"", i)-i);
                    }
                }
                response = request->beginResponse(200, F("application/json; charset=utf-8"), tmp);
            }

            String filename = ah::getDateTimeStrFile(gTimezone.toLocal(mApp->getTimestamp()));
            filename += "_v" + String(mApp->getVersion());

            response->addHeader("Content-Type", "application/octet-stream");
            response->addHeader("Content-Description", "File Transfer");
            response->addHeader("Content-Disposition", "attachment; filename=" + filename + "_ahoy_setup.json");
            request->send(response);
            fp.close();
        }

        void getGeneric(AsyncWebServerRequest *request, JsonObject obj) {
            obj[F("web_refresh")] = 5;
            obj[F("wifi_rssi")]   = (WiFi.status() != WL_CONNECTED) ? 0 : WiFi.RSSI();
            obj[F("ts_uptime")]   = mApp->getUptime();
            obj[F("ts_now")]      = mApp->getTimestamp();
            obj[F("version")]     = String(mApp->getVersion());
            obj[F("build")]       = String(AUTO_GIT_HASH);
            obj[F("menu_prot")]   = mApp->getProtection(request);
            obj[F("menu_mask")]   = (uint16_t)(mConfig->sys.protectionMask );
            obj[F("menu_protEn")] = (bool) (strlen(mConfig->sys.adminPwd) > 0);

            obj[F("esp_type")]    = F("ESP32");
        }

        void getSysInfo(AsyncWebServerRequest *request, JsonObject obj) {
            obj[F("ssid")]         = mConfig->sys.stationSsid;
            obj[F("ap_pwd")]       = mConfig->sys.apPwd;
            obj[F("hidd")]         = mConfig->sys.isHidden;

            obj[F("device_name")]  = mConfig->sys.deviceName;
            obj[F("dark_mode")]    = (bool)mConfig->sys.darkMode;
            obj[F("sched_reboot")]    = (bool)mConfig->sys.schedReboot;

            obj[F("mac")]          = WiFi.macAddress();
            obj[F("hostname")]     = mConfig->sys.deviceName;
            obj[F("pwd_set")]      = (strlen(mConfig->sys.adminPwd) > 0);
            obj[F("prot_mask")]    = mConfig->sys.protectionMask;

            obj[F("sdk")]          = ESP.getSdkVersion();
            obj[F("cpu_freq")]     = ESP.getCpuFreqMHz();
            obj[F("heap_free")]    = mHeapFree;
            obj[F("sketch_total")] = ESP.getFreeSketchSpace();
            obj[F("sketch_used")]  = ESP.getSketchSize() / 1024; // in kb
            getGeneric(request, obj);

            obj[F("heap_total")]    = ESP.getHeapSize();
            obj[F("chip_revision")] = ESP.getChipRevision();
            obj[F("chip_model")]    = ESP.getChipModel();
            obj[F("chip_cores")]    = ESP.getChipCores();
          
            //uint8_t max;
            //mApp->getSchedulerInfo(&max);
            //obj[F("schMax")] = max;
        }

        void getHtmlSystem(AsyncWebServerRequest *request, JsonObject obj) {
            getSysInfo(request, obj.createNestedObject(F("system")));
            getGeneric(request, obj.createNestedObject(F("generic")));
            obj[F("html")] = F("<a href=\"/factory\" class=\"btn\">Factory Reset</a><br/><br/><a href=\"/reboot\" class=\"btn\">Reboot</a>");
        }

        void getHtmlLogout(AsyncWebServerRequest *request, JsonObject obj) {
            getGeneric(request, obj.createNestedObject(F("generic")));
            obj[F("refresh")] = 3;
            obj[F("refresh_url")] = "/";
            obj[F("html")] = F("succesfully logged out");
        }

        void getHtmlReboot(AsyncWebServerRequest *request, JsonObject obj) {
            getGeneric(request, obj.createNestedObject(F("generic")));
            obj[F("refresh")] = 20;
            obj[F("refresh_url")] = "/";
            obj[F("html")] = F("rebooting ...");
        }

        void getHtmlSave(AsyncWebServerRequest *request, JsonObject obj) {
            getGeneric(request, obj.createNestedObject(F("generic")));
            // obj["pending"] = (bool)mApp->getSavePending();
            // obj["success"] = (bool)mApp->getLastSaveSucceed();
            // obj["reboot"] = (bool)mApp->getShouldReboot();
        }

        void getReboot(AsyncWebServerRequest *request, JsonObject obj) {
            getGeneric(request, obj.createNestedObject(F("generic")));
            obj[F("refresh")] = 10;
            obj[F("refresh_url")] = "/";
            obj[F("html")] = F("reboot. Autoreload after 10 seconds");
        }

        void getIvAlarms(JsonObject obj, uint8_t id) {
            // Inverter<> *iv = mSys->getInverterByPos(id);
            // if(NULL == iv) {
            //     obj[F("error")] = F("inverter not found!");
            //     return;
            // }

            // record_t<> *rec = iv->getRecordStruct(RealTimeRunData_Debug);

            // obj[F("iv_id")]   = id;
            // obj[F("cnt")]     = iv->alarmCnt;
            // obj[F("last_id")] = iv->getChannelFieldValue(CH0, FLD_EVT, rec);

            // JsonArray alarm = obj.createNestedArray(F("alarm"));
            // for(uint8_t i = 0; i < 10; i++) {
            //     alarm[i][F("code")]  = iv->lastAlarm[i].code;
            //     alarm[i][F("str")]   = iv->getAlarmStr(iv->lastAlarm[i].code);
            //     alarm[i][F("start")] = iv->lastAlarm[i].start;
            //     alarm[i][F("end")]   = iv->lastAlarm[i].end;
            // }
        }

        void getMqtt(JsonObject obj) {
            // obj[F("broker")]     = String(mConfig->mqtt.broker);
            // obj[F("clientId")]   = String(mConfig->mqtt.clientId);
            // obj[F("port")]       = String(mConfig->mqtt.port);
            // obj[F("user")]       = String(mConfig->mqtt.user);
            // obj[F("pwd")]        = (strlen(mConfig->mqtt.pwd) > 0) ? F("{PWD}") : String("");
            // obj[F("topic")]      = String(mConfig->mqtt.topic);
            // obj[F("interval")]   = String(mConfig->mqtt.interval);
        }

        void getNtp(JsonObject obj) {
            obj[F("addr")] = String(mConfig->ntp.addr);
            obj[F("port")] = String(mConfig->ntp.port);
            obj[F("interval")] = String(mConfig->ntp.interval);
        }

        void getSun(JsonObject obj) {
            // obj[F("lat")] = mConfig->sun.lat ? String(mConfig->sun.lat, 5) : "";
            // obj[F("lon")] = mConfig->sun.lat ? String(mConfig->sun.lon, 5) : "";
            // obj[F("offs")] = mConfig->sun.offsetSec;
        }

        void getStaticIp(JsonObject obj) {
            char buf[16];
            ah::ip2Char(mConfig->sys.ip.ip, buf);      obj[F("ip")]      = String(buf);
            ah::ip2Char(mConfig->sys.ip.mask, buf);    obj[F("mask")]    = String(buf);
            ah::ip2Char(mConfig->sys.ip.dns1, buf);    obj[F("dns1")]    = String(buf);
            ah::ip2Char(mConfig->sys.ip.dns2, buf);    obj[F("dns2")]    = String(buf);
            ah::ip2Char(mConfig->sys.ip.gateway, buf); obj[F("gateway")] = String(buf);
        }

        void getIndex(AsyncWebServerRequest *request, JsonObject obj) {
            DPRINTLN(DBG_INFO, "RestApi: getIndex called");
            getGeneric(request, obj.createNestedObject(F("generic")));
            obj[F("ts_now")]       = mApp->getTimestamp();

            char out[300];
            serializeJson(obj, out, 300);
            DPRINTLN(DBG_INFO, "RestApi: getIndex " + String(out) );
           // obj[F("ts_sunrise")]   = mApp->getSunrise();
           // obj[F("ts_sunset")]    = mApp->getSunset();
           // obj[F("ts_offset")]    = mConfig->sun.offsetSec;


           // if((!mApp->getMqttIsConnected()) && (String(mConfig->mqtt.broker).length() > 0))
           //     warn.add(F("MQTT is not connected"));

           // JsonArray info = obj.createNestedArray(F("infos"));
           // if(mApp->getMqttIsConnected())
           //     info.add(F("MQTT is connected, ") + String(mApp->getMqttTxCnt()) + F(" packets sent, ") + String(mApp->getMqttRxCnt()) + F(" packets received"));
           // if(mConfig->mqtt.interval > 0)
           //     info.add(F("MQTT publishes in a fixed interval of ") + String(mConfig->mqtt.interval) + F(" seconds"));
       }

       void getSetup(AsyncWebServerRequest *request, JsonObject obj) {
            DPRINTLN(DBG_INFO, "RestApi: getSetup called");
            getGeneric(request, obj.createNestedObject(F("generic")));
            getSysInfo(request, obj.createNestedObject(F("system")));
            getMqtt(obj.createNestedObject(F("mqtt")));
            getNtp(obj.createNestedObject(F("ntp")));
            getSun(obj.createNestedObject(F("sun")));
            getStaticIp(obj.createNestedObject(F("static_ip")));
            char out[300];
            serializeJson(obj, out, 300);
            DPRINTLN(DBG_INFO, "RestApi: getSetup " + String(out));
       }

       void getNetworks(JsonObject obj) {
           mApp->getAvailNetworks(obj);
       }

       void getLive(AsyncWebServerRequest *request, JsonObject obj) {
           getGeneric(request, obj.createNestedObject(F("generic")));
           // obj[F("refresh")] = mConfig->nrf.sendInterval;

           // for (uint8_t fld = 0; fld < sizeof(acList); fld++) {
           //     obj[F("ch0_fld_units")][fld] = String(units[fieldUnits[acList[fld]]]);
           //     obj[F("ch0_fld_names")][fld] = String(fields[acList[fld]]);
           // }
           // for (uint8_t fld = 0; fld < sizeof(dcList); fld++) {
           //     obj[F("fld_units")][fld] = String(units[fieldUnits[dcList[fld]]]);
           //     obj[F("fld_names")][fld] = String(fields[dcList[fld]]);
           // }

           // Inverter<> *iv;
           // for(uint8_t i = 0; i < MAX_NUM_INVERTERS; i ++) {
           //     iv = mSys->getInverterByPos(i);
           //     bool parse = false;
           //     if(NULL != iv)
           //         parse = iv->config->enabled;
           //     obj[F("iv")][i] = parse;
           // }
       }


       bool setCtrl(JsonObject jsonIn, JsonObject jsonOut) {
           // Inverter<> *iv = mSys->getInverterByPos(jsonIn[F("id")]);
           // bool accepted = true;
           // if(NULL == iv) {
           //     jsonOut[F("error")] = F("inverter index invalid: ") + jsonIn[F("id")].as<String>();
           //     return false;
           // }

           // if(F("power") == jsonIn[F("cmd")])
           //     accepted = iv->setDevControlRequest((jsonIn[F("val")] == 1) ? TurnOn : TurnOff);
           // else if(F("restart") == jsonIn[F("cmd")])
           //     accepted = iv->setDevControlRequest(Restart);
           // else if(0 == strncmp("limit_", jsonIn[F("cmd")].as<const char*>(), 6)) {
           //     iv->powerLimit[0] = jsonIn["val"];
           //     if(F("limit_persistent_relative") == jsonIn[F("cmd")])
           //         iv->powerLimit[1] = RelativPersistent;
           //     else if(F("limit_persistent_absolute") == jsonIn[F("cmd")])
           //         iv->powerLimit[1] = AbsolutPersistent;
           //     else if(F("limit_nonpersistent_relative") == jsonIn[F("cmd")])
           //         iv->powerLimit[1] = RelativNonPersistent;
           //     else if(F("limit_nonpersistent_absolute") == jsonIn[F("cmd")])
           //         iv->powerLimit[1] = AbsolutNonPersistent;

           //     accepted = iv->setDevControlRequest(ActivePowerContr);
           // }
           // else if(F("dev") == jsonIn[F("cmd")]) {
           //     DPRINTLN(DBG_INFO, F("dev cmd"));
           //     iv->enqueCommand<InfoCommand>(jsonIn[F("val")].as<int>());
           // }
           // else {
           //     jsonOut[F("error")] = F("unknown cmd: '") + jsonIn["cmd"].as<String>() + "'";
           //     return false;
           // }

           // if(!accepted) {
           //     jsonOut[F("error")] = F("inverter does not accept dev control request at this moment");
           //     return false;
           // } else
           //     mApp->ivSendHighPrio(iv);

           return true;
       }

       bool setSetup(JsonObject jsonIn, JsonObject jsonOut) {
            char out[300];
            serializeJson(jsonIn, out, 300);
            DPRINTLN(DBG_INFO, "RestApi: setSetup " + String(out));
            if(F("scan_wifi") == jsonIn[F("cmd")])
                mApp->scanAvailNetworks();
            else 
                if(F("set_time") == jsonIn[F("cmd")])
                    mApp->setTimestamp(jsonIn[F("val")]);
                else if(F("sync_ntp") == jsonIn[F("cmd")])
                    mApp->setTimestamp(0); // 0: update ntp flag
                else if(F("serial_utc_offset") == jsonIn[F("cmd")])
                    mTimezoneOffset = jsonIn[F("val")];
                else if(F("discovery_cfg") == jsonIn[F("cmd")]) {
                    //mApp->setMqttDiscoveryFlag(); // for homeassistant
                } else {
                    jsonOut[F("error")] = F("unknown cmd");
                    return false;
                }

           return true;
       }

       IApp *mApp;
       AsyncWebServer *mSrv;
       settings_t *mConfig;

       uint32_t mTimezoneOffset;
       uint32_t mHeapFree, mHeapFreeBlk;
       uint8_t mHeapFrag;
       uint16_t nr;
};

#endif /*__WEB_API_H__*/
