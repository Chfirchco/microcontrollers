// THIS IS A RECEIVER BOARD

#define PIN_LATCH 9
#define PIN_CLK 12
#define PIN_DATA 11
#define PIN_TUMBLER 13
#define PIN_POT A0

#define PIN_SENDER 9
#define PIN_RECEIVER 2

#define MODE_RECEIVER 0
#define MODE_SENDER 1

int mode = 0;
int timeUnitMs = 0;
String receivedMessage = "";
String resultMessage = "";
String inputString = "";

byte digits[] = {
  0b11101110,  // A
  0b00111110,  // B
  0b10011010,  // C
  0b01111100,  // D
  0b10011110,  // E
  0b10001110,  // F
  0b10111010,  // G
  0b0101110,   // H,
  0b00100000,  // I
  0b01111000,  // J
  0b10101110,  // K
  0b01110000,  // L
  0b10101100,  // M
  0b00101100,  // N
  0b00111100,  // O,
  0b11001110,  // P
  0b11100110,  // Q
  0b00001100,  // R
  0b10110110,  // S
  0b00011110,  // T,
  0b01111010,  // U
  0b111000,    // V
  0b1010110,   // W
  0b01101110,  // X,
  0b01110110,  // Y
  0b11011100,  // Z
  0b11111010,  // 0
  0b01100000,  // 1
  0b11011100,  // 2
  0b11110100,  // 3
  0b01100110,  // 4
  0b10110110,  // 5
  0b10111110,  // 6
  0b11100000,  // 7
  0b11111110,  // 8
  0b11100110,  // 9
  0b00000000,  //Empty Space
};

const char letterMap[] = {
  'A',
  'B',
  'C',
  'D',
  'E',
  'F',
  'G',
  'H',
  'I',
  'J',
  'K',
  'L',
  'M',
  'N',
  'O',
  'P',
  'Q',
  'R',
  'S',
  'T',
  'U',
  'V',
  'W',
  'X',
  'Y',
  'Z',
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  ' '
};
String morseAlphabet[] = {
  ".-",
  "-...",
  "-.-.",
  "-..",
  ".",
  "..-.",
  "--.",
  "....",
  "..",
  ".---",
  "-.-",
  ".-..",
  "--",
  "-.",
  "---",
  ".--.",
  "--.-",
  ".-.",
  "...",
  "-",
  "..-",
  "...-",
  ".--",
  "-..-",
  "-.--",
  "--..",
  "-----",
  ".----",
  "..---",
  "...--",
  "....-",
  ".....",
  "-....",
  "--...",
  "---..",
  "----."
};

void setup() {
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_TUMBLER, INPUT);
  pinMode(PIN_POT, INPUT);
  pinMode(PIN_SENDER, OUTPUT);
  pinMode(PIN_RECEIVER, INPUT);

  Serial.begin(9600);
  if (digitalRead(PIN_TUMBLER) == 1) {
    Serial.println("Controller Mode: RECEIVER");
    mode = MODE_RECEIVER;
  } else {
    mode = MODE_RECEIVER;
    Serial.println("Controller Mode: RECEIVER");
  }
}

void clearDisplay() {
  setDisplay(0b11101110);
}

void loop() {
  if (digitalRead(PIN_TUMBLER) != mode) {
    mode = digitalRead(PIN_TUMBLER);
  }
  timeUnitMs = analogRead(PIN_POT);
  if (mode == MODE_RECEIVER) {
    receiveMorse();
  } else {
    receiveMorse();
    //if (Serial.available()) {
    //  inputString = Serial.readString();
    //  if (inputString.length() > 0) {
    //    transmitMorse(inputString);
    //  }
    //}
  }
}

void setDisplay(byte code) {
  Serial.print("Setting display to code: ");
  Serial.println(code, BIN);  // Print the code value in binary format

  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLK, LSBFIRST, code);
  digitalWrite(PIN_LATCH, HIGH);
}


int findIndex(char ch) {
  for (int i = 0; i < sizeof(letterMap); i++) {
    if (letterMap[i] == ch) {
      return i;
    }
  }
  return -1;
}

void printMessage(String message) {
  static unsigned long lastTime = 0;  
  static int currentIndex = 0;        

  if (millis() - lastTime >= 600) {         
    if (currentIndex < message.length()) {  
      int index = findIndex(message[currentIndex]);
      if (index != -1) {
        Serial.println(digits[index]);
        Serial.println("digits[index]");
        setDisplay(digits[index]);
      }
      currentIndex++;  
    } else {
      currentIndex = 0;
    }
    lastTime = millis(); 
  }
}

void transmitWord(String dotDashSequence) {
  for (int i = 0; i < dotDashSequence.length(); i++) {
    if (dotDashSequence[i] == '.') {
      transmitSignal(1);
    } else if (dotDashSequence[i] == '-') {
      transmitSignal(3);
    } else if (dotDashSequence[i] == ' ') {
      delay(5 * timeUnitMs);
    } else {
      delay(2 * timeUnitMs);
    }
  }
}

void transmitSignal(int duration) {
  digitalWrite(PIN_SENDER, LOW);
  delay(duration * timeUnitMs);
  digitalWrite(PIN_SENDER, HIGH);
  delay(1 * timeUnitMs);
}

bool transmited = false;

void receiveMorse() {

  delay(1);
  int idleCounter = 0;
  while (digitalRead(PIN_RECEIVER) == HIGH) {
    delay(timeUnitMs);
    idleCounter++;
    if (idleCounter > 10) {
      transmited = true;
      break;
    }
  }
  if (transmited == true) {
    printMessage(resultMessage);
    resultMessage = "";
    transmited = false;
  }

  if (idleCounter == 7) {
    receivedMessage += "%";
  } else if (idleCounter == 3) {
    receivedMessage += " ";
  } else if (idleCounter > 7) {
    Serial.println(idleCounter);
    Serial.println("idleCounter");
    resultMessage += decodeMorse(receivedMessage) + " ";
    Serial.println(resultMessage);
    receivedMessage = "";
  }

  int signalDuration = 0;
  while (digitalRead(PIN_RECEIVER) == LOW) {
    delay(timeUnitMs);
    signalDuration++;
  }

  if (signalDuration == 1) {
    receivedMessage += '.';
  } else if (signalDuration == 3) {
    receivedMessage += '-';
  }

  Serial.println(receivedMessage);
  delay(timeUnitMs);
}

String decodeMorse(String morseCode) {
  String decoded = "";
  String currentLetter = "";
  for (int i = 0; i <= morseCode.length(); i++) {
    char currentChar = i < morseCode.length() ? morseCode.charAt(i) : ' ';  // Use a space for the last iteration
    if (currentChar != ' ' && currentChar != '%') {
      currentLetter += currentChar;
    } else {
      char recognizedLetter = '7';
      for (int j = 0; j < 36; j++) {
        if (currentLetter == morseAlphabet[j]) {
          recognizedLetter = letterMap[j];
          break;
        }
      }
      if (currentLetter.length() != 0) {
        decoded += recognizedLetter;
        currentLetter = "";
      }

      if (currentChar == '%') {
        decoded += " ";
        currentLetter = "";
      }
    }
  }

  return decoded;
}

void transmitMorse(String inputString) {
  String prepared = inputString;
  prepared.toUpperCase();
  for (int i = 0; i < prepared.length(); i++) {
    char currentLetter = prepared.charAt(i);
    if (currentLetter == ' ') {
      delay(7 * timeUnitMs);
    } else {
      int letterIndex = 0;
      for (int j = 0; j < 36; j++) {
        if (letterMap[j] == currentLetter) {
          letterIndex = j;
          break;
        }
      }
      transmitWord(morseAlphabet[letterIndex]);
      Serial.print("Transmitting: ");
      Serial.println(morseAlphabet[letterIndex]);
      delay(3 * timeUnitMs);
    }
  }
  delay(8 * timeUnitMs);
}