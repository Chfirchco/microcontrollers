#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "mqtt_connect.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

bool wifiConnected = false;
ESP8266WebServer server(80);

#define CONFIG_FILE "/config.json"

String loginForm = "<!DOCTYPE html>\
<html>\
<head>\
  <title>WiFi Configuration</title>\
  <style>\
    body {\
      font-family: Arial, sans-serif;\
      display: flex;\
      justify-content: center;\
      align-items: center;\
      height: 100vh;\
      margin: 0;\
      background-color: #f2f2f2;\
    }\
    .container {\
      background-color: #fff;\
      padding: 20px;\
      border-radius: 8px;\
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
      width: 300px;\
    }\
    h2 {\
      text-align: center;\
      margin-bottom: 20px;\
    }\
    label {\
      display: block;\
      margin: 10px 0 5px;\
    }\
    input[type='text'], input[type='password'] {\
      width: 100%;\
      padding: 8px;\
      box-sizing: border-box;\
      border: 1px solid #ccc;\
      border-radius: 4px;\
    }\
    input[type='submit'] {\
      width: 100%;\
      padding: 10px;\
      background-color: #4CAF50;\
      color: white;\
      border: none;\
      border-radius: 4px;\
      cursor: pointer;\
      margin-top: 20px;\
    }\
    input[type='submit']:hover {\
      background-color: #45a049;\
    }\
    .mac-address {\
      text-align: center;\
      margin-top: 10px;\
      font-size: 12px;\
      color: #555;\
    }\
  </style>\
</head>\
<body>\
  <div class='container'>\
    <h2>Configuration</h2>\
    <form action='/login' method='post'>\
      <label for='ssid'>SSID:</label>\
      <input type='text' id='ssid' name='ssid' placeholder='Enter SSID'>\
      <label for='password'>Password:</label>\
      <input type='password' id='password' name='password' placeholder='Enter Password'>\
      <label for='topicMaxPeople'>Topic Max People:</label>\
      <input type='text' id='topicMaxPeople' name='topicMaxPeople' value='max_people'>\
      <label for='topicAlarm'>Topic Alarm:</label>\
      <input type='text' id='topicAlarm' name='topicAlarm' value='alarm'>\
      <label for='topicClearLogs'>Topic Clear Logs:</label>\
      <input type='text' id='topicClearLogs' name='topicClearLogs' value='clear_logs'>\
      <label for='topicClearCounters'>Topic Clear Counters:</label>\
      <input type='text' id='topicClearCounters' name='topicClearCounters' value='clear_counters'>\
      <label for='topicCurrentPeople'>Topic Current People:</label>\
      <input type='text' id='topicCurrentPeople' name='topicCurrentPeople' value='current_people'>\
      <label for='topicTotalPeople'>Topic Total People:</label>\
      <input type='text' id='topicTotalPeople' name='topicTotalPeople' value='total_people'>\
      <input type='submit' value='Submit'>\
    </form>\
    <form action='/download' method='get'>\
      <input type='submit' value='Download logs'>\
    </form>\
    <div class='mac-address'>MAC Address: <span id='macAddress'></span></div>\
  </div>\
  <script>\
    document.getElementById('macAddress').innerText = '" + WiFi.macAddress() + "';\
  </script>\
</body>\
</html>";


String logsPage = "<!DOCTYPE html>\
<html>\
<head>\
  <title>Logs</title>\
  <style>\
    body {\
      font-family: Arial, sans-serif;\
      display: flex;\
      justify-content: center;\
      align-items: center;\
      height: 100vh;\
      margin: 0;\
      background-color: #f2f2f2;\
    }\
    .container {\
      background-color: #fff;\
      padding: 20px;\
      border-radius: 8px;\
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
      width: 600px;\
      max-height: 80vh;\
      overflow-y: auto;\
    }\
    h2 {\
      text-align: center;\
      margin-bottom: 20px;\
    }\
    pre {\
      background-color: #f8f8f8;\
      padding: 10px;\
      border-radius: 4px;\
      overflow-x: auto;\
      white-space: pre-wrap;\
      word-wrap: break-word;\
      border: 1px solid #ccc;\
    }\
  </style>\
</head>\
<body>\
  <div class='container'>\
    <h2>Logs</h2>\
    <pre id='logContent'></pre>\
  </div>\
  <script>\
    fetch('/download').then(response => response.json()).then(data => {\
      document.getElementById('logContent').innerText = JSON.stringify(data, null, 2);\
    }).catch(error => {\
      document.getElementById('logContent').innerText = 'Error loading logs: ' + error;\
    });\
  </script>\
</body>\
</html>";

// Function to save Wi-Fi credentials and topics to a JSON file
bool saveConfig(const String& ssid, const String& password, const String topics[]) {
    DynamicJsonDocument doc(1024);
    doc["ssid"] = ssid;
    doc["password"] = password;

    JsonArray topicsArray = doc.createNestedArray("topics");
    for (int i = 0; i < 6; ++i) {
        topicsArray.add(topics[i]);
    }

    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        return false;
    }

    configFile.close();
    return true;
}

// Function to load Wi-Fi credentials and topics from a JSON file
bool loadConfig(String& ssid, String& password, String topics[]) {
    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        return false;
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();

    JsonArray topicsArray = doc["topics"];
    for (int i = 0; i < 6; ++i) {
        topics[i] = topicsArray[i].as<String>();
    }

    configFile.close();
    return true;
}

// Function to clear the configuration file
bool clearConfig() {
    return LittleFS.remove(CONFIG_FILE);
}

// Function to get the MAC address and format topics
String getFormattedTopic(const String& userTopic) {
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

        // Save credentials and topics to JSON file
        if (saveConfig(ssid, password, topics)) {
            // Once connected to Wi-Fi, stop AP
            WiFi.softAPdisconnect(true);
            wifiConnected = true;
            setupMQTT(topics);
            server.send(200, "text/plain", "Successfully connected to Wi-Fi and MQTT!");
            server.close();
        } else {
            Serial.println("Failed to save config");
            server.send(500, "text/plain", "Failed to save config.");
        }
    } else {
        Serial.println("\nFailed to connect to WiFi");
        setupAP();
        server.send(200, "text/plain", "Failed to connect. Please re-enter your credentials.");
    }
}

void handleDownload() {
    if (!LittleFS.exists("/logs.json")) {
        server.send(404, "text/plain", "File not found");
        return;
    }

    File file = LittleFS.open("/logs.json", "r");
    String logs = file.readString();
    file.close();

    server.send(200, "application/json", logs);
}

void handleLogsPage() {
    server.send(200, "text/html", logsPage);
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/login", HTTP_POST, handleLogin);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/logs", HTTP_GET, handleLogsPage);
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
        if (loadConfig(ssid, password, topics)) {
            Serial.println(ssid);
            Serial.println(password);
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
        }

        Serial.println("\nFailed to reconnect to WiFi, setting up AP.");
        clearConfig();
        setupAP();
        setupWebServer();
        while (!wifiConnected) {
            handleWebServer();
        }
    }
}

#endif
