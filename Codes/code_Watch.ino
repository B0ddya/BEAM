#include <iarduino_RTC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// DS1302: RST, CLK, DAT
iarduino_RTC watch(RTC_DS1302, 10, 8, 9);

// LCD 1602 I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Энкодер KY-040
#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  4

// Текущий режим:
// -1 = обычные часы
//  0 = часы
//  1 = минуты
//  2 = день
//  3 = месяц
//  4 = год
int setting = -1;

// Значения для настройки
int hoursValue;
int minutesValue;
int dayValue;
int monthValue;
int yearValue;

// Переменные энкодера
int lastCLK;
int lastButtonState = HIGH;
int buttonState = HIGH;

unsigned long lastButtonChange = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lastCLK = digitalRead(ENCODER_CLK);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RTC Clock");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();
}

void loop() {
  readEncoder();
  readButton();

  if (setting == -1) {
    showClock();
  } else {
    showSettings();
  }
}

// Получение и отображение времени
void showClock() {
  if (millis() - lastDisplayUpdate >= 200) {
    lastDisplayUpdate = millis();

    lcd.setCursor(0, 0);
    lcd.print(watch.gettime("H:i:s"));
    lcd.print("         ");

    lcd.setCursor(0, 1);
    lcd.print(watch.gettime("d-m-Y"));
    lcd.print("       ");
  }

  if (millis() - lastSerialUpdate >= 1000) {
    lastSerialUpdate = millis();

    Serial.println(watch.gettime("d-m-Y, H:i:s, D"));
  }
}

// Чтение поворота энкодера
void readEncoder() {
  static int previousState = 3;
  static int positionChange = 0;

  int clk = digitalRead(ENCODER_CLK);
  int dt = digitalRead(ENCODER_DT);

  int currentState = (clk << 1) | dt;

  if (currentState == previousState) {
    return;
  }

  // Таблица переходов квадратурного энкодера.
  // Правильные последовательности накапливают +4 или -4.
  int transition = 0;

  if (previousState == 0 && currentState == 1) transition = 1;
  if (previousState == 1 && currentState == 3) transition = 1;
  if (previousState == 3 && currentState == 2) transition = 1;
  if (previousState == 2 && currentState == 0) transition = 1;

  if (previousState == 1 && currentState == 0) transition = -1;
  if (previousState == 3 && currentState == 1) transition = -1;
  if (previousState == 2 && currentState == 3) transition = -1;
  if (previousState == 0 && currentState == 2) transition = -1;

  // Если переход некорректный, сбрасываем накопление.
  if (transition == 0) {
    positionChange = 0;
  } else {
    positionChange += transition;
  }

  previousState = currentState;

  if (positionChange >= 4) {
    if (setting >= 0) {
      changeValue(1);
    }

    positionChange = 0;
  }

  if (positionChange <= -4) {
    if (setting >= 0) {
      changeValue(-1);
    }

    positionChange = 0;
  }
}
// Чтение кнопки с защитой от дребезга
void readButton() {
  int reading = digitalRead(ENCODER_SW);

  if (reading != lastButtonState) {
    lastButtonChange = millis();
  }

  if (millis() - lastButtonChange >= 40) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        buttonPressed();
      }
    }
  }

  lastButtonState = reading;
}

// Обработка нажатия кнопки
void buttonPressed() {
  if (setting == -1) {
    // Входим в настройку
    loadTime();

    setting = 0;
    lcd.clear();
  } else {
    // Переходим к следующему параметру
    setting++;

    if (setting > 4) {
      // Все значения настроены
      saveTime();

      setting = -1;
      lcd.clear();

      Serial.println("Time saved!");
      Serial.println(watch.gettime("d-m-Y, H:i:s, D"));
    } else {
      lcd.clear();
    }
  }
}

// Загрузка текущих значений из DS1302
void loadTime() {
  watch.gettime();

  hoursValue = watch.Hours;
  minutesValue = watch.minutes;
  dayValue = watch.day;
  monthValue = watch.month;
  yearValue = watch.year + 2000;
}

// Изменение выбранного значения
void changeValue(int direction) {
  if (setting == 0) {
    hoursValue += direction;

    if (hoursValue > 23) hoursValue = 0;
    if (hoursValue < 0) hoursValue = 23;
  }

  if (setting == 1) {
    minutesValue += direction;

    if (minutesValue > 59) minutesValue = 0;
    if (minutesValue < 0) minutesValue = 59;
  }

  if (setting == 2) {
    dayValue += direction;

    if (dayValue > daysInMonth(monthValue, yearValue)) {
      dayValue = 1;
    }

    if (dayValue < 1) {
      dayValue = daysInMonth(monthValue, yearValue);
    }
  }

  if (setting == 3) {
    monthValue += direction;

    if (monthValue > 12) monthValue = 1;
    if (monthValue < 1) monthValue = 12;

    // Проверяем, существует ли выбранный день в новом месяце
    if (dayValue > daysInMonth(monthValue, yearValue)) {
      dayValue = daysInMonth(monthValue, yearValue);
    }
  }

  if (setting == 4) {
    yearValue += direction;

    if (yearValue > 2099) yearValue = 2000;
    if (yearValue < 2000) yearValue = 2099;

    // Проверяем високосный год
    if (dayValue > daysInMonth(monthValue, yearValue)) {
      dayValue = daysInMonth(monthValue, yearValue);
    }
  }
}

// Количество дней в месяце
int daysInMonth(int month, int year) {
  if (month == 2) {
    if (year % 400 == 0 ||
        (year % 4 == 0 && year % 100 != 0)) {
      return 29;
    }

    return 28;
  }

  if (month == 4 || month == 6 ||
      month == 9 || month == 11) {
    return 30;
  }

  return 31;
}

// Отображение меню настройки
void showSettings() {
  if (millis() - lastDisplayUpdate < 150) {
    return;
  }

  lastDisplayUpdate = millis();

  lcd.setCursor(0, 0);

  if (setting == 0) lcd.print("Set hours       ");
  if (setting == 1) lcd.print("Set minutes     ");
  if (setting == 2) lcd.print("Set day         ");
  if (setting == 3) lcd.print("Set month       ");
  if (setting == 4) lcd.print("Set year        ");

  lcd.setCursor(0, 1);

  if (setting == 0) {
    printTwoDigits(hoursValue);
    lcd.print("               ");
  }

  if (setting == 1) {
    printTwoDigits(minutesValue);
    lcd.print("               ");
  }

  if (setting == 2) {
    printTwoDigits(dayValue);
    lcd.print("               ");
  }

  if (setting == 3) {
    printTwoDigits(monthValue);
    lcd.print("               ");
  }

  if (setting == 4) {
    lcd.print(yearValue);
    lcd.print("            ");
  }
}

// Вывод числа двумя цифрами
void printTwoDigits(int number) {
  if (number < 10) {
    lcd.print("0");
  }

  lcd.print(number);
}

// Запись нового времени в DS1302
void saveTime() {
  int weekday = calculateWeekday(
    yearValue, monthValue, dayValue
  );

  watch.settime(
    0,
    minutesValue,
    hoursValue,
    dayValue,
    monthValue,
    yearValue - 2000,
    weekday
  );
}

// Расчёт дня недели:
// 0 = воскресенье, 1 = понедельник, ..., 6 = суббота
int calculateWeekday(int year, int month, int day) {
  int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

  if (month < 3) {
    year--;
  }

  int weekday = (
    year + year / 4 - year / 100 + year / 400
    + offsets[month - 1] + day
  ) % 7;

  return weekday;
}