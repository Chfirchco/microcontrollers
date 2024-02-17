const int IN0 = 3;
const int IN1 = 4;

void setup() {
  pinMode(IN0, OUTPUT);
  pinMode(IN1, OUTPUT);
}

void loop() {
  rotateClockwise();
  delay(2000);
  rotateCounterclockwise();
  delay(2000);
}

void rotateClockwise() {
  digitalWrite(IN0, HIGH);
  digitalWrite(IN1, LOW);
}

void rotateCounterclockwise() {
  digitalWrite(IN0, LOW);
  digitalWrite(IN1, HIGH);
}
