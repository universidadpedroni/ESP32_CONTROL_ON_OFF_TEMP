#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <ESP32Encoder.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "blink.h"
#include "config.h"

const char* ssid = "iPhone de Juan";
const char* password = "HelpUsObiJuan";
#define CONNECT_TIME 10000

AsyncWebServer server(80);
blink parpadeo(LED_BUILTIN);
DHT dht(DHT_PIN, DHT22);
ESP32Encoder myEnc;
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
controlSignals signals = {20, 0, 0, false};

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
  unsigned long tiempoDeInicio = millis();
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
      String json = "{ \"temperatureRef\": " + String(signals.tRef, 2) + ", \"temperatureReal\": " + String(signals.temp, 2) + ", \"error\": " + String(signals.error, 2) + " }";

      // Enviar el objeto JSON como respuesta
      request->send(200, "application/json", json);
    } else {
      request->send(SPIFFS, "/index.html", "text/html");
    }
  });
  server.begin();
}

// ***** FUNCIONES DE OPERACION ***** //



void displayUpdate(unsigned long intervalo){
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
    display.printf("Tref: %.1fC\nTemp:%.1fC\nErr:%.1f\nAccion:%s\nIP:%s%s\n",
                  signals.tRef,
                  signals.temp,
                  signals.error,
                  signals.u == true? "ON" : "OFF",
                  WiFi.localIP().toString(),"/home");

    display.display();
    
    	
	}

}

void signalsUpdate(unsigned long intervalos){
  signals.tRef = constrain(myEnc.getCount(),-40.0, 40.0);
  signals.temp = dht.readTemperature();
  signals.error = signals.tRef - signals.temp;
  signals.error > 0 ? signals.u = false: signals.u = true;
  digitalWrite(PIN_LED_G, signals.u);
  
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
}

void loop() {
  signalsUpdate(100);
  displayUpdate(1000);
  parpadeo.update(1000);
}

