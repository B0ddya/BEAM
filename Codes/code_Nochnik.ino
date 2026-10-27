
const int photoPin = A0;
const int ledPin = 12;
const int darkThreshold = 350;
const int lightThreshold = 450;
bool ledState = false;

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600);
}

void loop() {
  int lightValue;
  if (ledState == true) {
    digitalWrite(ledPin, LOW);
    delay(2);
  }

  lightValue = analogRead(photoPin);

  if (ledState == false && lightValue < darkThreshold) {
    ledState = true;
  }
  else if (ledState == true && lightValue > lightThreshold) {
    ledState = false;
  }

  if(ledState == false){
  digitalWrite(ledPin, LOW);
  }
  else if(ledState == true){
  digitalWrite(ledPin, HIGH);
  }

  Serial.print("Освещенность: ");
  Serial.print(lightValue);
  Serial.print(" | Светодиод: ");
  if(ledState == false){
  Serial.println("ВЫКЛ");
  }
  else if(ledState == true){
  Serial.println("ВКЛ");
  }
  delay(200);
}
