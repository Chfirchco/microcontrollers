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

#define EEPROM_SIZE 96
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

String loginForm = "<form action='/login' method='post'>\
                      <label for='ssid'>SSID:</label><br>\
                      <input type='text' id='ssid' name='ssid'><br>\
                      <label for='password'>Password:</label><br>\
                      <input type='password' id='password' name='password'><br><br>\
                      <input type='submit' value='Submit'>\
                    </form>\
                    <form action='/download' method='get'>\
                      <input type='submit' value='Download logs'>\
                    </form>";

// Function to save Wi-Fi credentials to EEPROM
void saveCredentials(const String& ssid, const String& password) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < MAX_SSID_LENGTH; ++i) {
    EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
  }
  for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
    EEPROM.write(PASSWORD_ADDR + i, i < password.length() ? password[i] : 0);
  }
  EEPROM.commit();
}

// Function to load Wi-Fi credentials from EEPROM
void loadCredentials(String& ssid, String& password) {
  char ssidArr[MAX_SSID_LENGTH];
  char passwordArr[MAX_PASSWORD_LENGTH];

  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < MAX_SSID_LENGTH; ++i) {
    ssidArr[i] = EEPROM.read(SSID_ADDR + i);
  }
  for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
    passwordArr[i] = EEPROM.read(PASSWORD_ADDR + i);
  }
  ssid = String(ssidArr);
  password = String(passwordArr);
}

// Function to clear Wi-Fi credentials from EEPROM
void clearCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < MAX_SSID_LENGTH; ++i) {
    EEPROM.write(SSID_ADDR + i, 0);
  }
  for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
    EEPROM.write(PASSWORD_ADDR + i, 0);
  }
  EEPROM.commit();
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
    
    // Save credentials to EEPROM
    saveCredentials(ssid, password);

    // Once connected to Wi-Fi, stop AP
    WiFi.softAPdisconnect(true);
    wifiConnected = true;
    setupMQTT();
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
    loadCredentials(ssid, password);

    if (ssid.length() > 0 && password.length() > 0) {
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
        Serial.println("\nReconnected to WiFi");
        wifiConnected = true;
        setupMQTT();
        return;
      }
    }

    Serial.println("\nFailed to reconnect to WiFi, setting up AP.");
    clearCredentials();
    setupAP();
    setupWebServer();
    while (!wifiConnected) {
      handleWebServer();
    }
  }
}

#endif
