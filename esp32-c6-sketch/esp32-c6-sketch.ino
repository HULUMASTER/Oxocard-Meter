#include <Arduino.h>
#include "Adafruit_SGP40.h"
#include "Adafruit_SHT4x.h"
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = MQTT_BROKER;
int        port     = 1883;

Adafruit_SGP40 sgp;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup() {
  Serial.begin(115200);
  if (Serial) {
    while (!Serial) { delay(10); }
  }

  Serial.println("SGP40 test with SHT31 compensation");

  if (! sgp.begin()){
    Serial.println("SGP40 sensor not found :(");
    while (1); delay(1);
  }
  Serial.println("Found SGP40 sensor");


  if (! sht4.begin()) {
    Serial.println("SHT41 sensor not found");
    while (1) delay(1);
  }
  Serial.println("Found SHT41 sensor");

  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void loop() {
  sensors_event_t humidity, temp;
  uint16_t sraw;
  int32_t voc_index;
  
  sht4.getEvent(&humidity, &temp);

  float t = temp.temperature;
  float h = humidity.relative_humidity;
  sraw = sgp.measureRaw(t, h);
  voc_index = sgp.measureVocIndex(t, h);

  Serial.print("Temperature: "); Serial.print(t); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(h); Serial.println("% rH");
  Serial.print("SRAW: "); Serial.print(sraw); Serial.println("ppm");
  Serial.print("VOC: "); Serial.println(voc_index);

  StaticJsonDocument<200> doc;
  doc["temp"] = t;
  doc["hum"] = h;
  doc["raw"] = sraw;
  doc["voc"] = voc_index;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  // Send over MQTT
  mqttClient.poll();
  mqttClient.beginMessage("/nodes/node1/measurements");
  mqttClient.print(jsonBuffer);
  mqttClient.endMessage();

  delay(1 * 1000);
}
