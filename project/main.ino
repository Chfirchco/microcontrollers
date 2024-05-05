#include <SoftwareSerial.h>
#include <TM1637Display.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
//wi-fi
const char* ssid = "VladOS";
const char* password = "1234567890";
const char* mqtt_server = "public.mqtthq.com";
const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

TM1637Display display(5, 4);  // D1, D2

// ------------------------------------Переменные----------------------------------------
// Определение, где подключены компоненты
#define LED_WAIT 12   // D6
#define LED_ENTER 13  // D7
#define BUZZER_IN 15  // D8
//int sensor1[] = {4,0}; // D2, D3
int sensor1[] = { 16, 0 };  // D0, D3
int sensor2[] = { 14, 2 };  // D5, D4


// Определение используемых переменных устройством
#define iterations 5  // Количество чтений на этапе калибровки
// Расстояние по умолчанию (в см) используется только если калибровка не удалась.
#define DEFAULT_DISTANCE 100
#define MAX_DISTANCE 300
#define MIN_DISTANCE 10

// Калибровка в функции setup() установит след. переменные на соответствующие значения.
float sensor1Initial = DEFAULT_DISTANCE, sensor2Initial = DEFAULT_DISTANCE;
// Это расстояния (в см), которые считывают каждый из ультразвуковых датчиков.
float sensor1Val, sensor2Val;
int currentPeople = 0;
// Предел максимального количества человек должен быть установлен здесь
int maxPeople = 10;
int dailyPeople = 0;
// Эти булевы значения записывают, был ли вход/выход заблокирован на предыдущем
// чтении датчика.
bool prevInblocked = false, prevOutblocked = false;

String sequence = "";

int timeoutCounter = 0;

// переменная для хранения времени последнего срабатывания
unsigned long previousMillisBuzzer = 0;
const long interval = 15;   // интервал времени между срабатываниями (в миллисекундах)
int buzzerFrequency = 700;  // начальная частота для пищалки

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
bool soundDirection = true;  // направление тональности звука вверх-вниз
// // -----------------------------------------------------------------------------------

const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,          // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
  SEG_C | SEG_E | SEG_G,                          // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G           // E
};

const uint8_t CONN[] = {
  SEG_G,  // -
  SEG_G,  // -
  SEG_G,  // -
  SEG_G   // -
};
// ---------------------------------------SETUP------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_WAIT, OUTPUT), pinMode(LED_ENTER, OUTPUT);
  calibrateDistance();
  display.setBrightness(7);  // Установка яркости дисплея
  display.setSegments(CONN);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(MQTTcallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266")) {
      Serial.println("connected");
      display.setSegments(done);
      delay(3000);
      display.showNumberDec(0, false);  // Показать 0 на дисплее после успешного подключения
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  client.subscribe("esp_counter/max_people");
  client.subscribe("esp_counter/alarm");
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
      Serial.println("Numeric message received: ");
      // Преобразуем сообщение в число и обновляем переменную maxPeople
      String message;
      for (int i = 0; i < length; i++) {
        message += (char)payload[i];
      }
      int newValue = message.toInt();  // Преобразуем строку в число
      maxPeople = newValue;            // Обновляем переменную maxPeople
      Serial.print("Updated maxPeople to: ");
      Serial.println(maxPeople);
    } else {
      // Обработка сообщения, не содержащего только цифры
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
      noTone(BUZZER_IN);  // Отключение пищалки
    }
    Serial.print("Message: ");
    Serial.print(message);
  }

  Serial.println();
  Serial.println("-----------------------");
}


// ----------------------------------------LOOP------------------------------------------
void loop() {
  //

  client.loop();
  unsigned long pub_currentMillis = millis();  // получаем текущее время

  // проверяем, прошло ли уже достаточно времени для отправки сообщения
  if (pub_currentMillis - pub_previousMillis >= publish_interval) {
    // сохраняем текущее время как последнее время отправки сообщения
    pub_previousMillis = pub_currentMillis;

    // ваш код для отправки сообщения
    client.publish("esp_counter/current_people", ("Текущее количество людей: " + String(currentPeople)).c_str());
    client.publish("esp_counter/all_people", String(dailyPeople).c_str());
  }


  // ------------------------------Считывание-данных-с-сенсоров--------------------------
  unsigned long currentMillisDistance = millis();  // получаем текущее время
  if (currentMillisDistance - previousMillisDistance >= interval1) {
    // проверяем, прошло ли нужное количество времени
    previousMillisDistance = currentMillisDistance;  // сохраняем текущее время
    sensor1Val = measureDistance(sensor1);
  }
  currentMillisDistance = millis();
  if (currentMillisDistance - previousMillisDistance2 >= interval2) {
    previousMillisDistance2 = currentMillisDistance;
    sensor2Val = measureDistance(sensor2);
    // Serial.println(sensor2Val);
  }
  // ------------------------------------------------------------------------------------

  // ----------------------------Обработка-данных-с-сенсоров-----------------------------

  //Process the data
  unsigned long currentMillisSequence = millis();  // получаем текущее время
  if (currentMillisSequence - previousMillisSequence >= interval3) {
    // проверяем, прошло ли нужное количество времени
    previousMillisSequence = currentMillisSequence;  // сохраняем текущее время
    //Serial.println("millis");
    if (sensor1Val < sensor1Initial - 15 && prevInblocked == false) {
      sequence += "1";
      prevInblocked = true;
      // Serial.println("perviy");
    } else if (sensor1Val >= sensor1Initial - 15) {
      prevInblocked = false;
    }
    if (sensor2Val < sensor2Initial - 15 && prevOutblocked == false) {
      sequence += "2";
      prevOutblocked = true;
      // Serial.println("vorviy");
    } else if (sensor2Val >= sensor2Initial - 15) {
      prevOutblocked = false;
    }
    //Serial.println(sequence);
  }

  if (sequence.equals("12")) {
    currentPeople++;
    Serial.println("+");
    sequence = "";
    dailyPeople++;
    //delay(550);

  } else if (sequence.equals("21") && currentPeople > 0) {
    currentPeople--;
    Serial.println("-");
    sequence = "";
    //delay(550);
  }

  //Resets the sequence if it is invalid or timeouts
  /*if(timeoutCounter > 200){
    sequence="";
  }
  if(sequence.length() == 1){ //
    timeoutCounter++;
  }else{
    timeoutCounter=0;
  }*/

  unsigned long currentMillisTimer = millis();  // получаем текущее время
  if (currentMillisTimer - previousMillisTimer >= interval4) {
    // проверяем, прошло ли нужное количество времени
    previousMillisTimer = currentMillisSequence;  // сохраняем текущее время
    if (timeoutCounter > 20) {
      Serial.println(sequence);
      sequence = "";
    }
    if (sequence.length() == 1 || sequence.length() > 2) {  //
      timeoutCounter++;
    } else {
      timeoutCounter = 0;
    }
  }

  //Print values to serial
  //Serial.print("Seq: ");
  //Serial.print(sequence[0]);
  //Serial.print(sequence[1]);
  //Serial.print(" S1: ");
  //Serial.print(sensor1Val);
  //Serial.print(" S2: ");
  //Serial.println(sensor2Val);

  //Serial.print("People counter: ");
  //Serial.println(currentPeople);
  String textValue = String(currentPeople);
  //Serial.println(currentPeople);
  while (textValue.length() < 4) {
    textValue = " " + textValue;
  }
  //showOnDisplayAdafruitLEDBackpack(textValue);

  display.showNumberDec(currentPeople, false);  // Expect: 0301
  // String people_value = String(currentPeople);
  // server.send(200, "text/plane", people_value);


  // tm.display(3, currentPeople % 10);
  // int pos2 = currentPeople / 10;
  // tm.display(2, pos2 % 10);
  // int pos1 = pos2 / 10;
  // tm.display(1, pos1 % 10);
  // int pos0 = pos1 / 10;
  // tm.display(0, pos0 % 10);

  // ------------------------------------------------------------------------------------

  // -----------------------Управление-лампочками-и-пьезоэлементом-----------------------
  // Если в %PLACE_NAME% меньше людей, чем лимит, свет горит зеленым,
  // в противном случае - красным.
  // Если количество людей ещё больше, то срабатывает пищалка.
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
  // ------------------------------------------------------------------------------------
}
// --------------------------------------------------------------------------------------


// --------------------------------------Функции-----------------------------------------
// Включает пьезоэлемент, водит частоту туда-сюда
void buzzerOn() {
  unsigned long currentMillisBuzzer = millis();  // получаем текущее время
  if (currentMillisBuzzer - previousMillisBuzzer >= interval) {
    // проверяем, прошло ли нужное количество времени
    // сохраняем текущее время
    previousMillisBuzzer = currentMillisBuzzer;
    tone(BUZZER_IN, buzzerFrequency);
    if (soundDirection) {
      buzzerFrequency = buzzerFrequency + 15;
    } else {
      buzzerFrequency = buzzerFrequency - 15;
    }
    // если частота достигла 800 или 700, начинаем уменьшать или увеличивать её
    if (buzzerFrequency >= 800 || buzzerFrequency <= 700) {
      soundDirection = !soundDirection;
    }
  }
}


// Измеряет дистанцию. Возвращает расстояние ультразвукового датчика, который передаёт
// a[0] = echo, a[1] = trig
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

// Калибровка расстояния до стены, чтобы знать, когда находится объект до стены
void calibrateDistance() {
  // Оба светодиода включены, чтобы предупредить пользователя о текущей калибровке.
  digitalWrite(LED_WAIT, HIGH);
  digitalWrite(LED_ENTER, HIGH);
  Serial.println("Calibrating...");
  for (int a = 0; a < iterations; a++) {
    delay(40);
    sensor1Initial += measureDistance(sensor1);
    delay(40);
    sensor2Initial += measureDistance(sensor2);
  }
  //Пороговое значение установлено на уровне 75% от среднего значения этих показаний.
  //Это должно помешать системе подсчитывать людей в случае сбоя.
  sensor1Initial = 0.75 * sensor1Initial / iterations;
  sensor2Initial = 0.75 * sensor2Initial / iterations;
  if (sensor1Initial > MAX_DISTANCE || sensor1Initial < MIN_DISTANCE) {
    // Если калибровка дала показания, выходящие за разумные пределы, то используется
    // значение по умолчанию
    sensor1Initial = DEFAULT_DISTANCE;
  }
  if (sensor2Initial > MAX_DISTANCE || sensor2Initial < MIN_DISTANCE) {
    sensor2Initial = DEFAULT_DISTANCE;
  }
  // Оба светодиода выключены, чтобы предупредить пользователя о завершении калибровки
  digitalWrite(LED_WAIT, LOW);
  digitalWrite(LED_ENTER, LOW);
}
// --------------------------------------------------------------------------------------
