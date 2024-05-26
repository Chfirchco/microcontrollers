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

void setupMQTT(String topics[]) {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(MQTTcallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266")) {
      Serial.println("connected");
      for (int i = 0; i < 4; ++i) {
        client.subscribe(topics[i].c_str());
      }
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

#endif
