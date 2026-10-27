#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// Дисплей LCD 1602 I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Подключение фоторезистора
const int sensorPin = A0;

// Сопротивление постоянного резистора в делителе
const float fixedResistor = 10000.0;

// Параметры расчета освещенности
// Начальные значения, которые можно откалибровать
const float luxCoefficient = 500.0;
const float luxExponent = 1.4;

// Сглаживание показаний
float filteredResistance = 0;
float filteredLux = 0;

bool firstMeasurement = true;

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  lcd.setCursor(0, 0);
  lcd.print("Lux meter");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(1500);

  lcd.clear();
}

void loop() {
  // Считываем несколько измерений
  long sensorSum = 0;

  for (int i = 0; i < 20; i++) {
    sensorSum += analogRead(sensorPin);
    delay(2);
  }

  float sensorValue = sensorSum / 20.0;

  // Защита от деления на ноль
  if (sensorValue < 1) {
    sensorValue = 1;
  }

  if (sensorValue > 1022) {
    sensorValue = 1022;
  }

  // Рассчитываем сопротивление фоторезистора
  // Фоторезистор подключен к 5V,
  // постоянный резистор 10 кОм — к GND
  float resistance = fixedResistor *
                    (1023.0 / sensorValue - 1.0);

  // Переводим сопротивление в килоомы
  float resistanceKOhm = resistance / 1000.0;

  // Примерная оценка освещенности
  // Используется нелинейная степенная зависимость
  float lux = luxCoefficient *
              pow(10.0 / resistanceKOhm, luxExponent);

  // Ограничиваем диапазон отображения
  if (lux > 99999) {
    lux = 99999;
  }

  if (lux < 0) {
    lux = 0;
  }

  // Сглаживаем показания
  if (firstMeasurement) {
    filteredResistance = resistanceKOhm;
    filteredLux = lux;
    firstMeasurement = false;
  } else {
    filteredResistance =
        filteredResistance * 0.75 +
        resistanceKOhm * 0.25;

    filteredLux =
        filteredLux * 0.75 +
        lux * 0.25;
  }

  // Вывод сопротивления
  lcd.setCursor(0, 0);
  lcd.print("R:");

  if (filteredResistance < 100) {
    lcd.print(filteredResistance, 1);
  } else {
    lcd.print(filteredResistance, 0);
  }

  lcd.print(" kOhm      ");

  // Вывод освещенности
  lcd.setCursor(0, 1);
  lcd.print("Lux:");

  lcd.print((long)filteredLux);
  lcd.print(" lx       ");
  // Дублирование показаний в монитор порта
  Serial.print("ADC: ");
  Serial.print(sensorValue, 0);

  Serial.print(" | R: ");
  Serial.print(filteredResistance, 1);
  Serial.print(" kOhm");

  Serial.print(" | Lux: ");
  Serial.print((long)filteredLux);
  Serial.println(" lx");
  delay(500);
}