void setup() {
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(9600);
}

void turn_on_1() {
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
}

void turn_on_2() {
  digitalWrite(13, HIGH);
  digitalWrite(12, LOW);
}

void turn_off_12() {
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
}

bool state = false;

void loop() {
    if (Serial.available() > 1) {
      Serial.println(Serial.readString());
    }

    if (digitalRead(2) == LOW) {
      if (!state) {
        state = true;
        Serial.println("pressed");
        delay(100);
      }
    } else {
      if (state) {
        state = false;
        Serial.println("released"); 
      }
    }
}
