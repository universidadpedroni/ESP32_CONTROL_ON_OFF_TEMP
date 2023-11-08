#ifndef MQTTCONSTANTS_H
#define MQTTCONSTANTS_H

#include "WiFi.h"

//#define MQTT_HOST IPAddress(192,168,1,106)  // Dirección IP del host, en caso de que sea local
#define MQTT_HOST "io.adafruit.com"   // Nombre de dominio de broker, en caso de que esté en la nube
#define MQTT_PORT 1883
#define IO_USERNAME  "Obi_Juan"
#define IO_KEY       "aio_wWCu92dXQp4vqnmzkVcC9hImDOOM"


#define MQTT_PUB_TREF "Obi_Juan/feeds/esp-slash-control-slash-tempref"
#define MQTT_PUB_TEMP "Obi_Juan/feeds/esp-slash-control-slash-temp"
#define MQTT_PUB_ERR "Obi_Juan/feeds/esp-slash-control-slash-error"
#define MQTT_PUB_ACC "Obi_Juan/feeds/esp-slash-control-slash-acciondecontrol"

#define MQTT_DASHBOARD "https://io.adafruit.com/Obi_Juan/dashboards/esp32-control-on-off"
bool mqttDisponible = true;

#endif