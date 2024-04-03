#define SPEED_1      5 
#define DIR_1        4 
#define SPEED_2      6 
#define DIR_2        7 
 
int counter_turns = 0; // stay at one place
bool was_wall_left = false;
int target_angle_to_turn = 90; // delay in 700 ms == 90 degrees 
int const CAR_SPEED = 255; // 0 to 255
int left_sensor[] = {10,11};
int forward_sensor[] = {12, 13};
float left_sensor_value;
float forward_sensor_value;
int const MAX_LEFT_SENSOR_VALUE = 20;
int const MAX_FORWARD_SENSOR_VALUE = 20;
int const MIN_LEFT_SENSOR_VALUE = 7;
int const MIN_FORWARD_SENSOR_VALUE = 7;

bool isLeftWall = false;
bool isForwardWall = false;
bool isLeftWallTooClose = false;
bool isForwardWallTooClose = false;

int const MAX_FORWARD_VALUE_TO_END_PROGRAM = 100;
bool isContinues = true;

bool isInversed = false;

String states[8] = {"STOP", "MOVE_FORWARD", "ROTATE_RIGHT", "MOVE_BACK", "ROTATE_LEFT", "TURN_RIGHT", "TURN_LEFT", "CHANGE_STATE"};
int currentState = 0;
int intervals[8] = {1000, 600, 200, 1500, 500, 300, 300, 0};
unsigned long previousMillisSequence = 0;
unsigned long previousMillisThreshold = 0;
int intervalRotateToMoveForward = 50;

int measureDistance(int a[]) {
    pinMode(a[1], OUTPUT);
    digitalWrite(a[1], LOW);
    delayMicroseconds(2);
    digitalWrite(a[1], HIGH);
    delayMicroseconds(10);
    digitalWrite(a[1], LOW);
    pinMode(a[0], INPUT);
    long duration = pulseIn(a[0], HIGH, 100000);
    return duration / 29 / 2;
}


void rotate_right(int car_speed) {
	digitalWrite(DIR_1, LOW);
	digitalWrite(DIR_2, LOW);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}


void rotate_left(int car_speed) {
	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}


void setup() {
  Serial.begin(9600);
  for (int i = 4; i < 8; i++) {     
    pinMode(i, OUTPUT);
  }
  stop();
  delay(1000);
  currentState = 7; // CHANGE_STATE
}


// void loop() { // test directions and speed
//   rotate_right(CAR_SPEED);
//   Serial.println("RIGHT");
// }


void loop() {
  if (isContinues) {
    left_sensor_value = measureDistance(left_sensor);
    forward_sensor_value = measureDistance(forward_sensor);
    isLeftWall = left_sensor_value < MAX_LEFT_SENSOR_VALUE;
    isForwardWall = forward_sensor_value < MAX_FORWARD_SENSOR_VALUE;
    isLeftWallTooClose = 0 < left_sensor_value && left_sensor_value < MIN_LEFT_SENSOR_VALUE;
    isForwardWallTooClose = 0 < forward_sensor_value && forward_sensor_value < MIN_FORWARD_SENSOR_VALUE;

    Serial.print("LEFT: ");
    Serial.println(left_sensor_value);
    Serial.print("FORWARD: ");
    Serial.println(forward_sensor_value);
    delay(30);

    // if (counter_turns == (360/target_angle_to_turn)) {
    //   if (forward_sensor_value > MAX_FORWARD_VALUE_TO_END_PROGRAM) {
    //     isContinues = false;
    //     Serial.print("finish");
    //   }
    //   move_forward(CAR_SPEED);
    //   counter_turns = 0;
    //   delay(500);
    // }
    unsigned long currentMillisSequence = millis();
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
        // if (isForwardWall == false) {
        //   currentState = 1; // MOVE_FORWARD
        // }
        // else {
          currentState = 7; // CHANGE_STATE
        // }
      }
      else {
        rotate_left(CAR_SPEED);
      }
    }

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


void move_forward(int car_speed) {
	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, LOW);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}


void move_back(int car_speed) {
	digitalWrite(DIR_1, LOW);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}


void stop() {
	analogWrite(SPEED_1, 0);
	analogWrite(SPEED_2, 0);
}


// void turn_right(int car_speed, float stepness) {

// 	if (stepness > 1) {
// 		stepness = 1;
// 	} else if (stepness < 0) {
// 		stepness = 0;
// 	}

// 	digitalWrite(DIR_1, HIGH);
// 	digitalWrite(DIR_2, HIGH);
// 	analogWrite(SPEED_1, car_speed * stepness);
// 	analogWrite(SPEED_2, car_speed);
// }


// void turn_left(int car_speed, float stepness) {

// 	if (stepness > 1) {
// 		stepness = 1;
// 	} else if (stepness < 0) {
// 		stepness = 0;
// 	}

// 	digitalWrite(DIR_1, HIGH);
// 	digitalWrite(DIR_2, HIGH);
// 	analogWrite(SPEED_1, car_speed);
// 	analogWrite(SPEED_2, car_speed * stepness);
// }