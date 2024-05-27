# Attendance System / Система учета посещений

## Описание проекта

Этот проект представляет собой систему учета посещаемости, основанную на плате ESP8266, использующую датчики расстояния, светодиоды и зуммер для отслеживания и управления количеством людей в помещении. Данные о посещаемости публикуются через MQTT, а также сохраняются в EEPROM и LittleFS. Управление устройством осуществляется с помощью бесплатной программы IoT MQTT Panel, доступной на iOS и Android. Для получения опциональных уведомлений с устройства используется телеграмм-бот. Корпус для устройства печатается на 3D принтере.

## Компоненты проекта

### Основные файлы

- `project_iot.ino` - Основной файл прошивки, содержащий логический цикл программы.
- `mqtt_connect.h` - Заголовочный файл для настройки и обработки MQTT соединения.
- `wifi_connect.h` - Заголовочный файл для подключения к Wi-Fi.
- `readme.md` - Файл с описанием проекта.
- `user_manual_attendance_system.pdf` - Инструкция пользователя.

### Директории

- `device_housing` - Директория с файлами для корпуса устройства.
- `tg_bot` - Директория с файлами для запуска телеграм-бота на собственном устройстве.

## Аппаратные компоненты

- Беспроводной модуль Wi-Fi NodeMCU V3 на базе ESP8266 ардуино
- Ультразвуковы датчики расстояния HC-SR04 5V
- Светодиоды
- Зуммер пьезоэлектрический (активный) TMB12A03 3V
- Дисплей TM1637 0.36”

## Установка и настройка

Установка, настройка, то, как работает устройство, доступно в [Инструкции пользователя](user_manual_attendance_system.pdf).

## Закрывающая информация

Проект выполнен тремя студентами ФБКИ ИГУ.