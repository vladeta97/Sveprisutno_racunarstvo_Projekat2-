
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <PubSubClient.h>

#include "DHT.h"

ESP8266WiFiMulti wifiMulti;

#define DHTTYPE DHT22 
#define ANALOG_INPUT A0

const char* ssid = "*******";
const char* password = "********";

const char* mqtt_server = "*********";

WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
//const int lamp = 4;

// Relay1
const int relay1=13;

//Relay2
const int relay2=12;

//Mux select
const int muxA= 2;

const int AirValue = 790;
const int WaterValue = 390; 
int soilMoistureValue = 0;
int soilmoisturepercent=0;

// Initialize DHT sensor
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;


void setup_wifi() {
  delay(10);

  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if(topic=="room/relay1"){
    Serial.print("Realy1 changed to ");
    if(messageTemp=="on"){
      digitalWrite(relay1, HIGH);
      Serial.print("On");
      }
      else if(messageTemp=="off"){
        digitalWrite(relay1, LOW);
        Serial.print("Off");
        }
    }
    if(topic=="room/relay2"){
    Serial.print("Realy2 changed to ");
    if(messageTemp=="on"){
      digitalWrite(relay2, HIGH);
      Serial.print("On");
      }
      else if(messageTemp=="off"){
        digitalWrite(relay2, LOW);
        Serial.print("Off");
        }
    }
	
  Serial.println();
}


void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
   
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
   
      client.subscribe("room/lamp");
      client.subscribe("room/relay1");
       client.subscribe("room/relay2");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      delay(5000);
    }

  }
}

void setup() {

  pinMode(relay1,OUTPUT);
  pinMode(relay2,OUTPUT);
  pinMode(muxA,OUTPUT);
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void changeMux(int a){
  digitalWrite(muxA, a);
  }

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
   
    float h = dht.readHumidity();
   
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    int water;
    int sendWater;
    int moisture;
    
    changeMux(LOW);
    water=analogRead(ANALOG_INPUT);
    sendWater=map(water,0,300,0,100);
    static char waterLevel[7];
    dtostrf(sendWater, 6, 2, waterLevel);
    Serial.println("Water Level: ");
    Serial.println(waterLevel);
    
    changeMux(HIGH);
    moisture=analogRead(ANALOG_INPUT);
    soilmoisturepercent = map(moisture, AirValue, WaterValue, 0, 100);
    static char soilMoisture[7];
    dtostrf(soilmoisturepercent, 6, 2, soilMoisture);
    Serial.println("Soil moisture: ");
    Serial.println(soilMoisture);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/humidity", humidityTemp);
    client.publish("room/moisture",soilMoisture);
    client.publish("room/water",waterLevel);
 
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(hic);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");
  }
}