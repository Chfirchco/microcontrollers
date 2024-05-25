// mqtt_connect.h
#ifndef MQTT_CONNECT_H
#define MQTT_CONNECT_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// MQTT server details
const char* mqtt_server = "public.mqtthq.com";
const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

void MQTTcallback(char* topic, byte* payload, unsigned int length);

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(MQTTcallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266")) {
      Serial.println("connected");
      client.subscribe("esp_counter/max_people");
      client.subscribe("esp_counter/alarm");
      client.subscribe("esp_counter/clear_logs");
      client.subscribe("esp_counter/clear_counters");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

#endif
