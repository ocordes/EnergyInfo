//-----------------------------------------------------------------------------
// 2022 Ahoy, https://www.mikrocontroller.net/topic/525778
// Creative Commons - http://creativecommons.org/licenses/by-nc-sa/4.0/deed
//-----------------------------------------------------------------------------

#ifndef __WEB_H__
#define __WEB_H__

#include "../config/config.h"
#include "../config/settings.h"

#include "../utils/dbg.h"

#include "AsyncTCP.h"
#include "Update.h"

#include "../appInterface.h"
#include "../utils/helper.h"
#include "ESPAsyncWebServer.h"


#include "html/h/api_js.h"
#include "html/h/colorBright_css.h"
#include "html/h/colorDark_css.h"
#include "html/h/favicon_ico.h"
#include "html/h/index_html.h"
#include "html/h/login_html.h"
#include "html/h/serial_html.h"
#include "html/h/setup_html.h"
#include "html/h/style_css.h"
#include "html/h/system_html.h"
#include "html/h/save_html.h"
#include "html/h/update_html.h"
#include "html/h/visualization_html.h"
#include "html/h/about_html.h"

#define WEB_SERIAL_BUF_SIZE 2048

const char* const pinArgNames[] = {"pinCs", "pinCe", "pinIrq", "pinSclk", "pinMosi", "pinMiso", "pinLed0", "pinLed1", "pinLedHighActive", "pinCsb", "pinFcsb", "pinGpio3"};

template <class HMSYSTEM>
class Web {
   public:
        Web(void) : mWeb(80), mEvts("/events") {
            mProtected     = true;
            mLogoutTimeout = 0;

            memset(mSerialBuf, 0, WEB_SERIAL_BUF_SIZE);
            mSerialBufFill     = 0;
            mSerialAddTime     = true;
            mSerialClientConnnected = false;
        }

        void setup(IApp *app, settings_t *config) {
            mApp     = app;
            mConfig  = config;

            DPRINTLN(DBG_VERBOSE, F("app::setup-on"));
            mWeb.on("/",               HTTP_GET,  std::bind(&Web::onIndex,        this, std::placeholders::_1));
            mWeb.on("/login",          HTTP_ANY,  std::bind(&Web::onLogin,        this, std::placeholders::_1));
            mWeb.on("/logout",         HTTP_GET,  std::bind(&Web::onLogout,       this, std::placeholders::_1));
            mWeb.on("/colors.css",     HTTP_GET,  std::bind(&Web::onColor,        this, std::placeholders::_1));
            mWeb.on("/style.css",      HTTP_GET,  std::bind(&Web::onCss,          this, std::placeholders::_1));
            mWeb.on("/api.js",         HTTP_GET,  std::bind(&Web::onApiJs,        this, std::placeholders::_1));
            mWeb.on("/favicon.ico",    HTTP_GET,  std::bind(&Web::onFavicon,      this, std::placeholders::_1));
            mWeb.onNotFound (                     std::bind(&Web::showNotFound,   this, std::placeholders::_1));
            mWeb.on("/reboot",         HTTP_ANY,  std::bind(&Web::onReboot,       this, std::placeholders::_1));
            mWeb.on("/system",         HTTP_ANY,  std::bind(&Web::onSystem,       this, std::placeholders::_1));
            mWeb.on("/erase",          HTTP_ANY,  std::bind(&Web::showErase,      this, std::placeholders::_1));
            mWeb.on("/factory",        HTTP_ANY,  std::bind(&Web::showFactoryRst, this, std::placeholders::_1));

            mWeb.on("/setup",          HTTP_GET,  std::bind(&Web::onSetup,        this, std::placeholders::_1));
            mWeb.on("/save",           HTTP_POST, std::bind(&Web::showSave,       this, std::placeholders::_1));

            mWeb.on("/live",           HTTP_ANY,  std::bind(&Web::onLive,         this, std::placeholders::_1));
            //mWeb.on("/api1",           HTTP_POST, std::bind(&Web::showWebApi,     this, std::placeholders::_1));

        #ifdef ENABLE_PROMETHEUS_EP
            mWeb.on("/metrics",        HTTP_ANY,  std::bind(&Web::showMetrics,    this, std::placeholders::_1));
        #endif

            mWeb.on("/update",         HTTP_GET,  std::bind(&Web::onUpdate,       this, std::placeholders::_1));
            mWeb.on("/update",         HTTP_POST, std::bind(&Web::showUpdate,     this, std::placeholders::_1),
                                                  std::bind(&Web::showUpdate2,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
            mWeb.on("/upload",         HTTP_POST, std::bind(&Web::onUpload,       this, std::placeholders::_1),
                                                  std::bind(&Web::onUpload2,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
            mWeb.on("/serial",         HTTP_GET,  std::bind(&Web::onSerial,       this, std::placeholders::_1));
            mWeb.on("/about",          HTTP_GET,  std::bind(&Web::onAbout,        this, std::placeholders::_1));
            mWeb.on("/debug",          HTTP_GET,  std::bind(&Web::onDebug,        this, std::placeholders::_1));


            mEvts.onConnect(std::bind(&Web::onConnect, this, std::placeholders::_1));
            mWeb.addHandler(&mEvts);

            mWeb.begin();

            registerDebugCb(std::bind(&Web::serialCb, this, std::placeholders::_1)); // dbg.h

            mUploadFail = false;
        }

        void tickSecond() {
            if (0 != mLogoutTimeout) {
                mLogoutTimeout -= 1;
                if (0 == mLogoutTimeout) {
                    if (strlen(mConfig->sys.adminPwd) > 0)
                        mProtected = true;
                }

                DPRINTLN(DBG_DEBUG, "auto logout in " + String(mLogoutTimeout));
            }

            if (mSerialClientConnnected) {
                if (mSerialBufFill > 0) {
                    mEvts.send(mSerialBuf, "serial", millis());
                    memset(mSerialBuf, 0, WEB_SERIAL_BUF_SIZE);
                    mSerialBufFill = 0;
                }
            }
        }

        AsyncWebServer *getWebSrvPtr(void) {
            return &mWeb;
        }

        void setProtection(bool protect) {
            mProtected = protect;
        }

        bool isProtected(AsyncWebServerRequest *request) {
            bool prot;
            prot = mProtected;
            if(!prot) {
                if(strlen(mConfig->sys.adminPwd) > 0) {
                    uint8_t ip[4];
                    ah::ip2Arr(ip, request->client()->remoteIP().toString().c_str());
                    for(uint8_t i = 0; i < 4; i++) {
                        if(mLoginIp[i] != ip[i])
                            prot = true;
                    }
                }
            }

            return prot;
        }

        void showUpdate2(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // OC: mApp->setOnUpdate();
            
            if (!index) {
                Serial.printf("Update Start: %s\n", filename.c_str());
                if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError()) {
                if (Update.write(data, len) != len)
                    Update.printError(Serial);
            }
            if (final) {
                if (Update.end(true))
                    Serial.printf("Update Success: %uB\n", index + len);
                else
                    Update.printError(Serial);
            }
        }

        void onUpload2(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                mUploadFail = false;
                mUploadFp = LittleFS.open("/tmp.json", "w");
                if (!mUploadFp) {
                    DPRINTLN(DBG_ERROR, F("can't open file!"));
                    mUploadFail = true;
                    mUploadFp.close();
                    return;
                }
            }
            mUploadFp.write(data, len);
            if (final) {
                mUploadFp.close();
                char pwd[PWD_LEN];
                strncpy(pwd, mConfig->sys.stationPwd, PWD_LEN); // backup WiFi PWD
                //OC if (!mApp->readSettings("/tmp.json")) {
                //OC     mUploadFail = true;
                //OC     DPRINTLN(DBG_ERROR, F("upload JSON error!"));
                //OC } else {
                //OC     LittleFS.remove("/tmp.json");
                //OC     strncpy(mConfig->sys.stationPwd, pwd, PWD_LEN); // restore WiFi PWD
                //OC     mApp->saveSettings(true);
                //OC }
                if (!mUploadFail)
                    DPRINTLN(DBG_INFO, F("upload finished!"));
            }
        }

        void serialCb(String msg) {
            if (!mSerialClientConnnected)
                return;

            msg.replace("\r\n", "<rn>");
            if (mSerialAddTime) {
                if ((9 + mSerialBufFill) < WEB_SERIAL_BUF_SIZE) {
                    //OC: if (mApp->getTimestamp() > 0) {
                    //OC:    strncpy(&mSerialBuf[mSerialBufFill], mApp->getTimeStr(mApp->getTimezoneOffset()).c_str(), 9);
                    //OC:    mSerialBufFill += 9;
                    //OC:}
                } else {
                    mSerialBufFill = 0;
                    mEvts.send("webSerial, buffer overflow!<rn>", "serial", millis());
                    return;
                }
                mSerialAddTime = false;
            }

            if (msg.endsWith("<rn>"))
                mSerialAddTime = true;

            uint16_t length = msg.length();
            if ((length + mSerialBufFill) < WEB_SERIAL_BUF_SIZE) {
                strncpy(&mSerialBuf[mSerialBufFill], msg.c_str(), length);
                mSerialBufFill += length;
            } else {
                mSerialBufFill = 0;
                mEvts.send("webSerial, buffer overflow!<rn>", "serial", millis());
            }
        }

    private:
        inline void checkRedirect(AsyncWebServerRequest *request) {
            if ((mConfig->sys.protectionMask & PROT_MASK_INDEX) != PROT_MASK_INDEX)
                request->redirect(F("/index"));
            else if ((mConfig->sys.protectionMask & PROT_MASK_LIVE) != PROT_MASK_LIVE)
                request->redirect(F("/live"));
            else if ((mConfig->sys.protectionMask & PROT_MASK_SERIAL) != PROT_MASK_SERIAL)
                request->redirect(F("/serial"));
            else if ((mConfig->sys.protectionMask & PROT_MASK_SYSTEM) != PROT_MASK_SYSTEM)
                request->redirect(F("/system"));
            else
                request->redirect(F("/login"));
        }

        void checkProtection(AsyncWebServerRequest *request) {
            if(isProtected(request)) {
                checkRedirect(request);
                return;
            }
        }

        void getPage(AsyncWebServerRequest *request, uint8_t mask, const uint8_t *zippedHtml, uint32_t len) {
            if (CHECK_MASK(mConfig->sys.protectionMask, mask))
                checkProtection(request);

            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), zippedHtml, len);
            response->addHeader(F("Content-Encoding"), "gzip");
            response->addHeader(F("content-type"), "text/html; charset=UTF-8");
            if(request->hasParam("v"))
                response->addHeader(F("Cache-Control"), F("max-age=604800"));
            request->send(response);
        }

        void onUpdate(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_UPDATE, update_html, update_html_len);
        }

        void showUpdate(AsyncWebServerRequest *request) {
            bool reboot = (!Update.hasError());

            String html = F("<!doctype html><html><head><title>Update</title><meta http-equiv=\"refresh\" content=\"20; URL=/\"></head><body>Update: ");
            if (reboot)
                html += "success";
            else
                html += "failed";
            html += F("<br/><br/>rebooting ... auto reload after 20s</body></html>");

            AsyncWebServerResponse *response = request->beginResponse(200, F("text/html; charset=UTF-8"), html);
            response->addHeader("Connection", "close");
            request->send(response);
            //OC: mApp->setRebootFlag();
        }

        void onUpload(AsyncWebServerRequest *request) {
            bool reboot = !mUploadFail;

            String html = F("<!doctype html><html><head><title>Upload</title><meta http-equiv=\"refresh\" content=\"20; URL=/\"></head><body>Upload: ");
            if (reboot)
                html += "success";
            else
                html += "failed";
            html += F("<br/><br/>rebooting ... auto reload after 20s</body></html>");

            AsyncWebServerResponse *response = request->beginResponse(200, F("text/html; charset=UTF-8"), html);
            response->addHeader("Connection", "close");
            request->send(response);
            //OC: mApp->setRebootFlag();
        }

        void onConnect(AsyncEventSourceClient *client) {
            DPRINTLN(DBG_VERBOSE, "onConnect");

            mSerialClientConnnected = true;

            if (client->lastId())
                DPRINTLN(DBG_VERBOSE, "Client reconnected! Last message ID that it got is: " + String(client->lastId()));

            client->send("hello!", NULL, millis(), 1000);
        }

        void onIndex(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_INDEX, index_html, index_html_len);
        }

        void onLogin(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("onLogin"));

            if (request->args() > 0) {
                if (String(request->arg("pwd")) == String(mConfig->sys.adminPwd)) {
                    mProtected = false;
                    ah::ip2Arr(mLoginIp, request->client()->remoteIP().toString().c_str());
                    request->redirect("/");
                }
            }

            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), login_html, login_html_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            request->send(response);
        }

        void onLogout(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("onLogout"));

            checkProtection(request);

            mProtected = true;

            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), system_html, system_html_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            request->send(response);
        }

        void onColor(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("onColor"));
            AsyncWebServerResponse *response;
            if (mConfig->sys.darkMode)
                response = request->beginResponse_P(200, F("text/css"), colorDark_css, colorDark_css_len);
            else
                response = request->beginResponse_P(200, F("text/css"), colorBright_css, colorBright_css_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            if(request->hasParam("v")) {
                response->addHeader(F("Cache-Control"), F("max-age=604800"));
            }
            request->send(response);
        }

        void onCss(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("onCss"));
            mLogoutTimeout = LOGOUT_TIMEOUT;
            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/css"), style_css, style_css_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            if(request->hasParam("v")) {
                response->addHeader(F("Cache-Control"), F("max-age=604800"));
            }
            request->send(response);
        }

        void onApiJs(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("onApiJs"));

            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/javascript"), api_js, api_js_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            if(request->hasParam("v"))
                response->addHeader(F("Cache-Control"), F("max-age=604800"));
            request->send(response);
        }

        void onFavicon(AsyncWebServerRequest *request) {
            static const char favicon_type[] PROGMEM = "image/x-icon";
            AsyncWebServerResponse *response = request->beginResponse_P(200, favicon_type, favicon_ico, favicon_ico_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            request->send(response);
        }

        void showNotFound(AsyncWebServerRequest *request) {
            checkProtection(request);
            request->redirect("/setup");
        }

        void onReboot(AsyncWebServerRequest *request) {
            //OC: mApp->setRebootFlag();
            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), system_html, system_html_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            request->send(response);
        }

        void showErase(AsyncWebServerRequest *request) {
            checkProtection(request);

            DPRINTLN(DBG_VERBOSE, F("showErase"));
            //OC: mApp->eraseSettings(false);
            onReboot(request);
        }

        void showFactoryRst(AsyncWebServerRequest *request) {
            checkProtection(request);

            DPRINTLN(DBG_VERBOSE, F("showFactoryRst"));
            String content = "";
            int refresh = 3;
            if (request->args() > 0) {
                if (request->arg("reset").toInt() == 1) {
                    refresh = 10;
                    //OC: if (mApp->eraseSettings(true))
                    //OC:    content = F("factory reset: success\n\nrebooting ... ");
                    //OC:else
                    //OC:    content = F("factory reset: failed\n\nrebooting ... ");
                } else {
                    content = F("factory reset: aborted");
                    refresh = 3;
                }
            } else {
                content = F("<h1>Factory Reset</h1>"
                    "<p><a href=\"/factory?reset=1\">RESET</a><br/><br/><a href=\"/factory?reset=0\">CANCEL</a><br/></p>");
                refresh = 120;
            }
            request->send(200, F("text/html; charset=UTF-8"), F("<!doctype html><html><head><title>Factory Reset</title><meta http-equiv=\"refresh\" content=\"") + String(refresh) + F("; URL=/\"></head><body>") + content + F("</body></html>"));
            if (refresh == 10)
                onReboot(request);
        }

        void onSetup(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_SETUP, setup_html, setup_html_len);
        }

        void showSave(AsyncWebServerRequest *request) {
            DPRINTLN(DBG_VERBOSE, F("showSave"));

            checkProtection(request);

            if (request->args() == 0)
                return;

            char buf[20] = {0};

            // general
            #if !defined(ETHERNET)
            if (request->arg("ssid") != "")
                request->arg("ssid").toCharArray(mConfig->sys.stationSsid, SSID_LEN);
            if (request->arg("pwd") != "{PWD}")
                request->arg("pwd").toCharArray(mConfig->sys.stationPwd, PWD_LEN);
            if (request->arg("ap_pwd") != "")
                request->arg("ap_pwd").toCharArray(mConfig->sys.apPwd, PWD_LEN);
            mConfig->sys.isHidden = (request->arg("hidd") == "on");
            #endif /* !defined(ETHERNET) */
            if (request->arg("device") != "")
                request->arg("device").toCharArray(mConfig->sys.deviceName, DEVNAME_LEN);
            mConfig->sys.darkMode = (request->arg("darkMode") == "on");
            mConfig->sys.schedReboot = (request->arg("schedReboot") == "on");

            // protection
            if (request->arg("adminpwd") != "{PWD}") {
                request->arg("adminpwd").toCharArray(mConfig->sys.adminPwd, PWD_LEN);
                mProtected = (strlen(mConfig->sys.adminPwd) > 0);
            }
            mConfig->sys.protectionMask = 0x0000;
            for (uint8_t i = 0; i < 6; i++) {
                if (request->arg("protMask" + String(i)) == "on")
                    mConfig->sys.protectionMask |= (1 << i);
            }

            // static ip
            request->arg("ipAddr").toCharArray(buf, 20);
            ah::ip2Arr(mConfig->sys.ip.ip, buf);
            request->arg("ipMask").toCharArray(buf, 20);
            ah::ip2Arr(mConfig->sys.ip.mask, buf);
            request->arg("ipDns1").toCharArray(buf, 20);
            ah::ip2Arr(mConfig->sys.ip.dns1, buf);
            request->arg("ipDns2").toCharArray(buf, 20);
            ah::ip2Arr(mConfig->sys.ip.dns2, buf);
            request->arg("ipGateway").toCharArray(buf, 20);
            ah::ip2Arr(mConfig->sys.ip.gateway, buf);

            

            // ntp
            if (request->arg("ntpAddr") != "") {
                request->arg("ntpAddr").toCharArray(mConfig->ntp.addr, NTP_ADDR_LEN);
                mConfig->ntp.port = request->arg("ntpPort").toInt() & 0xffff;
                mConfig->ntp.interval = request->arg("ntpIntvl").toInt() & 0xffff;
            }

            

            // // mqtt
            // if (request->arg("mqttAddr") != "") {
            //     String addr = request->arg("mqttAddr");
            //     addr.trim();
            //     addr.toCharArray(mConfig->mqtt.broker, MQTT_ADDR_LEN);
            // } else
            //     mConfig->mqtt.broker[0] = '\0';
            // request->arg("mqttClientId").toCharArray(mConfig->mqtt.clientId, MQTT_CLIENTID_LEN);
            // request->arg("mqttUser").toCharArray(mConfig->mqtt.user, MQTT_USER_LEN);
            // if (request->arg("mqttPwd") != "{PWD}")
            //     request->arg("mqttPwd").toCharArray(mConfig->mqtt.pwd, MQTT_PWD_LEN);
            // request->arg("mqttTopic").toCharArray(mConfig->mqtt.topic, MQTT_TOPIC_LEN);
            // mConfig->mqtt.port = request->arg("mqttPort").toInt();
            // mConfig->mqtt.interval = request->arg("mqttInterval").toInt();

            

            //OC: mApp->saveSettings((request->arg("reboot") == "on"));

            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), save_html, save_html_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            request->send(response);
        }

        void onLive(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_LIVE, visualization_html, visualization_html_len);
        }

        void onAbout(AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html; charset=UTF-8"), about_html, about_html_len);
            response->addHeader(F("Content-Encoding"), "gzip");
            response->addHeader(F("content-type"), "text/html; charset=UTF-8");
            if(request->hasParam("v")) {
                response->addHeader(F("Cache-Control"), F("max-age=604800"));
            }

            request->send(response);
        }

        void onDebug(AsyncWebServerRequest *request) {
            //OC: mApp->getSchedulerNames();
            AsyncWebServerResponse *response = request->beginResponse(200, F("text/html; charset=UTF-8"), "ok");
            request->send(response);
        }

        void onSerial(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_SERIAL, serial_html, serial_html_len);
        }

        void onSystem(AsyncWebServerRequest *request) {
            getPage(request, PROT_MASK_SYSTEM, system_html, system_html_len);
        }


        AsyncWebServer mWeb;
        AsyncEventSource mEvts;
        bool mProtected;
        uint32_t mLogoutTimeout;
        uint8_t mLoginIp[4];
        IApp *mApp;

        settings_t *mConfig;

        bool mSerialAddTime;
        char mSerialBuf[WEB_SERIAL_BUF_SIZE];
        uint16_t mSerialBufFill;
        bool mSerialClientConnnected;

        File mUploadFp;
        bool mUploadFail;
};

#endif /*__WEB_H__*/
