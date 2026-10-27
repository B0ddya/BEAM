#include "DHT.h"
#include <Wire.h> // библиотека для управления устройствами по I2C 
#include <LiquidCrystal_I2C.h> // подключаем библиотеку для QAPASS 1602

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C LCD(0x27,16,2); // присваиваем имя LCD для дисплея


void setup() {
  Serial.begin(9600);
   dht.begin();
   LCD.init(); // инициализация LCD дисплея
   LCD.backlight(); // включение подсветки дисплея
   
   LCD.setCursor(0, 0);
   LCD.print("Meteo Station");
   LCD.setCursor(0, 1);
   LCD.print("Starting...");
   delay(2000);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("Sensor Error!");
    delay(2000);
    return;
  }

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Temp: ");
  LCD.print(t);
  LCD.print(" C");
  LCD.setCursor(0, 1);
  LCD.print("Humi: ");
  LCD.print(h);
  LCD.print(" %");
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print(" C");
  Serial.print("     Humi: ");
  Serial.print(h);
  Serial.println(" %");
  delay(2000);
}