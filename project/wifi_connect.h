#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "mqtt_connect.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

bool wifiConnected = false;
ESP8266WebServer server(80);

#define EEPROM_SIZE 512  // Увеличим размер EEPROM для хранения новых данных
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

#define TOPIC_BASE_ADDR 96
#define TOPIC_MAX_PEOPLE_ADDR (TOPIC_BASE_ADDR + 32)
#define TOPIC_ALARM_ADDR (TOPIC_MAX_PEOPLE_ADDR + 32)
#define TOPIC_CLEAR_LOGS_ADDR (TOPIC_ALARM_ADDR + 32)
#define TOPIC_CLEAR_COUNTERS_ADDR (TOPIC_CLEAR_LOGS_ADDR + 32)
#define TOPIC_CURRENT_PEOPLE_ADDR (TOPIC_CLEAR_COUNTERS_ADDR + 32)
#define TOPIC_TOTAL_PEOPLE_ADDR (TOPIC_CURRENT_PEOPLE_ADDR + 32)
#define MAX_TOPIC_LENGTH 32

String loginForm = "<form action='/login' method='post'>\
                      <label for='ssid'>SSID:</label><br>\
                      <input type='text' id='ssid' name='ssid'><br>\
                      <label for='password'>Password:</label><br>\
                      <input type='password' id='password' name='password'><br>\
                      <label for='topicMaxPeople'>topicMaxPeople:</label><br>\
                      <input type='text' id='topicMaxPeople' name='topicMaxPeople'><br>\
                      <label for='topicAlarm'>topicAlarm:</label><br>\
                      <input type='text' id='topicAlarm' name='topicAlarm'><br>\
                      <label for='topicClearLogs'>topicClearLogs:</label><br>\
                      <input type='text' id='topicClearLogs' name='topicClearLogs'><br>\
                      <label for='topicClearCounters'>topicClearCounters:</label><br>\
                      <input type='text' id='topicClearCounters' name='topicClearCounters'><br>\
                      <label for='topicCurrentPeople'>topicCurrentPeople:</label><br>\
                      <input type='text' id='topicCurrentPeople' name='topicCurrentPeople'><br>\
                      <label for='topicTotalPeople'>topicTotalPeople:</label><br>\
                      <input type='text' id='topicTotalPeople' name='topicTotalPeople'><br>\
                      <input type='submit' value='Submit'>\
                    </form>\
                    <form action='/download' method='get'>\
                      <input type='submit' value='Download logs'>\
                    </form>";

// Function to save Wi-Fi credentials and topics to EEPROM
void saveCredentialsAndTopics(const String& ssid, const String& password, const String topics[]) {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < MAX_SSID_LENGTH; ++i) {
        EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
    }
    for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
        EEPROM.write(PASSWORD_ADDR + i, i < password.length() ? password[i] : 0);
    }
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < MAX_TOPIC_LENGTH; ++j) {
            EEPROM.write(TOPIC_BASE_ADDR + i * MAX_TOPIC_LENGTH + j, j < topics[i].length() ? topics[i][j] : 0);
        }
    }
    EEPROM.commit();
}


// Function to load Wi-Fi credentials and topics from EEPROM
void loadCredentialsAndTopics(String& ssid, String& password, String topics[]) {
    char ssidArr[MAX_SSID_LENGTH];
    char passwordArr[MAX_PASSWORD_LENGTH];
    char topicArr[6][MAX_TOPIC_LENGTH];

    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < MAX_SSID_LENGTH; ++i) {
        ssidArr[i] = EEPROM.read(SSID_ADDR + i);
    }
    for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
        passwordArr[i] = EEPROM.read(PASSWORD_ADDR + i);
    }
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < MAX_TOPIC_LENGTH; ++j) {
            topicArr[i][j] = EEPROM.read(TOPIC_BASE_ADDR + i * MAX_TOPIC_LENGTH + j);
        }
        topics[i] = String(topicArr[i]);
    }
    ssid = String(ssidArr);
    password = String(passwordArr);
}


// Function to clear Wi-Fi credentials and topics from EEPROM
void clearCredentialsAndTopics() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

// Function to get the MAC address and format topics
String getFormattedTopic(const String& userTopic) {
  String macAddress2 = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress2);
  return WiFi.macAddress() + "/" + userTopic;
  
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP8266AP", "password"); // Имя сети и пароль для точки доступа
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());
}

void handleRoot() {
  server.send(200, "text/html", loginForm);
}

void handleLogin() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String topics[6] = {
        getFormattedTopic(server.arg("topicMaxPeople")),
        getFormattedTopic(server.arg("topicAlarm")),
        getFormattedTopic(server.arg("topicClearLogs")),
        getFormattedTopic(server.arg("topicClearCounters")),
        getFormattedTopic(server.arg("topicCurrentPeople")),
        getFormattedTopic(server.arg("topicTotalPeople"))
    };

    // Connect to the provided Wi-Fi network
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print(password);
    WiFi.begin(ssid.c_str(), password.c_str());
    int attemptCount = 0;
    const int maxAttempts = 20;  // Максимальное количество попыток подключения
    const int delayTime = 1000;  // Время задержки между попытками подключения в миллисекундах

    while (WiFi.status() != WL_CONNECTED && attemptCount < maxAttempts) {
        delay(delayTime);
        Serial.print(".");
        attemptCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");

        // Save credentials and topics to EEPROM
        saveCredentialsAndTopics(ssid, password, topics);

        // Once connected to Wi-Fi, stop AP
        WiFi.softAPdisconnect(true);
        wifiConnected = true;
        setupMQTT(topics);
        server.send(200, "text/plain", "Successfully connected to Wi-Fi and MQTT!");
        server.close();
    } else {
        Serial.println("\nFailed to connect to WiFi");
        setupAP();
        server.send(200, "text/plain", "Failed to connect. Please re-enter your credentials.");
    }
}


void handleDownload() {
    File file = LittleFS.open("/logs.json", "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }

    server.streamFile(file, "application/json");
    file.close();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/download", HTTP_GET, handleDownload);  // Добавить этот обработчик
  server.begin();
  Serial.println("HTTP server started");
}

void handleWebServer() {
  server.handleClient();
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void reconnectWiFi() {
    if (!wifiConnected) {
        String ssid, password;
        String topics[6];
        loadCredentialsAndTopics(ssid, password, topics);

        if (ssid.length() > 0 && password.length() > 0) {
            WiFi.begin(ssid.c_str(), password.c_str());
            int attemptCount = 0;
            const int maxAttempts = 20;  // Maximum connection attempts
            const int delayTime = 1000;  // Delay between connection attempts in milliseconds

            while (WiFi.status() != WL_CONNECTED && attemptCount < maxAttempts) {
                delay(delayTime);
                Serial.print(".");
                attemptCount++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nReconnected to WiFi");
                wifiConnected = true;
                setupMQTT(topics); // Pass the topics array to MQTT setup
                return;
            }
        }

        Serial.println("\nFailed to reconnect to WiFi, setting up AP.");
        clearCredentialsAndTopics();
        setupAP();
        setupWebServer();
        while (!wifiConnected) {
            handleWebServer();
        }
    }
}



#endif
