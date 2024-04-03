//Определение значений скорости и направления
#define SPEED_1      5 
#define DIR_1        4 
#define SPEED_2      6 
#define DIR_2        7 

//Повороты и границы датчиков
int counter_turns = 0; // текущее положение
bool was_wall_left = false; // наличие стены слева
int target_angle_to_turn = 90; // угол поворота 
int const CAR_SPEED = 255; // скорость робота (максимальное значение 255)
int left_sensor[] = {10,11}; // левый датчик расстояния
int forward_sensor[] = {12, 13}; // передний датчик расстояния
float left_sensor_value; // текущее значение левого датчика
float forward_sensor_value; // текущее значение переднего датчика
int const MAX_LEFT_SENSOR_VALUE = 20; // максимальное значение левого датчика
int const MAX_FORWARD_SENSOR_VALUE = 20; // максимальное значение переднего датчика
int const MIN_LEFT_SENSOR_VALUE = 7; // минимальное значение левого датчика
int const MIN_FORWARD_SENSOR_VALUE = 7; // минимальное значение переднего датчика

//Проверка на наличие стены
bool isLeftWall = false;
bool isForwardWall = false;
bool isLeftWallTooClose = false;
bool isForwardWallTooClose = false;

int const MAX_FORWARD_VALUE_TO_END_PROGRAM = 100; // максимальное значение для завершения программы
bool isContinues = true; // продолжать ли движение

bool isInversed = false; // переменная обратного движения

//Состояния машины
String states[8] = {"STOP", "MOVE_FORWARD", "ROTATE_RIGHT", "MOVE_BACK", "ROTATE_LEFT", "TURN_RIGHT", "TURN_LEFT", "CHANGE_STATE"};
int currentState = 0; // текущее состояние
int intervals[8] = {1000, 600, 200, 1500, 500, 300, 300, 0}; // временные интервалы для состояний
unsigned long previousMillisSequence = 0; 
unsigned long previousMillisThreshold = 0; 
int intervalRotateToMoveForward = 50;

//Функция для измерения расстояния
int measureDistance(int a[]) {
    // настройка входов/выходов
    pinMode(a[1], OUTPUT);
    digitalWrite(a[1], LOW);
    delayMicroseconds(2);
    digitalWrite(a[1], HIGH);
    delayMicroseconds(10);
    digitalWrite(a[1], LOW);
    pinMode(a[0], INPUT);
    
    // измерение времени поступления сигнала
    long duration = pulseIn(a[0], HIGH, 100000);
    
    // возвращаем расстояние в см
    return duration / 29 / 2;
}


//Функция поворота вправо
void rotate_right(int car_speed) {
	digitalWrite(DIR_1, LOW); // установка направления
	digitalWrite(DIR_2, LOW); 
	analogWrite(SPEED_1, car_speed); // установка скорости
	analogWrite(SPEED_2, car_speed);
}

//Функция поворота влево
void rotate_left(int car_speed) {
	digitalWrite(DIR_1, HIGH); // установка направления
	digitalWrite(DIR_2, HIGH); 
	analogWrite(SPEED_1, car_speed); // установка скорости
	analogWrite(SPEED_2, car_speed);
}


void setup() {
  Serial.begin(9600); // начало передачи данных
  for (int i = 4; i < 8; i++) { // настройка входов/выходов 
    pinMode(i, OUTPUT);
  }
  stop(); // вызов функции остановки робота
  delay(1000); // задержка перед началом движения
  currentState = 7; // начальное состояние - смена состояния
}


void loop() {
  if (isContinues) { 
    // измерение расстояния
    left_sensor_value = measureDistance(left_sensor); 
    forward_sensor_value = measureDistance(forward_sensor);

    // проверка на наличие стен
    isLeftWall = left_sensor_value < MAX_LEFT_SENSOR_VALUE;
    isForwardWall = forward_sensor_value < MAX_FORWARD_SENSOR_VALUE;
    isLeftWallTooClose = left_sensor_value > 0 && left_sensor_value < MIN_LEFT_SENSOR_VALUE;
    isForwardWallTooClose = forward_sensor_value > 0 && forward_sensor_value < MIN_FORWARD_SENSOR_VALUE;

     // вывод значений в Serial Monitor
    Serial.print("ЛЕВЫЙ: ");
    Serial.println(left_sensor_value);
    Serial.print("ВПЕРЕД: ");
    Serial.println(forward_sensor_value);
    delay(30);

    unsigned long currentMillisSequence = millis();
     //Проверка текущего состояния машины и переключение в случае необходимости
    if (states[currentState] == "MOVE_FORWARD") {
      if (currentMillisSequence - previousMillisSequence >= intervals[currentState]) {
        previousMillisSequence = currentMillisSequence;
        currentState = 7; // CHANGE_STATE
      }
      else {
        move_forward(CAR_SPEED);
        unsigned long currentMillisThresholdSequence = millis();
        if (currentMillisThresholdSequence - previousMillisThreshold >= intervalRotateToMoveForward) {
          previousMillisThreshold = currentMillisThresholdSequence;
          if (isLeftWallTooClose) {
            rotate_right(CAR_SPEED);
            delay(1);
          }
        }
      }
    }
    else if (states[currentState] == "ROTATE_RIGHT")  {
       counter_turns += 1;
       if (currentMillisSequence - previousMillisSequence >= intervals[currentState]) {
        previousMillisSequence = currentMillisSequence;
        currentState = 7; // CHANGE_STATE
      }
      else {
        rotate_right(CAR_SPEED);
      }
    }
    else if (states[currentState] == "ROTATE_LEFT")  {
       if (currentMillisSequence - previousMillisSequence >= intervals[currentState]) {
        previousMillisSequence = currentMillisSequence;
          currentState = 7; // CHANGE_STATE
      }
      else {
        rotate_left(CAR_SPEED);
      }
    }
  //Обновление текущего состояния на основе окружения
    if (currentState == 7) {
      if (isLeftWall) {
        was_wall_left = true;
        if (isForwardWall) {
          currentState = 2; // ROTATE_RIGHT
        }
        else {
          currentState = 1; // MOVE_FORWARD
        }
      }
      else if (was_wall_left) {
        was_wall_left = false;
        currentState = 4; // ROTATE_LEFT
      }
      else if (isForwardWall) {
        currentState = 2; // ROTATE_RIGHT
      }
      else {
        currentState = 1; // MOVE_FORWARD
      }
    }
  }
}


//Функция движения вперед
void move_forward(int car_speed) {
	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, LOW);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}


//Функция движения назад
void move_back(int car_speed) {
	digitalWrite(DIR_1, LOW);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}

//Функция остановки движения
void stop() {
	analogWrite(SPEED_1, 0);
	analogWrite(SPEED_2, 0);
}
