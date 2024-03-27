#define SPEED_1      5 
#define DIR_1        4 
#define SPEED_2      6 
#define DIR_2        7 
 
void setup() {
  for (int i = 4; i < 8; i++) {     
    pinMode(i, OUTPUT);
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  stop();
  delay(1000);
  move_forward(255);
  delay(1000);
  stop();
  delay(1000);
  rotate_right(255);
  delay(600);
}


void move_forward(int car_speed) {
	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}

void move_back(int car_speed) {
	digitalWrite(DIR_1, LOW);
	digitalWrite(DIR_2, LOW);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}

void stop() {
	analogWrite(SPEED_1, 0);
	analogWrite(SPEED_2, 0);
}

void rotate_right(int car_speed) {
	digitalWrite(DIR_1, LOW);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}

void left_right(int car_speed) {
	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, LOW);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed);
}

void turn_right(int car_speed, float stepness) {

	if (stepness > 1) {
		stepness = 1;
	} else if (stepness < 0) {
		stepness = 0;
	}

	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed * stepness);
	analogWrite(SPEED_2, car_speed);
}

void turn_left(int car_speed, float stepness) {

	if (stepness > 1) {
		stepness = 1;
	} else if (stepness < 0) {
		stepness = 0;
	}


	digitalWrite(DIR_1, HIGH);
	digitalWrite(DIR_2, HIGH);
	analogWrite(SPEED_1, car_speed);
	analogWrite(SPEED_2, car_speed * stepness);
}