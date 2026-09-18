#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_SCLK D8
#define TFT_MOSI D10
#define TFT_DC D6
#define TFT_CS D9
#define TFT_RST -1

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

const uint8_t buttonPins[6] = {D0, D1, D2, D3, D4, D5};

uint8_t lastReading[6];
uint8_t stableState[6];
uint32_t lastDebounceTime[6] = {0};

uint8_t hours = 12;
uint8_t minutes = 0;
uint8_t seconds = 0;
uint32_t lastSecondTick = 0;

enum DisplayMode {
  CLOCK_MODE,
  STOPWATCH_MODE
};

DisplayMode mode = CLOCK_MODE;

bool stopwatchRunning = false;
uint32_t stopwatchStartedAt = 0;
uint32_t stopwatchSavedMs = 0;

uint32_t lastDisplayUpdate = 0;
bool screenNeedsRedraw = true;

void printTwoDigits(uint32_t value) {
  if (value < 10) {
    tft.print('0');
  }
  tft.print(value);
}

void drawClockTime() {
  tft.fillRect(0, 65, tft.width(), 65, ST77XX_BLACK);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(4);
  tft.setCursor(18, 78);
  printTwoDigits(hours);
  tft.print(':');
  printTwoDigits(minutes);
  tft.print(':');
  printTwoDigits(seconds);
}

uint32_t currentStopwatchMs() {
  if (stopwatchRunning) {
    return stopwatchSavedMs + (millis() - stopwatchStartedAt);
  }
  return stopwatchSavedMs;
}

void drawStopwatchTime() {
  uint32_t totalSeconds = currentStopwatchMs() / 1000;
  uint32_t stopwatchHours = totalSeconds / 3600;
  uint32_t stopwatchMinutes = (totalSeconds / 60) % 60;
  uint32_t stopwatchSeconds = totalSeconds % 60;

  tft.fillRect(0, 65, tft.width(), 65, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.setCursor(18, 78);
  printTwoDigits(stopwatchHours);
  tft.print(':');
  printTwoDigits(stopwatchMinutes);
  tft.print(':');
  printTwoDigits(stopwatchSeconds);

  tft.fillRect(0, 140, tft.width(), 25, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(18, 145);

  if (stopwatchRunning) {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("RUNNING");
  } else {
    tft.setTextColor(ST77XX_WHITE);
    tft.print("STOPPED");
  }
}

void drawScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(18, 20);

  if (mode == CLOCK_MODE) {
    tft.setTextColor(ST77XX_WHITE);
    tft.print("MANUAL CLOCK");
    drawClockTime();
  } else {
    tft.setTextColor(ST77XX_WHITE);
    tft.print("STOPWATCH");
    drawStopwatchTime();
  }
}

void increaseHour() {
  hours++;
  if (hours >= 24) {
    hours = 0;
  }
  seconds = 0;
  lastSecondTick = millis();
  screenNeedsRedraw = true;
}

void increaseMinute() {
  minutes++;
  if (minutes >= 60) {
    minutes = 0;
    hours++;
    if (hours >= 24) {
      hours = 0;
    }
  }
  seconds = 0;
  lastSecondTick = millis();
  screenNeedsRedraw = true;
}

void startStopwatch() {
  if (!stopwatchRunning) {
    stopwatchStartedAt = millis();
    stopwatchRunning = true;
    screenNeedsRedraw = true;
  }
}

void stopStopwatch() {
  if (stopwatchRunning) {
    stopwatchSavedMs += millis() - stopwatchStartedAt;
    stopwatchRunning = false;
    screenNeedsRedraw = true;
  }
}

void resetStopwatch() {
  stopwatchRunning = false;
  stopwatchSavedMs = 0;
  screenNeedsRedraw = true;
}

void onButtonPressed(uint8_t buttonNumber) {
  switch (buttonNumber) {
    case 0:
      if (mode == CLOCK_MODE) {
        increaseHour();
      }
      break;

    case 1:
      if (mode == CLOCK_MODE) {
        increaseMinute();
      }
      break;

    case 2:
      mode = mode == CLOCK_MODE ? STOPWATCH_MODE : CLOCK_MODE;
      screenNeedsRedraw = true;
      break;

    case 3:
      if (mode == STOPWATCH_MODE) {
        startStopwatch();
      }
      break;

    case 4:
      if (mode == STOPWATCH_MODE) {
        stopStopwatch();
      }
      break;

    case 5:
      if (mode == STOPWATCH_MODE) {
        resetStopwatch();
      }
      break;
  }
}

void pollButtons() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t reading = digitalRead(buttonPins[i]);

    if (reading != lastReading[i]) {
      lastReading[i] = reading;
      lastDebounceTime[i] = now;
    }

    if ((now - lastDebounceTime[i]) > 30 && reading != stableState[i]) {
      stableState[i] = reading;

      if (stableState[i] == LOW) {
        onButtonPressed(i);
      }
    }
  }
}

void updateClock() {
  uint32_t now = millis();

  while (now - lastSecondTick >= 1000) {
    lastSecondTick += 1000;
    seconds++;

    if (seconds >= 60) {
      seconds = 0;
      minutes++;

      if (minutes >= 60) {
        minutes = 0;
        hours++;

        if (hours >= 24) {
          hours = 0;
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    stableState[i] = digitalRead(buttonPins[i]);
    lastReading[i] = stableState[i];
  }

  tft.init(76, 284);
  tft.setOffsets(82, 18);
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);

  lastSecondTick = millis();
  drawScreen();
  screenNeedsRedraw = false;
  lastDisplayUpdate = millis();

  Serial.println("TFT Initialised");
}

void loop() {
  pollButtons();
  updateClock();

  uint32_t now = millis();

  if (screenNeedsRedraw) {
    drawScreen();
    screenNeedsRedraw = false;
    lastDisplayUpdate = now;
  } else {
    uint32_t refreshInterval = 1000;

    if (mode == STOPWATCH_MODE && stopwatchRunning) {
      refreshInterval = 100;
    }

    if (now - lastDisplayUpdate >= refreshInterval) {
      lastDisplayUpdate = now;

      if (mode == CLOCK_MODE) {
        drawClockTime();
      } else {
        drawStopwatchTime();
      }
    }
  }
}
