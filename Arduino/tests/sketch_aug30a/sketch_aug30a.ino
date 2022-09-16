

void setup() {
  Serial.begin(9600);
}



void loop() {
  Serial.println("Przeczytano:");
  if (Serial.available() > 1) {
    Serial.println(Serial.readString());
  }
  delay(1000);
}
