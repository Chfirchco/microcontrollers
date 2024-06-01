#include <SoftwareSerial.h>
#include <TM1637Display.h>
#include "wifi_connect.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define CONFIG_FILE "/config.json"

// Define pins for the display
TM1637Display display(5, 4);  // D1, D2

// Define pins for LEDs and buzzer
#define LED_WAIT 12  // D6
#define LED_ENTER 13 // D7
#define BUZZER_IN 15 // D8

// Define pins for sensors
int sensor1[] = {16, 0}; // D0, D3
int sensor2[] = {14, 2}; // D5, D4

#define iterations 5
#define DEFAULT_DISTANCE 100
#define MAX_DISTANCE 300
#define MIN_DISTANCE 10

float sensor1Initial = DEFAULT_DISTANCE, sensor2Initial = DEFAULT_DISTANCE;
float sensor1Val, sensor2Val;
int currentPeople = 0;
int maxPeople = 10;
int dailyPeople = 0;
bool prevInblocked = false, prevOutblocked = false;
String sequence = "";
int timeoutCounter = 0;

unsigned long previousMillisBuzzer = 0;
const long interval = 15;
int buzzerFrequency = 700;

unsigned long previousMillisDistance = 0;
const long interval1 = 100;

unsigned long previousMillisDistance2 = 0;
const long interval2 = 100;

unsigned long previousMillisSequence = 0;
const long interval3 = 100;

unsigned long previousMillisTimer = 0;
const long interval4 = 10;

const long publish_interval = 10000;
unsigned long pub_previousMillis = 0;
bool soundDirection = true;

const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
  SEG_C | SEG_E | SEG_G,
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G
};

const uint8_t CONN[] = {
 0x39, // C
    0x3F, // O
    0x37, // N
    0x37  // N
};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 60 seconds update interval

struct IoTConfig {
    char ssid[32];
    char password[32];
    char topics[6][32];
    int currentPeople;
    int maxPeople;
    int dailyPeople;
};

IoTConfig config;

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    display.setBrightness(7);
    display.setSegments(CONN);

    loadConfig();
    validateConfig();
    saveConfig();

    reconnectWiFi();

    while (!wifiConnected) {
        handleWebServer();
    }

    timeClient.begin();
    while (!timeClient.update()) {
        timeClient.forceUpdate();
    }

    setTime(timeClient.getEpochTime());

    display.setSegments(done);
    delay(3000);
    display.showNumberDec(0, false);
    pinMode(LED_WAIT, OUTPUT);
    pinMode(LED_ENTER, OUTPUT);
    calibrateDistance();
    printSubscribedTopics();
}

void loop() {
    timeClient.update();
    setTime(timeClient.getEpochTime());
    client.loop();

    unsigned long currentMillisDistance = millis();
    if (currentMillisDistance - previousMillisDistance >= interval1) {
        previousMillisDistance = currentMillisDistance;
        sensor1Val = measureDistance(sensor1);
    }

    currentMillisDistance = millis();
    if (currentMillisDistance - previousMillisDistance2 >= interval2) {
        previousMillisDistance2 = currentMillisDistance;
        sensor2Val = measureDistance(sensor2);
    }

    unsigned long currentMillisSequence = millis();
    if (currentMillisSequence - previousMillisSequence >= interval3) {
        previousMillisSequence = currentMillisSequence;
        if (sensor1Val < sensor1Initial - 15 && !prevInblocked) {
            sequence += "1";
            prevInblocked = true;
            // Serial.println("1");
            // Serial.println(sensor1Val);
        } else if (sensor1Val >= sensor1Initial - 15) {
            prevInblocked = false;
        }
        if (sensor2Val < sensor2Initial - 15 && !prevOutblocked) {
            sequence += "2";
            prevOutblocked = true;
            // Serial.println("2");
            // Serial.println(sensor2Val);
        } else if (sensor2Val >= sensor2Initial - 15) {
            prevOutblocked = false;
        }
    }

    if (sequence.equals("12")) {
        config.currentPeople++;
        sequence = "";
        config.dailyPeople++;
        saveConfig();
        logPeopleCountChange(config.currentPeople);
        Serial.println("Person entered, currentPeople: " + String(config.currentPeople));
    } else if (sequence.equals("21") && config.currentPeople > 0) {
        config.currentPeople--;
        sequence = "";
        saveConfig();
        logPeopleCountChange(config.currentPeople);
        Serial.println("Person exited, currentPeople: " + String(config.currentPeople));
    }

    unsigned long currentMillisTimer = millis();
    if (currentMillisTimer - previousMillisTimer >= interval4) {
        previousMillisTimer = currentMillisSequence;
        if (timeoutCounter > 20) {
            sequence = "";
        }
        if (sequence.length() == 1 || sequence.length() > 2) {
            timeoutCounter++;
        } else {
            timeoutCounter = 0;
        }
    }

    display.showNumberDec(config.currentPeople, false);

    if (config.currentPeople < config.maxPeople) {
        digitalWrite(LED_WAIT, LOW);
        digitalWrite(LED_ENTER, HIGH);
    } else {
        digitalWrite(LED_WAIT, HIGH);
        digitalWrite(LED_ENTER, LOW);
        if (config.currentPeople > config.maxPeople) {
            buzzerOn();
        } else {
            noTone(BUZZER_IN);
        }
    }

    if (!isWiFiConnected()) {
        Serial.println("WiFi disconnected, setting up AP again.");
        wifiConnected = false;
        reconnectWiFi();
        while (!wifiConnected) {
            handleWebServer();
        }
        Serial.println("Reconnected to WiFi.");
    }

    unsigned long pub_currentMillis = millis();
    if (pub_currentMillis - pub_previousMillis >= publish_interval) {
        pub_previousMillis = pub_currentMillis;
        publishMQTTMessages();
    }
}

void publishMQTTMessages() {
    client.publish(config.topics[4], String(config.currentPeople).c_str());
    client.publish(config.topics[5], String(config.dailyPeople).c_str());
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
    String topicStr(topic);

    Serial.print("Message received in topic: ");
    Serial.println(topicStr);

    if (topicStr == config.topics[0]) {
        handleMaxPeople(payload, length);
    } else if (topicStr == config.topics[1]) {
        handleAlarm(payload, length);
    } else if (topicStr == config.topics[2]) {
        handleClearLogs(payload, length);
    } else if (topicStr == config.topics[3]) {
        handleClearCounters(payload, length);
    }

    Serial.println("-----------------------");
}

void handleMaxPeople(byte* payload, unsigned int length) {
    bool isNumeric = true;
    for (int i = 0; i < length; i++) {
        if (!isdigit(payload[i])) {
            isNumeric = false;
            break;
        }
    }

    if (isNumeric) {
        String message;
        for (int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        int newValue = message.toInt();
        config.maxPeople = newValue;
        Serial.print("Updated maxPeople to: ");
        Serial.println(config.maxPeople);
        saveConfig();
    } else {
        String message;
        for (int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        Serial.println("Non-numeric message received: " + message);
    }
}

void handleAlarm(byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    if (message == "on") {
        buzzerOn();
    } else if (message == "off") {
        noTone(BUZZER_IN);
    } else if (message == "cal") {
        calibrateDistance();
    } else if (message == "clear") {
        clearConfigAndRestart();
    }
    Serial.print("Message: ");
    Serial.print(message);
}

void handleClearLogs(byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    if (message == "clear") {
        clearLogs();
    } else {
        Serial.println("Invalid message for clearing logs");
    }
}

void handleClearCounters(byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    if (message == "clear") {
        clearCounters();
    } else {
        Serial.println("Invalid message for clearing counters");
    }
}

void clearLogs() {
    Serial.println("clear_logs tipa");
    if (LittleFS.remove("/logs.json")) {
        Serial.println("Logs successfully cleared");
    } else {
        Serial.println("Failed to clear logs");
    }
}

void clearCounters() {
    config.currentPeople = 0;
    config.dailyPeople = 0;
    saveConfig();
    Serial.println("Counters cleared");
}

void buzzerOn() {
    unsigned long currentMillisBuzzer = millis();
    if (currentMillisBuzzer - previousMillisBuzzer >= interval) {
        previousMillisBuzzer = currentMillisBuzzer;
        tone(BUZZER_IN, buzzerFrequency);
        if (soundDirection) {
            buzzerFrequency += 15;
        } else {
            buzzerFrequency -= 15;
        }
        if (buzzerFrequency >= 800 || buzzerFrequency <= 700) {
            soundDirection = !soundDirection;
        }
    }
}

int measureDistance(int a[]) {
    pinMode(a[1], OUTPUT);
    digitalWrite(a[1], LOW);
    delayMicroseconds(4);
    digitalWrite(a[1], HIGH);
    delayMicroseconds(10);
    digitalWrite(a[1], LOW);
    pinMode(a[0], INPUT);
    long distance = pulseIn(a[0], HIGH, 100000);
    return distance / 29 / 2;
}

void calibrateDistance() {
    digitalWrite(LED_WAIT, HIGH);
    digitalWrite(LED_ENTER, HIGH);
    Serial.println("Calibrating...");
    for (int a = 0; a < iterations; a++) {
        delay(40);
        sensor1Initial += measureDistance(sensor1);
        delay(40);
        sensor2Initial += measureDistance(sensor2);
    }
    sensor1Initial = 0.75 * sensor1Initial / iterations;
    sensor2Initial = 0.75 * sensor2Initial / iterations;
    if (sensor1Initial > MAX_DISTANCE || sensor1Initial < MIN_DISTANCE) {
        sensor1Initial = DEFAULT_DISTANCE;
    }
    if (sensor2Initial > MAX_DISTANCE || sensor2Initial < MIN_DISTANCE) {
        sensor2Initial = DEFAULT_DISTANCE;
    }
    digitalWrite(LED_WAIT, LOW);
    digitalWrite(LED_ENTER, LOW);
    Serial.println("calibrated koroche");
    Serial.println(sensor1Initial);
    Serial.println(sensor2Initial);
}

void saveConfig() {
    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    for (int i = 0; i < 6; ++i) {
        doc["topics"][i] = config.topics[i];
    }
    doc["currentPeople"] = config.currentPeople;
    doc["maxPeople"] = config.maxPeople;
    doc["dailyPeople"] = config.dailyPeople;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write config file");
    }

    configFile.close();
    Serial.println("Config saved");
}

void loadConfig() {
    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("Failed to open config file for reading");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to read config file");
        return;
    }

    strlcpy(config.ssid, doc["ssid"] | "", sizeof(config.ssid));
    strlcpy(config.password, doc["password"] | "", sizeof(config.password));
    for (int i = 0; i < 6; ++i) {
        strlcpy(config.topics[i], doc["topics"][i] | "", sizeof(config.topics[i]));
    }
    config.currentPeople = doc["currentPeople"] | 0;
    config.maxPeople = doc["maxPeople"] | 10;
    config.dailyPeople = doc["dailyPeople"] | 0;

    configFile.close();
    Serial.println("Config loaded");
}

void validateConfig() {
    if (config.currentPeople < 0 || config.currentPeople > 10000) {
        config.currentPeople = 0;
    }
    if (config.maxPeople <= 0 || config.maxPeople > 1000) {
        config.maxPeople = 10;
    }
    if (config.dailyPeople < 0 || config.dailyPeople > 10000) {
        config.dailyPeople = 0;
    }
}

void logPeopleCountChange(int peopleCount) {
    File file = LittleFS.open("/logs.json", "a");
    if (!file) {
        Serial.println("Failed to open log file for writing");
        return;
    }

    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = localtime(&now);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    StaticJsonDocument<200> doc;
    doc["datetime"] = buffer;
    doc["people_count"] = peopleCount;

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to log file");
    }

    file.close();
}
void printSubscribedTopics() {
    Serial.println("Subscribed topics:");
    for (int i = 0; i < 6; ++i) {
        if (strlen(config.topics[i]) > 0) {
            Serial.println(config.topics[i]);
        }
    }
}
void clearConfigAndRestart() {
    if (LittleFS.remove(CONFIG_FILE)) {
        Serial.println("Config file successfully cleared, restarting...");
        ESP.restart();
    } else {
        Serial.println("Failed to clear config file");
    }
}