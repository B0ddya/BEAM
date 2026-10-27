
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RTC.h>

// Дисплей и часы
LiquidCrystal_I2C lcd(0x27, 16, 2);
iarduino_RTC watch(RTC_DS1302, 10, 8, 9);

// Энкодер
const int encoderCLK = 2;
const int encoderDT = 3;
const int encoderSW = 4;

// Кнопки
const int startButton = 5;
const int modeButton = 6;

// Режимы
const int TIMER_MODE = 0;
const int STOPWATCH_MODE = 1;

int currentMode = TIMER_MODE;

// Состояния
bool running = false;
bool stopped = false;
bool editing = false;

// Цифры времени: ЧЧ:ММ:СС
int digits[6] = {0, 0, 0, 0, 0, 0};
int digitIndex = 0;

// Время
unsigned long presetTime = 0;
unsigned long timerRemaining = 0;
unsigned long stopwatchElapsed = 0;
unsigned long runStartMillis = 0;

// Дисплей
unsigned long lastDisplayUpdate = 0;

// Энкодер: полный цикл сигналов
byte lastEncoderState = 0;
int encoderStep = 0;

// Таблица переходов квадратурного энкодера
const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

// Кнопки
unsigned long lastEncoderPress = 0;
unsigned long lastStartPress = 0;
unsigned long lastModePress = 0;

// --------------------------------------------------
// Число с ведущим нулём
// --------------------------------------------------

void printTwo(unsigned long number) {
  if (number < 10) {
    lcd.print('0');
  }

  lcd.print(number);
}

// --------------------------------------------------
// Вывод ЧЧ:ММ:СС
// --------------------------------------------------

void printTime(unsigned long totalSeconds) {
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  printTwo(hours);
  lcd.print(':');
  printTwo(minutes);
  lcd.print(':');
  printTwo(seconds);
}

// --------------------------------------------------
// Перевод цифр в секунды
// --------------------------------------------------

unsigned long getPresetSeconds() {
  unsigned long hours = digits[0] * 10 + digits[1];
  unsigned long minutes = digits[2] * 10 + digits[3];
  unsigned long seconds = digits[4] * 10 + digits[5];

  return hours * 3600UL + minutes * 60UL + seconds;
}

// --------------------------------------------------
// Заполнение цифр из заданного времени
// --------------------------------------------------

void loadDigits(unsigned long value) {
  digits[0] = (value / 36000UL) % 10;
  digits[1] = (value / 3600UL) % 10;
  digits[2] = (value / 600UL) % 6;
  digits[3] = (value / 60UL) % 10;
  digits[4] = (value / 10UL) % 6;
  digits[5] = value % 10;
}

// --------------------------------------------------
// Изменение выбранной цифры
// --------------------------------------------------

void changeDigit(int direction) {
  if (!editing || currentMode != TIMER_MODE) {
    return;
  }

  int maxDigit = 9;

  if (digitIndex == 2 || digitIndex == 4) {
    maxDigit = 5;
  }

  digits[digitIndex] += direction;

  if (digits[digitIndex] > maxDigit) {
    digits[digitIndex] = 0;
  }

  if (digits[digitIndex] < 0) {
    digits[digitIndex] = maxDigit;
  }
}

// --------------------------------------------------
// Стабильное считывание энкодера
// --------------------------------------------------

void handleEncoder() {
  byte clk = digitalRead(encoderCLK);
  byte dt = digitalRead(encoderDT);

  byte currentState = (clk << 1) | dt;

  if (currentState != lastEncoderState) {
    byte index = (lastEncoderState << 2) | currentState;

    encoderStep += encoderTable[index];

    lastEncoderState = currentState;

    // Изменяем цифру только после полного шага
    if (encoderStep >= 4) {
      changeDigit(1);
      encoderStep = 0;
    }
    else if (encoderStep <= -4) {
      changeDigit(-1);
      encoderStep = 0;
    }
  }
}

// --------------------------------------------------
// Кнопка: одно событие на одно нажатие
// --------------------------------------------------

bool buttonPressed(int pin, unsigned long &lastPress) {
  if (digitalRead(pin) == LOW &&
      millis() - lastPress >= 180) {

    delay(15);

    if (digitalRead(pin) == LOW) {
      lastPress = millis();

      while (digitalRead(pin) == LOW) {
        delay(1);
      }

      return true;
    }
  }

  return false;
}

// --------------------------------------------------
// Кнопка энкодера: настройка и подтверждение цифр
// --------------------------------------------------

void handleEncoderButton() {
  if (currentMode != TIMER_MODE || running || stopped) {
    return;
  }

  // Первое нажатие открывает настройку
  if (!editing) {
    editing = true;
    digitIndex = 0;
    loadDigits(presetTime);
    encoderStep = 0;
    return;
  }

  // Подтверждение выбранной цифры
  digitIndex++;

  if (digitIndex >= 6) {
    editing = false;

    presetTime = getPresetSeconds();
    timerRemaining = presetTime;

    lcd.noCursor();
    lcd.noBlink();
  }
}

// --------------------------------------------------
// Кнопка запуска, остановки и сброса
// --------------------------------------------------

void handleStartButton() {
  if (editing) {
    return;
  }

  // Второе нажатие: остановка
  if (running) {
    unsigned long now = millis();
    unsigned long elapsed = now - runStartMillis;

    if (currentMode == TIMER_MODE) {
      unsigned long elapsedSeconds = elapsed / 1000UL;

      if (elapsedSeconds >= timerRemaining) {
        timerRemaining = 0;
      } else {
        timerRemaining -= elapsedSeconds;
      }
    } else {
      stopwatchElapsed += elapsed;
    }

    running = false;
    stopped = true;
    return;
  }

  // Третье нажатие: сброс
  if (stopped) {
    stopped = false;

    if (currentMode == TIMER_MODE) {
      timerRemaining = presetTime;
    } else {
      stopwatchElapsed = 0;
    }

    return;
  }

  // Первое нажатие: запуск
  if (currentMode == TIMER_MODE) {
    if (timerRemaining == 0) {
      return;
    }
  }

  runStartMillis = millis();
  running = true;
}

// --------------------------------------------------
// Кнопка смены режима
// --------------------------------------------------

void handleModeButton() {
  // Переключение разрешено только после сброса
  if (running || stopped || editing) {
    return;
  }

  if (currentMode == TIMER_MODE) {
    currentMode = STOPWATCH_MODE;
    stopwatchElapsed = 0;
  } else {
    currentMode = TIMER_MODE;
    timerRemaining = presetTime;
  }

  encoderStep = 0;

  lcd.noCursor();
  lcd.noBlink();
}

// --------------------------------------------------
// Обновление дисплея
// --------------------------------------------------


void updateDisplay() {
  if (millis() - lastDisplayUpdate < 100) {
    return;
  }

  lastDisplayUpdate = millis();

  // Меню настройки таймера
  if (editing) {
    lcd.setCursor(0, 0);
    lcd.print("Set timer       ");

    lcd.setCursor(0, 1);

    for (int i = 0; i < 6; i++) {
      lcd.print(digits[i]);

      if (i == 1 || i == 3) {
        lcd.print(':');
      }
    }

    lcd.print("        ");

    int cursorPosition = digitIndex;

    if (digitIndex >= 2) {
      cursorPosition++;
    }

    if (digitIndex >= 4) {
      cursorPosition++;
    }

    lcd.setCursor(cursorPosition, 1);
    lcd.cursor();
    lcd.blink();

    return;
  }

  lcd.noCursor();
  lcd.noBlink();

  // Первая строка — название режима
  lcd.setCursor(0, 0);

  if (currentMode == TIMER_MODE) {
    lcd.print("Timer           ");
  } else {
    lcd.print("Stopwatch       ");
  }

  // Вторая строка — показания
  lcd.setCursor(0, 1);

  unsigned long shownSeconds = 0;

  if (running) {
    unsigned long elapsed = millis() - runStartMillis;

    if (currentMode == TIMER_MODE) {
      unsigned long elapsedSeconds = elapsed / 1000UL;

      if (elapsedSeconds >= timerRemaining) {
        timerRemaining = 0;
        running = false;
        stopped = true;
      } else {
        shownSeconds = timerRemaining - elapsedSeconds;
      }
    } else {
      shownSeconds = (stopwatchElapsed + elapsed) / 1000UL;
    }
  } else {
    if (currentMode == TIMER_MODE) {
      shownSeconds = timerRemaining;
    } else {
      shownSeconds = stopwatchElapsed / 1000UL;
    }
  }

  printTime(shownSeconds);

  if (running) {
    lcd.print("*");
  } else if (stopped) {
    lcd.print("P");
  } else {
    lcd.print(" ");
  }

  lcd.print("       ");
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {
  pinMode(encoderCLK, INPUT_PULLUP);
  pinMode(encoderDT, INPUT_PULLUP);
  pinMode(encoderSW, INPUT_PULLUP);

  pinMode(startButton, INPUT_PULLUP);
  pinMode(modeButton, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  watch.begin();

  lastEncoderState =
    (digitalRead(encoderCLK) << 1) |
    digitalRead(encoderDT);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer");
  lcd.setCursor(0, 1);
  lcd.print("Stopwatch");

  delay(1200);
  lcd.clear();
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {
  handleEncoder();

  if (buttonPressed(encoderSW, lastEncoderPress)) {
    handleEncoderButton();
  }

  if (buttonPressed(startButton, lastStartPress)) {
    handleStartButton();
  }

  if (buttonPressed(modeButton, lastModePress)) {
    handleModeButton();
  }

  updateDisplay();
}
