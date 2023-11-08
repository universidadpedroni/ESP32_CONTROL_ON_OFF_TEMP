#include <Arduino.h>
#include <WiFiManager.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <ESP32Encoder.h>
//#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "blink.h"
#include "config.h"
#include "controlConstants.h"
#include "MQTTConstants.h"



#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "ArduinoHttpClient.h"
#include "WiFiClientSecure.h"

//const char* ssid = "iPhone de Juan";
//const char* password = "HelpUsObiJuan";
//const char* ssid = "TheShield";
//const char* password = "JamesBond007";
//#define CONNECT_TIME 10000
#define AP_NAME "ESP32 AP"
#define AP_PASS "password"

AsyncWebServer server(80);
blink parpadeo(LED_BUILTIN);
DHT dht(DHT_PIN, DHT22);
ESP32Encoder myEnc;
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
controlSignals signals = {TREF_DEFAULT, 0, 0, false, EMAX_DEFAULT};



WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_HOST, MQTT_PORT, IO_USERNAME, IO_KEY);

Adafruit_MQTT_Publish mqttTemp = Adafruit_MQTT_Publish(&mqtt, MQTT_PUB_TEMP);
Adafruit_MQTT_Publish mqttTref = Adafruit_MQTT_Publish(&mqtt, MQTT_PUB_TREF);
Adafruit_MQTT_Publish mqttError = Adafruit_MQTT_Publish(&mqtt, MQTT_PUB_ERR);
Adafruit_MQTT_Publish mqttAccion = Adafruit_MQTT_Publish(&mqtt, MQTT_PUB_ACC);

// ***** FUNCIONES DE INICIALIZACION ***** //

void displayInit(){
  display.begin(I2C_ADDRESS, true); // Address 0x3C default
  //display.begin()
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.println(F("Hola Mundo!"));
  display.println(F("Datos de Compilacion:"));
  display.printf("Fecha %s\n",__DATE__);
  display.printf("Hora: %s\n",__TIME__);
  display.println(F("Conectando a Wifi..."));
  display.display();
}

void dhtInit(){
  dht.begin();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  Serial.printf("Temperatura: %d°C\tHumedad: %d%\n",
                (int)temp,
                (int)hum);
}

void myEncInit(){
  myEnc.attachSingleEdge(PIN_ENC_A, PIN_ENC_B);
  myEnc.setCount(signals.tRef);
}

void spiffsInit(){
  if(!SPIFFS.begin(true)){
    Serial.println("Error al inicializar SPIFFs. El sistema se detendra");
    while(1){
      delay(100);
    }
  }
  else{
    Serial.println("SPIFFS inicializado con exito");
    Serial.println("Listado de archivos disponibles:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
      Serial.printf("Archivo: %s\n", file.name());
      file.close();
      file = root.openNextFile();
    }
    root.close();
    
  }

}

void wifiInit(){
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool resultado;
  resultado = wm.autoConnect(AP_NAME, AP_PASS);
  if(!resultado){
    Serial.println("No se pudo conectar a la red. Llamen a los Avengers");
  }
  else{
    Serial.println("Conectadazo");
  }

  /*unsigned long tiempoDeInicio = millis();
  Serial.printf("Intentando conectar a la red Wifi %s...", ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED && (tiempoDeInicio + CONNECT_TIME > millis())){
    Serial.print(".");
    delay(1000);
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("\nIP del servidor:");
    Serial.println(WiFi.localIP());
  }
  else{
    Serial.println("No se pudo conectar al Wifi. Llamen a los Avengers");
  }
  */
}

void displayWiFi(){
  display.clearDisplay();
  display.setCursor(0,0);
  if(WiFi.status() == WL_CONNECTED){
    display.println(F("Conectado al Wifi"));
    display.println(F("IP:"));
    display.println(WiFi.localIP());

  }
  else{
    display.println(F("No se pudo conectar a WiFi"));
  }
  display.display();

}

void serverInit(){
  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("getJson")) {
      // Obtener las últimas lecturas de temperatura y otros datos
      //float temperatureRef = myEnc.getCount(); // Reemplaza esta función por la que obtiene la temperatura de referencia
      //float temperatureReal = dht.readTemperature(); // Reemplaza esta función por la que obtiene la temperatura real
      // Calcular el error (puedes ajustar esto según tu lógica)
      //float error = temperatureRef - temperatureReal;

      // Crear un objeto JSON con los datos
      String json = "{ \"temperatureRef\": " + String(signals.tRef, 2) + ", \"temperatureReal\": " + String(signals.temp, 2) + ", \"error\": " + String(signals.error, 2) + ", \"accion\": " + String(signals.u == true? 1:0, 2) + " }";

      // Enviar el objeto JSON como respuesta
      request->send(200, "application/json", json);
    } else {
      request->send(SPIFFS, "/index.html", "text/html");
    }
  });
  server.begin();
}

// ***** FUNCIONES DE OPERACION ***** //
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
          mqttDisponible   = false;
         // basically die and wait for WDT to reset me
         //while (1);
       }
  }
  mqttDisponible = true;
  Serial.println("MQTT Connected!");
}

void mqttUpdate(unsigned long intervalo){
  static unsigned long previousMillis = 0;        // will store last time LED was updated
	//const long interval = 1000;           // interval at which to blink (milliseconds)
	unsigned long currentMillis = millis();
	
	
	if(currentMillis - previousMillis > intervalo) 
	{
		previousMillis = currentMillis;
    MQTT_connect();
    mqttTemp.publish(signals.temp);//== true? Serial.println("MQTT temp Enviado"): Serial.println("MQTT temp no Enviado");
    mqttTref.publish(signals.tRef);// == true? Serial.println("MQTT tref Enviado"): Serial.println("MQTT tref no Enviado");
    mqttError.publish(signals.error);
    mqttAccion.publish(signals.u);
    //Serial.println("MQTT enviados");
  }
}

void displayUpdate(){
  static controlSignals signalsOld = {TREF_DEFAULT, 0, 0, false, EMAX_DEFAULT}; 
  if((signals.temp != signalsOld.temp) ||
      (signals.tRef != signalsOld.tRef)) {  // Basta con revisar si alguna de estas dos señales cambió, ya que ellas arrastran los cambios de error y acción de control
        signalsOld.temp = signals.temp;
        signalsOld.tRef = signals. tRef;
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(1);
        display.printf("Tref: %.1fC\nTemp:%.1fC\nErr:%.1f\nAccion:%s\n\nIP:%s%s\nServ MQTT:%s\n",
                  signals.tRef,
                  signals.temp,
                  signals.error,
                  signals.u == true? "ON" : "OFF",
                  WiFi.localIP().toString(),"/home",
                  mqttDisponible == true? "OK": "No disp");

        display.display();
      }
  
  /*
  static unsigned long previousMillis = 0;        // will store last time LED was updated
	//const long interval = 1000;           // interval at which to blink (milliseconds)
	unsigned long currentMillis = millis();
	static bool estadoPin=false;
	
	if(currentMillis - previousMillis > intervalo) 
	{
		previousMillis = currentMillis;
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.printf("Tref: %.1fC\nTemp:%.1fC\nErr:%.1f\nAccion:%s\n\nIP:%s%s\n",
                  signals.tRef,
                  signals.temp,
                  signals.error,
                  signals.u == true? "ON" : "OFF",
                  WiFi.localIP().toString(),"/home");

    display.display();
    
    	
	}*/

}

void signalsUpdate(unsigned long intervalos){
  signals.tRef = constrain(myEnc.getCount(),-40.0, 40.0);
  signals.temp = dht.readTemperature();
  signals.error = signals.tRef - signals.temp;
  //signals.error > 0 ? signals.u = false: signals.u = true; // Control sin histéresis
  if(abs(signals.error) > signals.emax) signals.error >= 0? signals.u = false: signals.u = true; //Control con histeresis
  digitalWrite(PIN_LED_G, signals.u);
  digitalWrite(PIN_RELAY, signals.u);
  
}

void setup() {
  delay(1000);
  Serial.begin(BAUDRATE);
  Serial.println(F("Hola Mundo!"));
  Serial.printf("Fecha y hora de compilación: %s, %s\n", __DATE__, __TIME__);
  Serial.println(F("Iniciando Entradas Digitales"));
  pinMode(PIN_ENC_PUSH,INPUT_PULLUP);
  pinMode(PIN_LED_G, OUTPUT);
  digitalWrite(PIN_LED_G, HIGH);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  Serial.println(F("Iniciando Display"));
  displayInit();
  Serial.println(F("Iniciando DHT22"));
  dhtInit();
  Serial.println(F("Configurando ADC..."));
  analogReadResolution(12);
  Serial.println(F("Inicializando Encoder"));
  myEncInit();
  Serial.println(F("Inicializando SPIFFS"));
  spiffsInit();
  Serial.println(F("Inicializando WiFi"));
  wifiInit();
  displayWiFi();
  serverInit();
  delay(2000);
  Serial.printf("MQTT Dashboard: %s\n", MQTT_DASHBOARD);
}

void loop() {
  signalsUpdate(10);
  //displayUpdate(1000);
  displayUpdate();
  if(mqttDisponible) mqttUpdate(10000);
  parpadeo.update(1000);
}

