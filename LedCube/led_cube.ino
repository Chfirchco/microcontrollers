
int ledPins[6] = {3, 5, 6, 9, 12, 13}; // Определение 6 пинов для светодиодов

int delayTime = 100; // Задержка между переключениями

void setup() {
  for (int i = 0; i < 6; i++) { // Настройка пинов как OUTPUT
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  for (int i = 0; i < 256; i++) { // Цикл по 256 комбинациям
    for (int j = 0; j < 8; j++) { // Цикл по 8 светодиодам
      digitalWrite(ledPins[j], (bitRead(i, j) == 1) ? HIGH : LOW); // Включение/выключение светодиода
    }
    delay(delayTime); // Задержка между комбинациями
  }
}
