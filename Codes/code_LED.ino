void setup() {
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(3, HIGH);
  digitalWrite(5, HIGH);
}

void loop() {
  digitalWrite(9, HIGH);
  digitalWrite(12, HIGH);
  delay(1000);
  digitalWrite(9, LOW);
  digitalWrite(12, LOW);
  delay(500);
}
