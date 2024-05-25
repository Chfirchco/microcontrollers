#include <SoftwareSerial.h>
#include <TM1637Display.h>
#include "wifi_connect.h"
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define EEPROM addresses for storing variables
#define EEPROM_CURRENT_PEOPLE_ADDR 0
#define EEPROM_MAX_PEOPLE_ADDR 4
#define EEPROM_DAILY_PEOPLE_ADDR 8

// Define pins for the display
TM1637Display display(5, 4);  // D1, D2

// Define pins for LEDs and buzzer
#define LED_WAIT 12  // D6
#define LED_ENTER 13 // D7
#define BUZZER_IN 15 // D8

// Define pins for sensors
int sensor1[] = {16, 0}; // D0, D3
int sensor2[] = {14, 2}; // D5, D4

// Define other constants and variables
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
  SEG_G,
  SEG_G,
  SEG_G,
  SEG_G
};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 60 seconds update interval

void setup() {
    Serial.begin(115200);

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    
    display.setBrightness(7);
    display.setSegments(CONN);

    // Initialize EEPROM
    EEPROM.begin(512);

    // Load saved variables
    loadVariables();

    // Attempt to reconnect to saved Wi-Fi credentials
    reconnectWiFi();

    // Wait until Wi-Fi is connected
    while (!wifiConnected) {
        handleWebServer();
    }

    timeClient.begin();
    while (!timeClient.update()) {
        timeClient.forceUpdate();
    }

    setTime(timeClient.getEpochTime()); // Set the time using the NTP client

    display.setSegments(done);
    delay(3000);
    display.showNumberDec(0, false);
    pinMode(LED_WAIT, OUTPUT);
    pinMode(LED_ENTER, OUTPUT);
    calibrateDistance();
}

void loop() {
    timeClient.update();
    setTime(timeClient.getEpochTime());
    // MQTT loop
    client.loop();

    // Publish MQTT messages
    unsigned long pub_currentMillis = millis();
    if (pub_currentMillis - pub_previousMillis >= publish_interval) {
        pub_previousMillis = pub_currentMillis;
        client.publish("esp_counter/current_people", ("Текущее количество людей: " + String(currentPeople)).c_str());
        client.publish("esp_counter/all_people", String(dailyPeople).c_str());
    }

    // Distance sensor 1 measurement
    unsigned long currentMillisDistance = millis();
    if (currentMillisDistance - previousMillisDistance >= interval1) {
        previousMillisDistance = currentMillisDistance;
        sensor1Val = measureDistance(sensor1);
    }

    // Distance sensor 2 measurement
    currentMillisDistance = millis();
    if (currentMillisDistance - previousMillisDistance2 >= interval2) {
        previousMillisDistance2 = currentMillisDistance;
        sensor2Val = measureDistance(sensor2);
    }

    // Update sequence based on sensor values
    unsigned long currentMillisSequence = millis();
    if (currentMillisSequence - previousMillisSequence >= interval3) {
        previousMillisSequence = currentMillisSequence;
        if (sensor1Val < sensor1Initial - 15 && !prevInblocked) {
            sequence += "1";
            prevInblocked = true;
            Serial.println(sequence);
        } else if (sensor1Val >= sensor1Initial - 15) {
            prevInblocked = false;
        }
        if (sensor2Val < sensor2Initial - 15 && !prevOutblocked) {
            sequence += "2";
            prevOutblocked = true;
            Serial.println(sequence);
        } else if (sensor2Val >= sensor2Initial - 15) {
            prevOutblocked = false;
        }
    }

    // Update people count based on sequence
    if (sequence.equals("12")) {
        currentPeople++;
        sequence = "";
        dailyPeople++;
        saveVariables();
        logPeopleCountChange(currentPeople); // Log the change
    } else if (sequence.equals("21") && currentPeople > 0) {
        currentPeople--;
        sequence = "";
        saveVariables();
        logPeopleCountChange(currentPeople); // Log the change
    }

    // Handle sequence timeout
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

    // Display and control LEDs based on people count
    display.showNumberDec(currentPeople, false);

    if (currentPeople < maxPeople) {
        digitalWrite(LED_WAIT, LOW);
        digitalWrite(LED_ENTER, HIGH);
    } else {
        digitalWrite(LED_WAIT, HIGH);
        digitalWrite(LED_ENTER, LOW);
        if (currentPeople > maxPeople) {
            buzzerOn();
        } else {
            noTone(BUZZER_IN);
        }
    }

    // Check Wi-Fi connection and re-setup if disconnected
    if (!isWiFiConnected()) {
        Serial.println("WiFi disconnected, setting up AP again.");
        wifiConnected = false;
        reconnectWiFi();
        while (!wifiConnected) {
            handleWebServer();
        }
        Serial.println("Reconnected to WiFi.");
    }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message received in topic: ");
    Serial.println(topic);

    if (String(topic) == "esp_counter/max_people") {
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
            maxPeople = newValue;
            Serial.print("Updated maxPeople to: ");
            Serial.println(maxPeople);
            saveVariables();
        } else {
            String message;
            for (int i = 0; i < length; i++) {
                message += (char)payload[i];
            }
            Serial.println("Non-numeric message received: " + message);
        }
    } else if (String(topic) == "esp_counter/alarm") {
        String message;
        for (int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        if (message == "on") {
            buzzerOn();
        } else if (message == "off") {
            noTone(BUZZER_IN);
        }
        Serial.print("Message: ");
        Serial.print(message);
    }
    else if (String(topic) == "esp_counter/clear_logs") {
        String message;
        for (int i = 0; i < length; i++) {
            message += (char)payload[i];
            Serial.println(message);
        }
        if (message == "clear") {
            clearLogs();
        } else {
            Serial.println("Invalid message for clearing logs");
        }
    }
    else if (String(topic) == "esp_counter/clear_counters") {
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
    Serial.println();
    Serial.println("-----------------------");
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
    currentPeople = 0;
    dailyPeople = 0;
    // Обнуляем значения в EEPROM
    EEPROM.put(EEPROM_CURRENT_PEOPLE_ADDR, currentPeople);
    EEPROM.put(EEPROM_DAILY_PEOPLE_ADDR, dailyPeople);
    EEPROM.commit();
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
}

// Functions to save and load variables from EEPROM
void saveVariables() {
    EEPROM.put(EEPROM_CURRENT_PEOPLE_ADDR, currentPeople);
    EEPROM.put(EEPROM_MAX_PEOPLE_ADDR, maxPeople);
    EEPROM.put(EEPROM_DAILY_PEOPLE_ADDR, dailyPeople);
    EEPROM.commit();
}

void loadVariables() {
    EEPROM.get(EEPROM_CURRENT_PEOPLE_ADDR, currentPeople);
    EEPROM.get(EEPROM_MAX_PEOPLE_ADDR, maxPeople);
    EEPROM.get(EEPROM_DAILY_PEOPLE_ADDR, dailyPeople);

    // Ensure the loaded values are valid
    if (currentPeople < 0) currentPeople = 0;
    if (maxPeople <= 0) maxPeople = 10;
    if (dailyPeople < 0) dailyPeople = 0;
}

void logPeopleCountChange(int peopleCount) {
    File file = LittleFS.open("/logs.json", "a");
    if (!file) {
        Serial.println("Failed to open log file for writing");
        return;
    }

    // Получаем текущее время в секундах с epoch
    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = localtime(&now);

    // Форматируем строку даты и времени
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Создаем объект JSON для записи в файл
    StaticJsonDocument<200> doc;
    doc["datetime"] = buffer; // Дата и время в человеческом формате
    doc["people_count"] = peopleCount;

    // Сериализуем объект JSON и записываем в файл
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to log file");
    }

    file.close();
}
