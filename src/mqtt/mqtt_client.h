#ifndef __mqtt_client
#define __mqtt_client


#include "../utils/dbg.h"
#include "../config/config.h"
#include "../config/settings.h"
#include <espMqttClient.h>



class MqttEnClient {
    public:
        MqttEnClient() {

        }

        ~MqttEnClient() {}

        void setup(cfgMqtt_t *cfg_mqtt, const char *devName, const char *version, uint32_t *utcTs, uint32_t *uptime){
            DPRINTLN(DBG_INFO, cfg_mqtt->broker);
        }

        bool isConnected() {
            return true;
        }

        void loop() {
            
        }

    private :
        espMqttClient mqttClient;


};


#endif