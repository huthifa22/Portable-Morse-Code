#include <WiFiNINA.h>
#include "thingProperties.h"
#include "MorseUtils.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"
#include <vector>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// TFT Display SPI Pins
#define TFT_CLK 13
#define TFT_MOSI 11
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST -1

// Touchscreen Pins
#define YP A1
#define XM A0
#define YM 7
#define XP 6

// LED, Buzzer, and Button Pins
#define LED_PIN 2
#define BUZZER_PIN 4
#define BUTTON_PIN 5

// Touchscreen Calibration
#define MINPRESSURE 2
#define MAXPRESSURE 1000

// Color Definitions
#define ILI9341_GRAY 0x8410

// Calibration values
float xCalM = 0.52;
float xCalC = -115.05;
float yCalM = 0.49;
float yCalC = -158.64;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Keyboard Settings
const int keysPerRow = 10;
const int keyWidth = 25;
const int keyHeight = 25;
const int keySpacing = 5;
const int xMargin = 0;
int dynamicKeyWidth = 8;

char wifiSSID[32] = "";
char wifiPassword[32] = "";
String textMessage = "";
std::vector<String> messageQueue;
int currentMessageIndex = -1;
static bool previousWiFiStatus = false;

// User Customizable Variables
int WPM = 15;
long buzzerFrequency = 700;
long buttonPressTimingThreshold = 500;
long letterTerminationDelay = 1500;
int randomWordMinLength = 2;
int randomWordMaxLength = 10;

enum AppState {
  STATE_STARTUP,
  STATE_WIFI_SETUP,
  STATE_WIFI_CREDENTIALS,
  STATE_CONNECTION,
  STATE_MAIN_MENU,
  STATE_MAIL,
  STATE_GUIDE,
  STATE_ENCODE,
  STATE_DECODE,
  STATE_GAME,
  STATE_TOOLS,
  STATE_OTHER
};

enum KeyboardState {
  STATE_UPPERCASE,
  STATE_LOWERCASE,
  STATE_SYMBOLS
};

bool screenDrawn = false;
AppState currentState = STATE_STARTUP;
KeyboardState currentKeyboardState = STATE_UPPERCASE;

void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  switch (currentState) {
    case STATE_STARTUP:
      runStartupScreen();
      break;

    case STATE_WIFI_SETUP:
      runWiFiSetupScreen();
      break;

    case STATE_WIFI_CREDENTIALS:
      runWiFiCredentialsScreen();
      break;

    case STATE_CONNECTION:
      runConnectionState();
      break;

    case STATE_MAIN_MENU:
      runMainMenuScreen();
      break;

    case STATE_MAIL:
      runMailScreen();
      break;

    case STATE_GUIDE:
      guideScreen();
      break;

    case STATE_ENCODE:
      currentKeyboardState = STATE_UPPERCASE;
      runEncodeScreen();
      break;

    case STATE_DECODE:
      runDecodeScreen();
      break;

    case STATE_GAME:
      runGameScreen();
      break;

    case STATE_TOOLS:
      runDummyState("Tools");
      break;

    case STATE_OTHER:
      runDummyState("Other");
      break;
  }

  // Checking for Cloud Messages if the Wi-Fi and Cloud are connected
  if (WiFi.status() == WL_CONNECTED && ArduinoCloud.connected()) {
    ArduinoCloud.update();
  }
}

void runStartupScreen() {
  if (!screenDrawn) {
    resetState();

    tft.fillScreen(ILI9341_BLACK);
    tft.setFont(&FreeSansBoldOblique18pt7b);
    tft.setTextColor(tft.color565(0, 255, 0));

    const char* text = "PMC";
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int16_t centerX = (tft.width() - w) / 2;
    int16_t centerY = (tft.height() - h) / 2;

    for (int radius = 8; radius >= 0; radius -= 2) {
      tft.setTextColor(tft.color565(0, (radius * 255) / 8, 0));
      tft.setCursor(centerX - radius, centerY - radius);
      tft.print(text);
      delay(110);
    }

    tft.setTextColor(tft.color565(0, 255, 0));
    tft.setCursor(centerX, centerY);
    tft.print(text);

    int progressBarX = 20;
    int progressBarY = tft.height() - 50;
    int progressBarWidth = tft.width() - 40;
    int progressBarHeight = 20;

    tft.drawRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_WHITE);

    for (int progress = 0; progress <= progressBarWidth - 4; progress += 7) {
      uint16_t gradientColor = tft.color565(0, (progress * 255) / (progressBarWidth - 4), 0);
      tft.fillRect(progressBarX + 2, progressBarY + 2, progress, progressBarHeight - 4, gradientColor);

      int percent = min(100, (progress * 100) / (progressBarWidth - 4));
      tft.fillRect(progressBarX, progressBarY - 40, progressBarWidth, 30, ILI9341_BLACK);
      tft.setFont();
      tft.setTextSize(2);
      tft.setCursor(progressBarX + (progressBarWidth / 2) - 20, progressBarY - 30);
      tft.setTextColor(ILI9341_WHITE);
      tft.print(percent);
      tft.print("%");

      delay(20);
    }

    tft.fillRect(progressBarX + 2, progressBarY + 2, progressBarWidth - 4, progressBarHeight - 4, tft.color565(0, 255, 0));
    tft.fillRect(progressBarX, progressBarY - 40, progressBarWidth, 30, ILI9341_BLACK);
    tft.setFont();
    tft.setTextSize(2);
    tft.setCursor(progressBarX + (progressBarWidth / 2) - 20, progressBarY - 30);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("100%");

    tft.setFont();
    screenDrawn = true;

    delay(500);
  }

  screenDrawn = false;
  currentState = STATE_MAIN_MENU;
}

void runWiFiSetupScreen() {

  if (!screenDrawn) {
    resetState();

    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);

    const char* question = "Connect to Wi-Fi?";
    int16_t x1, y1;
    uint16_t w, h;

    // Center the question text
    tft.getTextBounds(question, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (tft.width() - w) / 2;
    int16_t centerY = (tft.height() / 4);
    tft.setCursor(centerX, centerY);
    tft.print(question);

    // "Yes" button
    int16_t yesX = tft.width() / 4 - 50;
    int16_t yesY = centerY + 50;
    tft.fillRect(yesX, yesY, 100, 50, ILI9341_GREEN);

    // Calculate center of "Yes" text
    tft.getTextBounds("Yes", 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(yesX + (100 - w) / 2, yesY + (50 - h) / 2);
    tft.setTextColor(ILI9341_BLACK);
    tft.print("Yes");

    // "No" button
    int16_t noX = (tft.width() * 3 / 4) - 50;
    int16_t noY = centerY + 50;
    tft.fillRect(noX, noY, 100, 50, ILI9341_RED);

    // Calculate center of "No" text
    tft.getTextBounds("No", 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(noX + (100 - w) / 2, noY + (50 - h) / 2);
    tft.print("No");

    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    // Checking "Yes" button
    if (calX > tft.width() / 4 - 50 && calX < tft.width() / 4 + 50 && calY > (tft.height() / 4) + 50 && calY < (tft.height() / 4) + 100) {
      screenDrawn = false;
      currentState = STATE_WIFI_CREDENTIALS;
    }

    // Checking "No" button
    if (calX > (tft.width() * 3 / 4) - 50 && calX < (tft.width() * 3 / 4) + 50 && calY > (tft.height() / 4) + 50 && calY < (tft.height() / 4) + 100) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
    }
  }
}

void runWiFiCredentialsScreen() {
  static bool screenDrawn = false;
  static String ssid = "iPhone";
  static String password = "coolwifi";
  const int inputBoxHeight = 30;
  const int outputBoxHeight = 30;
  const int textAreaY = 35;
  const int passAreaY = textAreaY + inputBoxHeight + 6;
  const int buttonWidth = 30;
  const int buttonHeight = inputBoxHeight;
  const int boxWidth = tft.width() - 70;
  const int maxInputChars = 32;
  const int visibleChars = boxWidth / 12;
  static int ssidScrollOffset = 0;
  static int passScrollOffset = 0;
  static bool isTopBoxSelected = true;
  static bool topPlaceholderActive = (ssid.length() == 0);
  static bool bottomPlaceholderActive = (password.length() == 0);

  if (!screenDrawn) {
    resetState();
    tft.fillScreen(ILI9341_BLACK);

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;

    tft.setCursor(6, 8);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Connect to Wi-Fi");

    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
    tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, textAreaY + 8);
    tft.setTextSize(2);
    if (topPlaceholderActive && ssid.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Enter Wi-Fi Name");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(ssid.substring(ssidScrollOffset, ssidScrollOffset + visibleChars));
    }

    tft.drawRect(5, passAreaY, boxWidth, outputBoxHeight, bottomColor);
    tft.fillRect(6, passAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, passAreaY + 8);
    tft.setTextSize(2);
    if (bottomPlaceholderActive && password.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Enter Wi-Fi Password");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(password.substring(passScrollOffset, passScrollOffset + visibleChars));
    }

    tft.fillRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    tft.fillRect(boxWidth + 10, passAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, passAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, passAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, passAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, passAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, passAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;
    tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
    int16_t textX = exitButtonX + (exitButtonWidth - 48) / 2;
    int16_t textY = exitButtonY + (exitButtonHeight - 16) / 2 + 2;
    tft.setCursor(textX, textY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("EXIT");
    tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);

    drawKeyboard();
    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    if (calX >= 5 && calX <= 5 + boxWidth && calY >= textAreaY && calY <= textAreaY + inputBoxHeight) {
      isTopBoxSelected = true;
      if (topPlaceholderActive && ssid.length() == 0) {
        topPlaceholderActive = false;
      }
    }
    if (calX >= 5 && calX <= 5 + boxWidth && calY >= passAreaY && calY <= passAreaY + outputBoxHeight) {
      isTopBoxSelected = false;
      if (bottomPlaceholderActive && password.length() == 0) {
        bottomPlaceholderActive = false;
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      isTopBoxSelected = true;
      if (ssidScrollOffset > 0) {
        ssidScrollOffset--;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(ssid.substring(ssidScrollOffset, ssidScrollOffset + visibleChars));
      }
    }
    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > passAreaY && calY < passAreaY + buttonHeight) {
      isTopBoxSelected = false;
      if (passScrollOffset > 0) {
        passScrollOffset--;
        tft.fillRect(6, passAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, passAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(password.substring(passScrollOffset, passScrollOffset + visibleChars));
      }
    }
    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      isTopBoxSelected = true;
      int maxOffset = max(0, (int)ssid.length() - visibleChars);
      if (ssidScrollOffset < maxOffset) {
        ssidScrollOffset++;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(ssid.substring(ssidScrollOffset, ssidScrollOffset + visibleChars));
      }
    }
    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > passAreaY && calY < passAreaY + buttonHeight) {
      isTopBoxSelected = false;
      int maxOffset = max(0, (int)password.length() - visibleChars);
      if (passScrollOffset < maxOffset) {
        passScrollOffset++;
        tft.fillRect(6, passAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, passAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(password.substring(passScrollOffset, passScrollOffset + visibleChars));
      }
    }

    char key = getKeyFromCoords(calX, calY);
    if (key) {
      if (key == '>') {
        if (ssid.isEmpty()) {
          isTopBoxSelected = true;
          uint16_t topColor = ILI9341_GREEN;
          uint16_t bottomColor = ILI9341_WHITE;
          tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
          tft.drawRect(5, passAreaY, boxWidth, outputBoxHeight, bottomColor);
        } else {
          strcpy(wifiSSID, ssid.c_str());
          strcpy(wifiPassword, password.c_str());
          currentState = STATE_CONNECTION;
          screenDrawn = false;
        }
      } else if (isTopBoxSelected) {
        if (topPlaceholderActive) {
          topPlaceholderActive = false;
          ssid = "";
        }
        if (key == '<') {
          if (!ssid.isEmpty()) {
            ssid.remove(ssid.length() - 1);
          }
          if (ssid.isEmpty()) {
            topPlaceholderActive = true;
          }
        } else if (ssid.length() < maxInputChars) {
          ssid += key;
        }
      } else {
        if (bottomPlaceholderActive) {
          bottomPlaceholderActive = false;
          password = "";
        }
        if (key == '<') {
          if (!password.isEmpty()) {
            password.remove(password.length() - 1);
          }
          if (password.isEmpty()) {
            bottomPlaceholderActive = true;
          }
        } else if (password.length() < maxInputChars) {
          password += key;
        }
      }

      delay(40);

      ssidScrollOffset = max(0, (int)ssid.length() - visibleChars);
      tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, textAreaY + 8);
      tft.setTextSize(2);
      if (topPlaceholderActive && ssid.length() == 0) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Enter Wi-Fi Name");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(ssid.substring(ssidScrollOffset, ssidScrollOffset + visibleChars));
      }

      passScrollOffset = max(0, (int)password.length() - visibleChars);
      tft.fillRect(6, passAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, passAreaY + 8);
      tft.setTextSize(2);
      if (bottomPlaceholderActive && password.length() == 0) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Enter Wi-Fi Password");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(password.substring(passScrollOffset, passScrollOffset + visibleChars));
      }
    }

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;
    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
    }

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
    tft.drawRect(5, passAreaY, boxWidth, outputBoxHeight, bottomColor);
  }
}

void drawKeyboard() {
  const char* keys;
  if (currentState == STATE_ENCODE) {
    keys = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
  } else {
    switch (currentKeyboardState) {
      case STATE_UPPERCASE:
        keys = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
        break;
      case STATE_LOWERCASE:
        keys = "1234567890qwertyuiopasdfghjklzxcvbnm";
        break;
      case STATE_SYMBOLS:
        keys = "!@#$%^&*()_+-=[]{}|;:'\",.?/\\`~";
        break;
    }
  }

  int screenW = tft.width() - 2 * xMargin;
  int screenH = tft.height();
  int rows = 4;
  int xStart = xMargin;
  int yStart = screenH - (rows * (keyHeight + keySpacing));

  int totalSpacing = (keysPerRow - 1) * keySpacing;
  int usableWidth = screenW - totalSpacing;
  dynamicKeyWidth = usableWidth / keysPerRow;
  int charWidth = 6 * 2;
  int charHeight = 8 * 2;

  tft.fillRect(0, yStart, tft.width(), screenH - yStart, ILI9341_BLACK);

  int index = 0;
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < keysPerRow && index < (int)strlen(keys); ++col) {
      int x = xStart + col * (dynamicKeyWidth + keySpacing);
      int y = yStart + row * (keyHeight + keySpacing);

      tft.fillRect(x, y, dynamicKeyWidth, keyHeight, ILI9341_BLUE);


      tft.setCursor(
        x + (dynamicKeyWidth - charWidth) / 2,
        y + (keyHeight - charHeight) / 2);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2);
      tft.print(keys[index]);
      ++index;
    }
  }

  int spaceRow = 3;
  int spaceCol = 6;
  int spaceX = xStart + spaceCol * (dynamicKeyWidth + keySpacing);
  int spaceY = yStart + spaceRow * (keyHeight + keySpacing);
  tft.fillRect(spaceX, spaceY, dynamicKeyWidth, keyHeight, ILI9341_GRAY);

  if (currentState != STATE_ENCODE) {
    int symbolCol = 7;
    int symbolX = xStart + symbolCol * (dynamicKeyWidth + keySpacing);
    tft.fillRect(symbolX, spaceY, dynamicKeyWidth, keyHeight, ILI9341_YELLOW);
    tft.setCursor(
      symbolX + (dynamicKeyWidth - charWidth) / 2,
      spaceY + (keyHeight - charHeight) / 2);
    tft.setTextColor(ILI9341_BLACK);
    if (currentKeyboardState == STATE_UPPERCASE) {
      tft.print("U");
    } else if (currentKeyboardState == STATE_LOWERCASE) {
      tft.print("L");
    } else {
      tft.print("S");
    }
  }

  tft.setTextColor(ILI9341_BLACK);
  int backspaceCol = 8;
  int backspaceX = xStart + backspaceCol * (dynamicKeyWidth + keySpacing);
  tft.fillRect(backspaceX, spaceY, dynamicKeyWidth, keyHeight, ILI9341_RED);
  tft.setCursor(
    backspaceX + (dynamicKeyWidth - charWidth) / 2,
    spaceY + (keyHeight - charHeight) / 2);
  tft.print("<");

  int enterCol = 9;
  int enterX = xStart + enterCol * (dynamicKeyWidth + keySpacing);
  tft.fillRect(enterX, spaceY, dynamicKeyWidth, keyHeight, ILI9341_GREEN);
  tft.setCursor(
    enterX + (dynamicKeyWidth - charWidth) / 2,
    spaceY + (keyHeight - charHeight) / 2);
  tft.print(">");
}

char getKeyFromCoords(int calX, int calY) {
  int screenW = tft.width() - 2 * xMargin;
  int screenH = tft.height();
  int rows = 4;
  int xStart = xMargin;
  int yStart = screenH - (rows * (keyHeight + keySpacing));

  const char* keys;
  switch (currentKeyboardState) {
    case STATE_UPPERCASE:
      keys = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
      break;
    case STATE_LOWERCASE:
      keys = "1234567890qwertyuiopasdfghjklzxcvbnm";
      break;
    case STATE_SYMBOLS:
      keys = "!@#$%^&*()_+-=[]{}|;:'\",.?/\\`~";
      break;
  }

  int index = 0;
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < keysPerRow && index < (int)strlen(keys); ++col) {
      int x = xStart + col * (dynamicKeyWidth + keySpacing);
      int y = yStart + row * (keyHeight + keySpacing);
      if (calX > x && calX < x + dynamicKeyWidth && calY > y && calY < y + keyHeight) {
        return keys[index];
      }
      ++index;
    }
  }

  int spaceRow = 3;
  int spaceCol = 6;
  int spaceX = xStart + spaceCol * (dynamicKeyWidth + keySpacing);
  int spaceY = yStart + spaceRow * (keyHeight + keySpacing);
  if (calX > spaceX && calX < spaceX + dynamicKeyWidth && calY > spaceY && calY < spaceY + keyHeight) {
    return ' ';
  }

  int symbolCol = 7;
  int symbolX = xStart + symbolCol * (dynamicKeyWidth + keySpacing);
  if (calX > symbolX && calX < symbolX + dynamicKeyWidth && calY > spaceY && calY < spaceY + keyHeight) {
    if (currentState != STATE_ENCODE) {
      if (currentKeyboardState == STATE_UPPERCASE) {
        currentKeyboardState = STATE_LOWERCASE;
      } else if (currentKeyboardState == STATE_LOWERCASE) {
        currentKeyboardState = STATE_SYMBOLS;
      } else {
        currentKeyboardState = STATE_UPPERCASE;
      }
      drawKeyboard();
    }
    return '\0';
  }

  int backspaceCol = 8;
  int backspaceX = xStart + backspaceCol * (dynamicKeyWidth + keySpacing);
  if (calX > backspaceX && calX < backspaceX + dynamicKeyWidth && calY > spaceY && calY < spaceY + keyHeight) {
    return '<';
  }

  int enterCol = 9;
  int enterX = xStart + enterCol * (dynamicKeyWidth + keySpacing);
  if (calX > enterX && calX < enterX + dynamicKeyWidth && calY > spaceY && calY < spaceY + keyHeight) {
    return '>';
  }

  return '\0';
}

void runConnectionState() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);

  int messageX = 10;
  int line1Y = 10;
  int line2Y = 30;

  int progressBarX = 20;
  int progressBarY = tft.height() / 2 - 10;
  int progressBarWidth = tft.width() - 40;
  int progressBarHeight = 20;

  tft.drawRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_WHITE);
  tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
  tft.print("0%");

  tft.setCursor(messageX, line1Y);
  tft.print("Connecting to Wi-Fi...");
  int wifiRetryCount = 0;
  const int maxWiFiRetries = 5;

  while (wifiRetryCount < maxWiFiRetries) {
    WiFi.disconnect();
    delay(500);
    WiFi.begin(wifiSSID, wifiPassword);
    delay(1500);

    int wifiProgress = (progressBarWidth * (wifiRetryCount + 1)) / maxWiFiRetries;
    tft.fillRect(progressBarX, progressBarY, wifiProgress, progressBarHeight, ILI9341_YELLOW);

    int wifiPercentage = ((wifiRetryCount + 1) * 100) / maxWiFiRetries;
    tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 60, 20, ILI9341_BLACK);
    tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
    tft.print(wifiPercentage);
    tft.print("%");

    if (WiFi.status() == WL_CONNECTED) {
      tft.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_GREEN);
      tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 60, 20, ILI9341_BLACK);
      tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
      tft.print("100%");

      tft.fillRect(messageX, line1Y, tft.width(), 40, ILI9341_BLACK);
      tft.setCursor(messageX, line1Y);
      tft.print("Wi-Fi Connected!");

      delay(1000);

      WiFiConnectionHandler ArduinoIoTPreferredConnection(wifiSSID, wifiPassword);
      initProperties();

      ArduinoCloud.begin(ArduinoIoTPreferredConnection, false);
      setDebugMessageLevel(2);
      ArduinoCloud.printDebugInfo();

      tft.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_BLACK);
      tft.drawRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_WHITE);

      tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 100, 20, ILI9341_BLACK);
      tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
      tft.print("0%");

      tft.fillRect(messageX, line1Y, tft.width(), 40, ILI9341_BLACK);
      tft.setCursor(messageX, line1Y);
      tft.print("Connecting to Cloud...");
      delay(500);

      int cloudRetryCount = 0;
      const int maxCloudRetries = 10;
      int cloudProgressStep = progressBarWidth / maxCloudRetries;

      while (cloudRetryCount < maxCloudRetries) {
        ArduinoCloud.update();
        delay(1000);

        int cloudProgress = cloudProgressStep * (cloudRetryCount + 1);
        tft.fillRect(progressBarX, progressBarY, cloudProgress, progressBarHeight, ILI9341_YELLOW);

        int cloudPercentage = ((cloudRetryCount + 1) * 100) / maxCloudRetries;
        tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 100, 20, ILI9341_BLACK);
        tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
        tft.print(cloudPercentage);
        tft.print("%");

        if (ArduinoCloud.connected()) {
          tft.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_GREEN);
          tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 100, 20, ILI9341_BLACK);
          tft.setCursor(progressBarX + progressBarWidth - 30, progressBarY + progressBarHeight + 10);
          tft.print("100%");

          tft.fillRect(messageX, line1Y, tft.width(), 40, ILI9341_BLACK);
          tft.setCursor(messageX, line1Y);
          tft.print("Wi-Fi and Cloud Connected");

          Serial.println("Device connected to Arduino Cloud:");
          Serial.print("Device ID: ");
          Serial.println(ArduinoCloud.getDeviceId());

          delay(2000);
          currentState = STATE_MAIN_MENU;
          return;
        }

        cloudRetryCount++;
      }

      tft.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_RED);
      tft.fillRect(messageX, line1Y, tft.width(), 40, ILI9341_BLACK);
      tft.setCursor(messageX, line1Y);
      tft.print("Cloud Connection Failed.");
      delay(2000);
      currentState = STATE_MAIN_MENU;
      return;
    }

    wifiRetryCount++;
  }

  tft.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, ILI9341_RED);
  tft.fillRect(progressBarX + progressBarWidth - 52, progressBarY + progressBarHeight + 5, 100, 20, ILI9341_BLACK);
  tft.fillRect(messageX, line1Y, tft.width(), 40, ILI9341_BLACK);
  tft.setCursor(messageX, line1Y);
  tft.print("Wi-Fi Connection Failed.");
  delay(2000);
  currentState = STATE_MAIN_MENU;
}

void runMainMenuScreen() {
  static int previousMessageCount = -1;

  const uint16_t colors[] = {
    tft.color565(180, 200, 220),
    tft.color565(180, 200, 220),
    tft.color565(180, 200, 220),
    tft.color565(180, 200, 220),
    tft.color565(180, 200, 220),
    tft.color565(180, 200, 220)
  };

  const uint16_t textColor = ILI9341_BLACK;

  if (!screenDrawn) {
    resetState();

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.println("Main Menu");

    bool isConnected = (WiFi.status() == WL_CONNECTED);
    int wifiSymbolX = tft.width() - 20;
    int wifiSymbolY = 35;

    // Wifi button on the Top right of the Main Menu
    // If Wifi is connected its Blue, if not its white with an 'X' Symbol
    drawWiFiSymbol(wifiSymbolX, wifiSymbolY, isConnected);
    previousWiFiStatus = isConnected;

    int mailButtonX = wifiSymbolX - 100;
    int mailButtonY = 10;
    int mailButtonWidth = 70;
    int mailButtonHeight = 30;

    tft.fillRect(mailButtonX, mailButtonY, mailButtonWidth, mailButtonHeight, ILI9341_BLUE);
    tft.setCursor(mailButtonX + 5, mailButtonY + 8);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Mail");

    // 6 Tiles
    int boxWidth = (tft.width() - 40) / 3;
    int boxHeight = 60;
    int boxSpacing = 10;
    int startY = 80;
    const char* labels[] = { "Guide", "Encode", "Decode", "Game", "Tools", "Other" };

    for (int i = 0; i < 6; ++i) {
      int row = i / 3;
      int col = i % 3;
      int boxX = 10 + col * (boxWidth + boxSpacing);
      int boxY = startY + row * (boxHeight + boxSpacing);

      tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 8, colors[i]);
      tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, 8, ILI9341_WHITE);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(labels[i], 0, 0, &x1, &y1, &w, &h);
      tft.setCursor(boxX + (boxWidth - w) / 2, boxY + (boxHeight - h) / 2);
      tft.setTextColor(textColor);
      tft.setTextSize(2);
      tft.print(labels[i]);
    }

    screenDrawn = true;

    previousMessageCount = -1;
  }

  bool isConnected = (WiFi.status() == WL_CONNECTED);
  if (isConnected != previousWiFiStatus) {
    drawWiFiSymbol(tft.width() - 20, 35, isConnected);
    previousWiFiStatus = isConnected;
  }

  int currentMessageCount = messageQueue.size();
  if (currentMessageCount != previousMessageCount) {
    int mailButtonX = tft.width() - 120;
    int mailButtonY = 10;
    int mailButtonWidth = 70;

    tft.fillCircle(mailButtonX + mailButtonWidth - 5, mailButtonY + 5, 10, ILI9341_RED);

    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    int16_t countX = mailButtonX + mailButtonWidth - 9;
    int16_t countY = mailButtonY;
    tft.setCursor(countX, countY);
    tft.print(currentMessageCount);

    previousMessageCount = currentMessageCount;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    int mailButtonX = tft.width() - 120;
    int mailButtonY = 10;
    int mailButtonWidth = 70;
    int mailButtonHeight = 30;

    int wifiButtonX = tft.width() - 40;
    int wifiButtonY = 10;
    int wifiButtonWidth = 35;
    int wifiButtonHeight = 20;

    if (calX > mailButtonX && calX < mailButtonX + mailButtonWidth && calY > mailButtonY && calY < mailButtonY + mailButtonHeight) {
      screenDrawn = false;
      currentState = STATE_MAIL;
    }

    if (calX > wifiButtonX && calX < wifiButtonX + wifiButtonWidth && calY > wifiButtonY && calY < wifiButtonY + wifiButtonHeight) {
      screenDrawn = false;
      currentState = STATE_WIFI_SETUP;
    }

    // Check if a tile is clicked
    int boxWidth = (tft.width() - 40) / 3;
    int boxHeight = 60;
    int boxSpacing = 10;
    int startY = 80;

    for (int i = 0; i < 6; ++i) {
      int row = i / 3;
      int col = i % 3;
      int boxX = 10 + col * (boxWidth + boxSpacing);
      int boxY = startY + row * (boxHeight + boxSpacing);

      if (calX > boxX && calX < boxX + boxWidth && calY > boxY && calY < boxY + boxHeight) {
        screenDrawn = false;
        switch (i) {
          case 0: currentState = STATE_GUIDE; break;
          case 1: currentState = STATE_ENCODE; break;
          case 2: currentState = STATE_DECODE; break;
          case 3: currentState = STATE_GAME; break;
          case 4: currentState = STATE_TOOLS; break;
          case 5: currentState = STATE_OTHER; break;
        }
        return;
      }
    }
  }
}

void drawWiFiSymbol(int centerX, int centerY, bool isConnected) {
  uint16_t centerDotColor = isConnected ? tft.color565(0, 255, 0) : ILI9341_WHITE;
  uint16_t arcColor = isConnected ? tft.color565(0, 255, 0) : ILI9341_WHITE;

  tft.fillCircle(centerX, centerY - 4, 2, centerDotColor);

  int startRadius = 10;
  int step = 6;
  for (int radius = startRadius; radius <= 26; radius += step) {
    for (float angle = PI / 4; angle <= (3 * PI) / 4; angle += 0.01) {
      int px = centerX + radius * cos(angle);
      int py = centerY - radius * sin(angle);
      tft.drawPixel(px, py, arcColor);
    }
  }

  if (!isConnected) {
    int xSize = 7;
    uint16_t xColor = ILI9341_RED;
    int xOffsetY = -15;
    int thickness = 1;

    for (int i = -thickness; i <= thickness; ++i) {
      tft.drawLine(centerX - xSize, centerY - xSize + xOffsetY + i, centerX + xSize, centerY + xSize + xOffsetY + i, xColor);
      tft.drawLine(centerX - xSize + i, centerY + xSize + xOffsetY, centerX + xSize + i, centerY - xSize + xOffsetY, xColor);
    }
  }
}

void resetState() {
  while (ts.getPoint().z > MINPRESSURE && ts.getPoint().z < MAXPRESSURE) {
    delay(5);
  }
  screenDrawn = false;
}

void onTextMessageChange() {
  textMessage.trim();
  if (textMessage.isEmpty() || textMessage == "" || textMessage.startsWith("Message Denied:") || textMessage.startsWith("PMC received:") || textMessage == "\x1B") {
    return;
  }

  Serial.print("Message received: ");
  Serial.println(textMessage);

  messageQueue.push_back(textMessage);
}

void runMailScreen() {
  if (messageQueue.empty()) {
    noMailPopup();
    return;
  }

  static bool screenDrawn = false;
  static bool messageDecoded = false;
  static bool messageSent = false;
  static String inputMessage;
  static String outputMessage = "";
  static String originalMessage = "";
  const int inputBoxHeight = 30;
  const int outputBoxHeight = 30;
  const int labelHeight = 20;
  const int labelMargin = 5;
  const int textAreaY = 10;
  const int incomingLabelY = textAreaY;
  const int inputBoxY = incomingLabelY + labelHeight + labelMargin + 6;
  const int outputAreaY = inputBoxY + inputBoxHeight + labelMargin + 2;
  const int buttonWidth = 30;
  const int buttonHeight = inputBoxHeight;
  const int boxWidth = tft.width() - 70;
  const int visibleChars = boxWidth / 12;
  static int inputScrollOffset = 0;
  static int outputScrollOffset = 0;
  static bool isTopBoxSelected = true;
  static int totalMessages = 0;
  static int currentMessageIndex = 0;
  const int messageY = outputAreaY + outputBoxHeight + 20;
  int buttonX = 80;
  int buttonY = messageY + 25;

  if (!screenDrawn) {
    if (!messageQueue.empty()) {
      totalMessages = messageQueue.size();
      if (currentMessageIndex >= totalMessages) {
        currentMessageIndex = totalMessages - 1;
      }
      originalMessage = messageQueue[currentMessageIndex];
      inputMessage = textToMorse(originalMessage);
    }

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(5, incomingLabelY + 2);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Incoming Messages");

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, topColor);
    tft.fillRect(6, inputBoxY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, inputBoxY + 8);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    if (inputMessage.length() > 0) {
      int displayLength = min(visibleChars, (int)(inputMessage.length() - inputScrollOffset));
      tft.print(inputMessage.substring(inputScrollOffset, inputScrollOffset + displayLength));
    }

    tft.setCursor(7, messageY);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Decode Incomming Message?");

    tft.setCursor(buttonX, buttonY);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Decode");

    tft.setCursor(buttonX + 95, buttonY);
    tft.setTextColor(ILI9341_RED);
    tft.print("Reject");

    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
    tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, bottomColor);
    tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, outputAreaY + 8);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);

    for (int y = inputBoxY, i = 0; i < 2; i++, y = outputAreaY) {
      tft.fillRect(boxWidth + 10, y, buttonWidth, buttonHeight, ILI9341_BLUE);
      tft.drawRect(boxWidth + 10, y, buttonWidth, buttonHeight, ILI9341_WHITE);
      tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, y + (buttonHeight - 16) / 2);
      tft.print("<");

      tft.fillRect(boxWidth + 40, y, buttonWidth, buttonHeight, ILI9341_BLUE);
      tft.drawRect(boxWidth + 40, y, buttonWidth, buttonHeight, ILI9341_WHITE);
      tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, y + (buttonHeight - 16) / 2);
      tft.print(">");
    }

    int exitButtonX = tft.width() - 60;
    int exitButtonY = incomingLabelY;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;
    tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
    tft.setCursor(exitButtonX + (exitButtonWidth - 48) / 2, exitButtonY + (exitButtonHeight - 16) / 2 + 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("EXIT");
    tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);

    if (totalMessages > 0) {
      String counter = String(currentMessageIndex + 1) + " of " + String(totalMessages);
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(counter, 0, 0, &x1, &y1, &w, &h);

      int lineY = tft.height() - 20;
      int centerX = ((tft.width() - w) / 2) + 4;
      tft.setCursor(centerX, lineY);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE);
      tft.print(counter);

      if (totalMessages > 1) {
        const int bottomBtnW = 60;
        const int bottomBtnH = 30;
        int prevBtnX = 0;
        int nextBtnX = tft.width() - bottomBtnW;
        int btnY = lineY - 10;

        if (currentMessageIndex > 0) {
          tft.setCursor(prevBtnX + 5, btnY + 7);
          tft.setTextSize(2);
          tft.setTextColor(ILI9341_YELLOW);
          tft.print("Prev");
        }

        if (currentMessageIndex < (totalMessages - 1)) {
          tft.setCursor(nextBtnX + 5, btnY + 7);
          tft.setTextSize(2);
          tft.setTextColor(ILI9341_YELLOW);
          tft.print("Next");
        }
      }
    }

    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    int decodeStartX = buttonX;
    int decodeEndX = buttonX + 60;
    int decodeStartY = buttonY - 10;
    int decodeEndY = buttonY + 15;

    if (calX > decodeStartX && calX < decodeEndX && calY > decodeStartY && calY < decodeEndY) {

      audioAndLightMorse(inputMessage, BUZZER_PIN, LED_PIN, WPM, buzzerFrequency);

      if (!messageSent) {
        textMessage = "PMC received: \"" + originalMessage + "\" | Morse: " + inputMessage;
        ArduinoCloud.update();
        messageSent = true;
      }

      messageDecoded = true;
      isTopBoxSelected = false;
      outputMessage = originalMessage;

      tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_WHITE);
      tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_GREEN);
      tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, outputAreaY + 8);
      tft.setTextColor(ILI9341_WHITE);
      int displayLength = min(visibleChars, (int)(outputMessage.length() - outputScrollOffset));
      tft.print(outputMessage.substring(outputScrollOffset, outputScrollOffset + displayLength));
    }

    int rejectStartX = buttonX + 95;
    int rejectEndX = rejectStartX + 55;
    int rejectStartY = buttonY - 10;
    int rejectEndY = buttonY + 15;

    if (calX > rejectStartX && calX < rejectEndX && calY > rejectStartY && calY < rejectEndY) {
      if (!messageDecoded) {
        textMessage = "Message Denied: \"" + messageQueue[currentMessageIndex] + "\"";
        ArduinoCloud.update();

        if (!messageQueue.empty() && currentMessageIndex < messageQueue.size()) {
          messageQueue.erase(messageQueue.begin() + currentMessageIndex);
          totalMessages--;

          if (totalMessages == 0) {
            screenDrawn = false;
            currentState = STATE_MAIN_MENU;
            return;
          }

          if (currentMessageIndex >= totalMessages) {
            currentMessageIndex = totalMessages - 1;
          }
          inputMessage = messageQueue[currentMessageIndex];
        }

        messageDecoded = false;
        messageSent = false;
        screenDrawn = false;
      }
    }


    if (calX >= 5 && calX <= 5 + boxWidth) {
      if (calY >= inputBoxY && calY <= inputBoxY + inputBoxHeight && !isTopBoxSelected) {
        isTopBoxSelected = true;
        tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_GREEN);
        tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_WHITE);
      }
      if (calY >= outputAreaY && calY <= outputAreaY + outputBoxHeight && isTopBoxSelected) {
        isTopBoxSelected = false;
        tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_WHITE);
        tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_GREEN);
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40) {
      if (calY > inputBoxY && calY < inputBoxY + buttonHeight) {
        if (!isTopBoxSelected) {
          isTopBoxSelected = true;
          tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_GREEN);
          tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_WHITE);
        }
        if (inputScrollOffset > 0) {
          inputScrollOffset--;
          tft.fillRect(6, inputBoxY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
          tft.setCursor(10, inputBoxY + 8);
          tft.setTextColor(ILI9341_WHITE);
          int displayLength = min(visibleChars, (int)(inputMessage.length() - inputScrollOffset));
          tft.print(inputMessage.substring(inputScrollOffset, inputScrollOffset + displayLength));
        }
      } else if (calY > outputAreaY && calY < outputAreaY + buttonHeight) {
        if (isTopBoxSelected) {
          isTopBoxSelected = false;
          tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_WHITE);
          tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_GREEN);
        }
        if (outputScrollOffset > 0) {
          outputScrollOffset--;
          tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
          tft.setCursor(10, outputAreaY + 8);
          tft.setTextColor(ILI9341_WHITE);
          int displayLength = min(visibleChars, (int)(outputMessage.length() - outputScrollOffset));
          tft.print(outputMessage.substring(outputScrollOffset, outputScrollOffset + displayLength));
        }
      }
    }

    if (calX > boxWidth + 40 && calX < boxWidth + 70) {
      if (calY > inputBoxY && calY < inputBoxY + buttonHeight) {
        if (!isTopBoxSelected) {
          isTopBoxSelected = true;
          tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_GREEN);
          tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_WHITE);
        }
        if (inputScrollOffset < (int)(inputMessage.length() - visibleChars)) {
          inputScrollOffset++;
          tft.fillRect(6, inputBoxY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
          tft.setCursor(10, inputBoxY + 8);
          tft.setTextColor(ILI9341_WHITE);
          int displayLength = min(visibleChars, (int)(inputMessage.length() - inputScrollOffset));
          tft.print(inputMessage.substring(inputScrollOffset, inputScrollOffset + displayLength));
        }
      } else if (calY > outputAreaY && calY < outputAreaY + buttonHeight) {
        if (isTopBoxSelected) {
          isTopBoxSelected = false;
          tft.drawRect(5, inputBoxY, boxWidth, inputBoxHeight, ILI9341_WHITE);
          tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, ILI9341_GREEN);
        }
        if (outputScrollOffset < (int)(outputMessage.length() - visibleChars)) {
          outputScrollOffset++;
          tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
          tft.setCursor(10, outputAreaY + 8);
          tft.setTextColor(ILI9341_WHITE);
          int displayLength = min(visibleChars, (int)(outputMessage.length() - outputScrollOffset));
          tft.print(outputMessage.substring(outputScrollOffset, outputScrollOffset + displayLength));
        }
      }
    }

    int exitButtonX = tft.width() - 70;
    int exitButtonY = incomingLabelY;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;

    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      if (messageDecoded && !messageQueue.empty() && currentMessageIndex < messageQueue.size()) {
        messageQueue.erase(messageQueue.begin() + currentMessageIndex);
        totalMessages--;
        messageDecoded = false;
      }

      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
      messageSent = false;
      return;
    }

    if (totalMessages > 1) {
      int lineY = tft.height() - 20;
      const int bottomBtnW = 60;
      const int bottomBtnH = 30;
      int prevBtnX = 0;
      int nextBtnX = tft.width() - bottomBtnW;
      int btnY = lineY - 10;

      if (currentMessageIndex > 0 && calX >= prevBtnX && calX <= prevBtnX + bottomBtnW && calY >= btnY && calY <= btnY + bottomBtnH) {
        if (messageDecoded && !messageQueue.empty() && currentMessageIndex < messageQueue.size()) {
          messageQueue.erase(messageQueue.begin() + currentMessageIndex);
          totalMessages--;
          messageDecoded = false;
        }

        if (totalMessages == 0) {
          screenDrawn = false;
          currentState = STATE_MAIN_MENU;
          return;
        }

        currentMessageIndex--;
        inputScrollOffset = 0;
        inputMessage = messageQueue[currentMessageIndex];
        outputMessage = "";
        outputScrollOffset = 0;
        messageSent = false;
        screenDrawn = false;
      }

      if (currentMessageIndex < totalMessages - 1 && calX >= nextBtnX && calX <= nextBtnX + bottomBtnW && calY >= btnY && calY <= btnY + bottomBtnH) {
        if (messageDecoded && !messageQueue.empty() && currentMessageIndex < messageQueue.size()) {
          messageQueue.erase(messageQueue.begin() + currentMessageIndex);
          totalMessages--;
          messageDecoded = false;
        }

        if (totalMessages == 0) {
          screenDrawn = false;
          currentState = STATE_MAIN_MENU;
          return;
        }

        currentMessageIndex++;
        inputScrollOffset = 0;
        inputMessage = messageQueue[currentMessageIndex];
        outputMessage = "";
        outputScrollOffset = 0;
        messageSent = false;
        screenDrawn = false;
      }
    }
  }
}

void noMailPopup() {
  int popupWidth = tft.width() - 140;
  int popupHeight = 80;
  int popupX = (tft.width() - popupWidth) / 2;
  int popupY = (tft.height() - popupHeight) / 2;

  int okButtonWidth = 60;
  int okButtonHeight = 30;
  int okButtonX = popupX + (popupWidth - okButtonWidth) / 2;
  int okButtonY = popupY + popupHeight - okButtonHeight - 10;

  tft.fillRect(popupX, popupY, popupWidth, popupHeight, ILI9341_BLACK);
  tft.drawRect(popupX, popupY, popupWidth, popupHeight, ILI9341_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  int textX = popupX + (popupWidth - (7 * 2 * 6)) / 2;
  int textY = popupY + 15;
  tft.setCursor(textX, textY);
  tft.print("No Mail.");

  tft.fillRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, ILI9341_RED);
  tft.drawRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(okButtonX + 12, okButtonY + 8);
  tft.print("OK");

  while (true) {
    TSPoint p = ts.getPoint();
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      int calX = (p.y * xCalM) + xCalC;
      int calY = (p.x * yCalM) + yCalC;

      if (calX > okButtonX && calX < okButtonX + okButtonWidth && calY > okButtonY && calY < okButtonY + okButtonHeight) {
        delay(200);
        screenDrawn = false;
        currentState = STATE_MAIN_MENU;
        return;
      }
    }
  }
}

void guideScreen() {
  static bool isShowingNumbers = false;

  if (!screenDrawn) {
    resetState();
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);

    if (!isShowingNumbers) {
      const char* morseCodes[] = {
        ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
        "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
        "..-", "...-", ".--", "-..-", "-.--", "--.."
      };
      const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

      int colWidth = tft.width() / 3;
      int rowHeight = 20;
      int colX[] = { 10, colWidth + 10, 2 * colWidth + 10 };
      int startY = 10;

      int index = 0;
      for (int col = 0; col < 3; col++) {
        int currentY = startY;
        for (int row = 0; row < 9 && index < 26; row++) {
          tft.setCursor(colX[col], currentY);
          tft.print(letters[index]);
          tft.print(": ");
          tft.print(morseCodes[index]);
          currentY += rowHeight;
          index++;
        }
      }
    } else {
      const char* morseCodes[] = {
        "-----", ".----", "..---", "...--", "....-", ".....",
        "-....", "--...", "---..", "----."
      };
      const char numbers[] = "0123456789";

      int colWidth = tft.width() / 2;
      int rowHeight = 20;
      int colX[] = { 10, colWidth + 10 };
      int startY = 10;

      int index = 0;
      for (int col = 0; col < 2; col++) {
        int currentY = startY;
        for (int row = 0; row < 5 && index < 10; row++) {
          tft.setCursor(colX[col], currentY);
          tft.print(numbers[index]);
          tft.print(": ");
          tft.print(morseCodes[index]);
          currentY += rowHeight;
          index++;
        }
      }
    }

    int toggleButtonX = 10;
    int toggleButtonY = tft.height() - 40;
    int toggleButtonWidth = 100;
    int toggleButtonHeight = 30;

    tft.fillRect(toggleButtonX, toggleButtonY, toggleButtonWidth, toggleButtonHeight, ILI9341_BLUE);
    tft.drawRect(toggleButtonX, toggleButtonY, toggleButtonWidth, toggleButtonHeight, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(toggleButtonX + 10, toggleButtonY + 8);
    tft.print(isShowingNumbers ? "Letters" : "Numbers");

    int exitButtonX = tft.width() - 110;
    int exitButtonY = tft.height() - 40;
    int exitButtonWidth = 100;
    int exitButtonHeight = 30;

    tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
    tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(exitButtonX + 25, exitButtonY + 8);
    tft.print("EXIT");

    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    int toggleButtonX = 10;
    int toggleButtonY = tft.height() - 40;
    int toggleButtonWidth = 100;
    int toggleButtonHeight = 30;
    if (calX > toggleButtonX && calX < toggleButtonX + toggleButtonWidth && calY > toggleButtonY && calY < toggleButtonY + toggleButtonHeight) {
      screenDrawn = false;
      isShowingNumbers = !isShowingNumbers;
      return;
    }

    int exitButtonX = tft.width() - 110;
    int exitButtonY = tft.height() - 40;
    int exitButtonWidth = 100;
    int exitButtonHeight = 30;
    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
      return;
    }
  }
}

void runEncodeScreen() {
  static String inputText = "";
  static String morseText = "";
  const int inputBoxHeight = 30;
  const int outputBoxHeight = 30;
  const int textAreaY = 35;
  const int morseAreaY = textAreaY + inputBoxHeight + 6;
  const int buttonWidth = 30;
  const int buttonHeight = inputBoxHeight;
  const int boxWidth = tft.width() - 70;
  const int maxInputChars = 32;
  const int visibleChars = boxWidth / 12;
  static int inputScrollOffset = 0;
  static int morseScrollOffset = 0;
  static bool isTopBoxSelected = true;
  static bool topPlaceholderActive = true;
  static bool bottomPlaceholderActive = true;

  if (!screenDrawn) {
    resetState();
    tft.fillScreen(ILI9341_BLACK);

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;

    tft.setCursor(6, 9);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Encode Messages");

    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
    tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, textAreaY + 8);
    tft.setTextSize(2);
    if (topPlaceholderActive && inputText.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Enter Message");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
    }

    tft.drawRect(5, morseAreaY, boxWidth, outputBoxHeight, bottomColor);
    tft.fillRect(6, morseAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, morseAreaY + 8);
    tft.setTextSize(2);
    if (bottomPlaceholderActive && morseText.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Encoded Message");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(morseText.substring(morseScrollOffset, morseScrollOffset + visibleChars));
    }

    tft.fillRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    tft.fillRect(boxWidth + 10, morseAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, morseAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, morseAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, morseAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, morseAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, morseAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;


    tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
    int16_t textX = exitButtonX + (exitButtonWidth - 48) / 2;
    int16_t textY = exitButtonY + (exitButtonHeight - 16) / 2 + 2;

    tft.setCursor(textX, textY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("EXIT");
    tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);

    drawKeyboard();
    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;

    if (calX >= 5 && calX <= 5 + boxWidth && calY >= textAreaY && calY <= textAreaY + inputBoxHeight) {
      isTopBoxSelected = true;
      if (topPlaceholderActive && inputText.length() == 0) {
        topPlaceholderActive = false;
      }
    }

    if (calX >= 5 && calX <= 5 + boxWidth && calY >= morseAreaY && calY <= morseAreaY + outputBoxHeight) {
      isTopBoxSelected = false;
      if (bottomPlaceholderActive && morseText.length() == 0) {
        bottomPlaceholderActive = false;
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      isTopBoxSelected = true;
      if (inputScrollOffset > 0) {
        inputScrollOffset--;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > morseAreaY && calY < morseAreaY + buttonHeight) {
      isTopBoxSelected = false;
      if (morseScrollOffset > 0) {
        morseScrollOffset--;
        tft.fillRect(6, morseAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, morseAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(morseText.substring(morseScrollOffset, morseScrollOffset + visibleChars));
      }
    }

    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      isTopBoxSelected = true;
      int maxOffset = max(0, (int)inputText.length() - visibleChars);
      if (inputScrollOffset < maxOffset) {
        inputScrollOffset++;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
      }
    }

    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > morseAreaY && calY < morseAreaY + buttonHeight) {
      isTopBoxSelected = false;
      int maxOffset = max(0, (int)morseText.length() - visibleChars);
      if (morseScrollOffset < maxOffset) {
        morseScrollOffset++;
        tft.fillRect(6, morseAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, morseAreaY + 8);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print(morseText.substring(morseScrollOffset, morseScrollOffset + visibleChars));
      }
    }

    char key = getKeyFromCoords(calX, calY);
    if (key) {
      if (key == '>') {
        if (inputText.isEmpty()) {
          morseText = "Invalid Text";
          bottomPlaceholderActive = false;
        } else {
          morseText = textToMorse(inputText);

          audioAndLightMorse(morseText, BUZZER_PIN, LED_PIN, WPM, buzzerFrequency);

          if (morseText == "Invalid Text" || morseText.isEmpty()) {
            bottomPlaceholderActive = true;
            morseText = "";
          } else {
            bottomPlaceholderActive = false;
          }
        }
        isTopBoxSelected = false;
      } else if (isTopBoxSelected) {
        if (topPlaceholderActive) {
          topPlaceholderActive = false;
          inputText = "";
        }
        if (key == '<') {
          if (!inputText.isEmpty()) {
            inputText.remove(inputText.length() - 1);
          }
          if (inputText.isEmpty()) {
            topPlaceholderActive = true;
            morseText = "";
            bottomPlaceholderActive = true;
          } else {
            morseText = textToMorse(inputText);
            if (morseText == "Invalid Text" || morseText.isEmpty()) {
              bottomPlaceholderActive = true;
              morseText = "";
            } else {
              bottomPlaceholderActive = false;
            }
          }
        } else if (inputText.length() < maxInputChars) {
          inputText += key;
          morseText = textToMorse(inputText);
          if (morseText == "Invalid Text" || morseText.isEmpty()) {
            bottomPlaceholderActive = true;
            morseText = "";
          } else {
            bottomPlaceholderActive = false;
          }
        }
      }

      delay(40);

      inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
      tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, textAreaY + 8);
      tft.setTextSize(2);
      if (topPlaceholderActive && inputText.length() == 0) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Enter Message");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
      }

      morseScrollOffset = max(0, (int)morseText.length() - visibleChars);
      tft.fillRect(6, morseAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, morseAreaY + 8);
      tft.setTextSize(2);
      if (bottomPlaceholderActive && morseText.length() == 0) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Encoded Message");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(morseText.substring(morseScrollOffset, morseScrollOffset + visibleChars));
      }
    }

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;


    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
    }

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
    tft.drawRect(5, morseAreaY, boxWidth, outputBoxHeight, bottomColor);
  }
}

void runDecodeScreen() {
  static String inputText = "";
  static String outputText = "";
  const int inputBoxHeight = 30;
  const int outputBoxHeight = 30;
  const int textAreaY = 35;
  const int outputAreaY = textAreaY + inputBoxHeight + 6;
  const int buttonWidth = 30;
  const int buttonHeight = inputBoxHeight;
  const int boxWidth = tft.width() - 70;
  const int visibleChars = boxWidth / 12;
  static int inputScrollOffset = 0;
  static int outputScrollOffset = 0;
  static bool isTopBoxSelected = true;
  static bool topPlaceholderActive = true;
  static bool bottomPlaceholderActive = true;
  static bool screenDrawn = false;
  const int spacebarWidth = 140;
  const int spacebarHeight = 30;
  const int spacebarX = 5;
  const int spacebarY = tft.height() - spacebarHeight - 10;
  static unsigned long lastButtonReleaseTime = 0;
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  const int nextLetterY = outputAreaY + outputBoxHeight + 10;

  static bool timerActive = false;
  static unsigned long timerStartTime = 0;
  static String lastTimerDisplay = "";
  static int timerX = -1;
  static unsigned long lastTimerDisplayUpdate = 0;

  if (!screenDrawn) {
    resetState();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(6, 9);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Decode Messages");

    tft.fillRect(spacebarX, spacebarY, spacebarWidth, spacebarHeight, ILI9341_DARKGREY);
    tft.drawRect(spacebarX, spacebarY, spacebarWidth, spacebarHeight, ILI9341_WHITE);
    tft.setCursor(spacebarX + (spacebarWidth - 40) / 2 - 10, spacebarY + (spacebarHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("Space");

    int deleteButtonX = spacebarX + spacebarWidth + 8;
    int deleteButtonY = spacebarY;
    int deleteButtonWidth = 100;
    int deleteButtonHeight = spacebarHeight;

    tft.fillRect(deleteButtonX, deleteButtonY, deleteButtonWidth, deleteButtonHeight, ILI9341_RED);
    tft.drawRect(deleteButtonX, deleteButtonY, deleteButtonWidth, deleteButtonHeight, ILI9341_WHITE);
    tft.setCursor(deleteButtonX + 15, deleteButtonY + (deleteButtonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("Delete");

    int helpButtonX = deleteButtonX + deleteButtonWidth + 6;
    int helpButtonY = deleteButtonY;
    int helpButtonWidth = 60;
    int helpButtonHeight = deleteButtonHeight;

    tft.drawRect(helpButtonX, helpButtonY, helpButtonWidth, helpButtonHeight, ILI9341_YELLOW);
    tft.setCursor(helpButtonX + (helpButtonWidth - 12) / 2, helpButtonY + (helpButtonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("?");

    uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
    tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, textAreaY + 8);
    tft.setTextSize(2);

    if (inputText.length() == 0) {
      topPlaceholderActive = true;
    }
    if (outputText.length() == 0) {
      bottomPlaceholderActive = true;
    }

    if (topPlaceholderActive && inputText.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Enter Code");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
    }

    uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
    tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, bottomColor);
    tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
    tft.setCursor(10, outputAreaY + 8);
    tft.setTextSize(2);

    if (bottomPlaceholderActive && outputText.length() == 0) {
      tft.setTextColor(ILI9341_GRAY);
      tft.print("Decoded Message");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
    }

    tft.setCursor(8, nextLetterY + 5);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("Next Letter In: ");

    if (timerX == -1) {
      timerX = tft.getCursorX();
    }

    tft.setCursor(timerX, nextLetterY + 5);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.print("0.00");
    tft.setTextSize(1);
    tft.setCursor(timerX + 50, nextLetterY + 13);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("sec(s)");
    lastTimerDisplay = "0.00";

    tft.fillRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, textAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, textAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    tft.fillRect(boxWidth + 10, outputAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 10, outputAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 10 + (buttonWidth - 12) / 2, outputAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("<");

    tft.fillRect(boxWidth + 40, outputAreaY, buttonWidth, buttonHeight, ILI9341_BLUE);
    tft.drawRect(boxWidth + 40, outputAreaY, buttonWidth, buttonHeight, ILI9341_WHITE);
    tft.setCursor(boxWidth + 40 + (buttonWidth - 12) / 2, outputAreaY + (buttonHeight - 16) / 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(">");

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;

    tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
    tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);
    tft.setCursor(exitButtonX + (exitButtonWidth - 48) / 2, exitButtonY + (exitButtonHeight - 16) / 2 + 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("EXIT");
    screenDrawn = true;
  }

  if (isTopBoxSelected) {
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
      buttonPressed = true;
      pressStartTime = millis();
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, buzzerFrequency);
    }

    if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
      buttonPressed = false;
      unsigned long pressTime = millis() - pressStartTime;
      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

      if (pressTime < buttonPressTimingThreshold) {
        inputText += ".";
      } else {
        inputText += "-";
      }

      outputText = morseToText(BUTTON_PIN, inputText, buttonPressTimingThreshold);
      inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
      outputScrollOffset = max(0, (int)outputText.length() - visibleChars);

      if (topPlaceholderActive && inputText.length() > 0) {
        topPlaceholderActive = false;
      }
      if (bottomPlaceholderActive && outputText.length() > 0) {
        bottomPlaceholderActive = false;
      }

      tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, textAreaY + 8);
      tft.setTextSize(2);

      if (inputText.length() == 0 && topPlaceholderActive) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Enter Code");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
      }

      tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
      tft.setCursor(10, outputAreaY + 8);
      tft.setTextSize(2);

      if (outputText.length() == 0 && bottomPlaceholderActive) {
        tft.setTextColor(ILI9341_GRAY);
        tft.print("Decoded Message");
      } else {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
      }
      lastButtonReleaseTime = millis();
      timerActive = true;
      timerStartTime = millis();
    }
  }

  if (isTopBoxSelected && !buttonPressed && (millis() - lastButtonReleaseTime >= letterTerminationDelay)) {
    if (inputText.length() > 0) {
      char lastChar = inputText.charAt(inputText.length() - 1);
      if (lastChar != ' ' && lastChar != '/') {
        inputText += " ";
        outputText = morseToText(BUTTON_PIN, inputText, buttonPressTimingThreshold);
        inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
        outputScrollOffset = max(0, (int)outputText.length() - visibleChars);

        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);
        if (inputText.length() == 0 && topPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Enter Code");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
        }

        tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, outputAreaY + 8);
        tft.setTextSize(2);
        if (outputText.length() == 0 && bottomPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Decoded Message");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
        }
      }
      lastButtonReleaseTime = millis();
    }
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;
    if (calX >= spacebarX && calX <= spacebarX + spacebarWidth && calY >= spacebarY && calY <= spacebarY + spacebarHeight) {
      if (isTopBoxSelected) {
        if (inputText.length() > 0 && inputText.charAt(inputText.length() - 1) == ' ') {
          inputText.remove(inputText.length() - 1);
        }
        inputText += " / ";
        outputText = morseToText(BUTTON_PIN, inputText, buttonPressTimingThreshold);
        inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
        outputScrollOffset = max(0, (int)outputText.length() - visibleChars);

        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);

        if (inputText.length() == 0 && topPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Enter Code");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
        }

        tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, outputAreaY + 8);
        tft.setTextSize(2);

        if (outputText.length() == 0 && bottomPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Decoded Message");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
        }
        delay(50);

        timerActive = false;
        lastTimerDisplay = "0.00";
        tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
        tft.setCursor(timerX, nextLetterY + 5);
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(2);
        tft.print("0.00");
      }
    }

    int deleteButtonX = spacebarX + spacebarWidth + 8;
    int deleteButtonY = spacebarY;
    int deleteButtonWidth = 100;
    int deleteButtonHeight = spacebarHeight;

    if (calX >= deleteButtonX && calX <= deleteButtonX + deleteButtonWidth && calY >= deleteButtonY && calY <= deleteButtonY + deleteButtonHeight) {
      if (isTopBoxSelected) {
        if (inputText.length() > 0) {
          if (inputText.charAt(inputText.length() - 1) == ' ') {
            inputText.remove(inputText.length() - 1);
          }
          int lastSpaceIndex = inputText.lastIndexOf(' ');
          if (lastSpaceIndex == -1) {
            inputText = "";
          } else {
            inputText.remove(lastSpaceIndex);
          }
        }

        outputText = morseToText(BUTTON_PIN, inputText, buttonPressTimingThreshold);
        inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
        outputScrollOffset = max(0, (int)outputText.length() - visibleChars);

        if (inputText.length() == 0 || outputText.length() == 0) {
          topPlaceholderActive = true;
          bottomPlaceholderActive = true;
        }

        lastButtonReleaseTime = 0;

        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);

        if (inputText.length() == 0 && topPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Enter Code");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
        }

        tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, outputAreaY + 8);
        tft.setTextSize(2);

        if (outputText.length() == 0 && bottomPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Decoded Message");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
        }
        delay(50);

        timerActive = false;
        lastTimerDisplay = "0.00";
        tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
        tft.setCursor(timerX, nextLetterY + 5);
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(2);
        tft.print("0.00");
      }
    }

    int helpButtonX = deleteButtonX + deleteButtonWidth + 6;
    int helpButtonY = deleteButtonY;
    int helpButtonWidth = 60;
    int helpButtonHeight = deleteButtonHeight;

    if (calX >= helpButtonX && calX <= helpButtonX + helpButtonWidth && calY >= helpButtonY && calY <= helpButtonY + helpButtonHeight) {
      screenDrawn = false;
      currentState = STATE_GUIDE;
    }

    if (calX >= 5 && calX <= 5 + boxWidth && calY >= textAreaY && calY <= textAreaY + inputBoxHeight) {
      isTopBoxSelected = true;
      if (topPlaceholderActive && inputText.length() == 0) {
        topPlaceholderActive = false;
      }
    }

    if (calX >= 5 && calX <= 5 + boxWidth && calY >= outputAreaY && calY <= outputAreaY + outputBoxHeight) {
      isTopBoxSelected = false;
      if (bottomPlaceholderActive && outputText.length() == 0) {
        bottomPlaceholderActive = false;
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      if (!isTopBoxSelected) isTopBoxSelected = true;
      if (inputScrollOffset > 0) {
        inputScrollOffset--;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);

        if (inputText.length() == 0 && topPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Enter Code");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
        }
      }
    }

    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > textAreaY && calY < textAreaY + buttonHeight) {
      if (!isTopBoxSelected) isTopBoxSelected = true;
      int maxOffset = max(0, (int)inputText.length() - visibleChars);
      if (inputScrollOffset < maxOffset) {
        inputScrollOffset++;
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);

        if (inputText.length() == 0 && topPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Enter Code");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(inputText.substring(inputScrollOffset, inputScrollOffset + visibleChars));
        }
      }
    }

    if (calX > boxWidth + 10 && calX < boxWidth + 40 && calY > outputAreaY && calY < outputAreaY + buttonHeight) {
      if (isTopBoxSelected) isTopBoxSelected = false;
      if (outputScrollOffset > 0) {
        outputScrollOffset--;
        tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, outputAreaY + 8);
        tft.setTextSize(2);

        if (outputText.length() == 0 && bottomPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Decoded Message");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
        }
      }
    }

    if (calX > boxWidth + 40 && calX < boxWidth + 70 && calY > outputAreaY && calY < outputAreaY + buttonHeight) {
      if (isTopBoxSelected) isTopBoxSelected = false;
      int maxOffset = max(0, (int)outputText.length() - visibleChars);
      if (outputScrollOffset < maxOffset) {
        outputScrollOffset++;
        tft.fillRect(6, outputAreaY + 1, boxWidth - 2, outputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, outputAreaY + 8);
        tft.setTextSize(2);

        if (outputText.length() == 0 && bottomPlaceholderActive) {
          tft.setTextColor(ILI9341_GRAY);
          tft.print("Decoded Message");
        } else {
          tft.setTextColor(ILI9341_WHITE);
          tft.print(outputText.substring(outputScrollOffset, outputScrollOffset + visibleChars));
        }
      }
    }

    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;

    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
    }

    uint16_t newTopColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
    uint16_t newBottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
    tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, newTopColor);
    tft.drawRect(5, outputAreaY, boxWidth, outputBoxHeight, newBottomColor);
  }

  unsigned long now = millis();
  if (buttonPressed) {
    if (now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      String fullTime = String(letterTerminationDelay / 1000.0, 2);
      if (fullTime != lastTimerDisplay) {
        tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
        tft.setCursor(timerX, nextLetterY + 5);
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(2);
        tft.print(fullTime);
        lastTimerDisplay = fullTime;
      }
    }
  } else if (timerActive) {
    if (now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      unsigned long elapsed = now - timerStartTime;
      if (elapsed >= letterTerminationDelay) {
        timerActive = false;
        String newTimerDisplay = "0.00";
        if (newTimerDisplay != lastTimerDisplay) {
          tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
          tft.setCursor(timerX, nextLetterY + 5);
          tft.setTextColor(ILI9341_RED);
          tft.setTextSize(2);
          tft.print(newTimerDisplay);
          lastTimerDisplay = newTimerDisplay;
        }
      } else {
        float remaining = (letterTerminationDelay - elapsed) / 1000.0;
        String newTimerDisplay = String(remaining, 2);
        if (newTimerDisplay != lastTimerDisplay) {
          tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
          tft.setCursor(timerX, nextLetterY + 5);
          tft.setTextColor(ILI9341_RED);
          tft.setTextSize(2);
          tft.print(newTimerDisplay);
          lastTimerDisplay = newTimerDisplay;
        }
      }
    }
  } else {
    if (lastTimerDisplay != "0.00" && now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
      tft.setCursor(timerX, nextLetterY + 5);
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(2);
      tft.print("0.00");
      lastTimerDisplay = "0.00";
    }
  }
}

void runGameScreen() {

  static const char* fallbackWordsByLength[11][5] = {
    { "", "", "", "", "" },
    { "A", "I", "O", "U", "Y" },                                              // 1-letter words
    { "of", "to", "it", "by", "in" },                                         // 2-letter words
    { "cat", "dog", "run", "sky", "car" },                                    // 3-letter words
    { "code", "love", "hate", "cool", "fast" },                               // 4-letter words
    { "hello", "world", "quick", "brown", "smart" },                          // 5-letter words
    { "morse", "system", "device", "puzzle", "secret" },                      // 6-letter words
    { "network", "program", "display", "example", "control" },                // 7-letter words
    { "computer", "keyboard", "function", "variable", "solution" },           // 8-letter words
    { "generator", "mechanism", "framework", "procedure", "interface" },      // 9-letter words
    { "microphone", "innovation", "technology", "perfection", "expression" }  // 10-letter words
  };

  static String displayWord = "";
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  static bool timerActive = false;
  static unsigned long timerStartTime = 0;
  static String lastTimerDisplay = "";
  static int timerX = -1;
  static unsigned long lastTimerDisplayUpdate = 0;
  static bool screenDrawn = false;
  static String currentInput = "";
  static String currentDecoded = "";
  static int streak = 0;

  const int nextLetterY = tft.height() - 40;

  if (!screenDrawn) {
    resetState();
    streak = 0;
    currentInput = "";
    currentDecoded = "";
    buttonPressed = false;
    timerActive = false;
    timerStartTime = 0;
    lastTimerDisplayUpdate = 0;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    tft.fillScreen(ILI9341_BLACK);

    tft.setFont();
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(6, 9);
    tft.print("Morse Game");

    tft.fillRect(6, 45, 150, 25, ILI9341_BLACK);
    tft.setCursor(6, 45);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print("Streak: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(streak);

    tft.fillRect(tft.width() - 60, 5, 60, 25, ILI9341_RED);
    tft.drawRect(tft.width() - 60, 5, 60, 25, ILI9341_WHITE);
    tft.setCursor(tft.width() - 60 + (60 - 48) / 2, 5 + ((25 - 16) / 2) + 2);
    tft.print("EXIT");

    tft.setFont();
    tft.setTextSize(2);
    tft.setCursor(8, nextLetterY + 5);
    tft.print("Next Letter In: ");

    if (timerX == -1) {
      timerX = tft.getCursorX();
    }

    tft.setFont();
    tft.setTextSize(2);
    tft.setCursor(timerX, nextLetterY + 5);
    tft.setTextColor(ILI9341_RED);
    tft.print("0.00");

    tft.setTextSize(1);
    tft.setCursor(timerX + 50, nextLetterY + 13);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("sec(s)");
    lastTimerDisplay = "0.00";

    int randomWordLength = random(randomWordMinLength, randomWordMaxLength + 1);

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      if (client.connect("random-word-api.herokuapp.com", 80)) {
        String request = "GET /word?number=1&length=" + String(randomWordLength) + " HTTP/1.1";
        client.println(request);
        client.println("Host: random-word-api.herokuapp.com");
        client.println("Connection: close");
        client.println();
        unsigned long timeout = millis();

        while (!client.available() && millis() - timeout < 600) {
        }

        String response = "";

        while (client.available()) {
          response += (char)client.read();
        }

        int bodyIndex = response.indexOf("\r\n\r\n");
        if (bodyIndex != -1) {
          String json = response.substring(bodyIndex + 4);
          json.trim();
          if (json.startsWith("[\"")) {
            json.remove(0, 2);
            int endQuote = json.indexOf("\"");
            if (endQuote != -1) {
              displayWord = json.substring(0, endQuote);
            }
          }
        }
        client.stop();
      }
    }

    if (displayWord == "") {
      randomSeed(analogRead(0));
      int index = random(0, 5);
      displayWord = fallbackWordsByLength[randomWordLength][index];
    }

    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(2);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(displayWord, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (tft.width() - w) / 2;
    int16_t centerY = (tft.height() - h) / 2;
    tft.setCursor(centerX, centerY + 20);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(displayWord);

    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int calX = (p.y * xCalM) + xCalC;
    int calY = (p.x * yCalM) + yCalC;
    int exitButtonX = tft.width() - 60;
    int exitButtonY = 5;
    int exitButtonWidth = 60;
    int exitButtonHeight = 25;

    if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth && calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
      tft.setFont();
      tft.setTextSize(2);
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
      displayWord = "";
      return;
    }
  }

  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    pressStartTime = millis();
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, buzzerFrequency);
  }

  if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
    buttonPressed = false;
    unsigned long pressTime = millis() - pressStartTime;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);

    if (pressTime < buttonPressTimingThreshold) {
      currentInput += ".";
    } else {
      currentInput += "-";
    }

    timerActive = true;
    timerStartTime = millis();
    lastTimerDisplayUpdate = millis();
  }

  unsigned long now = millis();
  if (buttonPressed) {
    if (now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      String fullTime = String(letterTerminationDelay / 1000.0, 2);

      tft.setFont();
      tft.setTextSize(2);
      tft.setCursor(timerX, nextLetterY + 5);
      tft.setTextColor(ILI9341_RED);
      tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
      tft.print(fullTime);

      tft.setTextSize(1);
      tft.setCursor(timerX + 50, nextLetterY + 13);
      tft.setTextColor(ILI9341_WHITE);
      tft.print("sec(s)");
      lastTimerDisplay = fullTime;
    }
  } else if (timerActive) {
    if (now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      unsigned long elapsed = now - timerStartTime;
      if (elapsed >= letterTerminationDelay) {
        tft.setFont();
        tft.setTextSize(2);
        tft.setCursor(timerX, nextLetterY + 5);
        tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
        tft.setTextColor(ILI9341_RED);
        tft.print("0.00");

        tft.setTextSize(1);
        tft.setCursor(timerX + 50, nextLetterY + 13);
        tft.setTextColor(ILI9341_WHITE);
        tft.print("sec(s)");
        lastTimerDisplay = "0.00";
        timerActive = false;

        if (currentInput.length() > 0) {
          char letter = decodeSingleMorse(currentInput);
          currentDecoded += letter;
          currentInput = "";
        }

        if (currentDecoded.length() == displayWord.length()) {
          bool isCorrect = currentDecoded.equalsIgnoreCase(displayWord);
          tft.setFont(&FreeSans9pt7b);
          tft.setTextSize(2);
          int16_t x1, y1;
          uint16_t w, h;
          tft.getTextBounds(displayWord, 0, 0, &x1, &y1, &w, &h);
          int16_t centerX2 = (tft.width() - w) / 2;
          int16_t centerY2 = (tft.height() - h) / 2;
          tft.fillRect(0, centerY2 - 20, tft.width(), h + 50, ILI9341_BLACK);
          tft.setCursor(centerX2, centerY2 + 20);
          tft.setTextColor(isCorrect ? ILI9341_GREEN : ILI9341_RED);
          tft.print(displayWord);

          delay(1000);

          if (isCorrect) {
            streak++;
          } else {
            streak = 0;
          }

          tft.fillRect(6, 45, 150, 25, ILI9341_BLACK);
          tft.setFont();
          tft.setTextSize(2);
          tft.setCursor(6, 45);
          tft.setTextColor(ILI9341_YELLOW);
          tft.print("Streak: ");
          tft.setTextColor(ILI9341_WHITE);
          tft.print(streak);

          String newLetter = "";
          int randomWordLength = random(randomWordMinLength, randomWordMaxLength + 1);
          if (WiFi.status() == WL_CONNECTED) {
            WiFiClient client;
            if (client.connect("random-word-api.herokuapp.com", 80)) {
              String request = "GET /word?number=1&length=" + String(randomWordLength) + " HTTP/1.1";
              client.println(request);
              client.println("Host: random-word-api.herokuapp.com");
              client.println("Connection: close");
              client.println();
              unsigned long timeout = millis();
              while (!client.available() && millis() - timeout < 400) {
              }
              String response = "";
              while (client.available()) {
                response += (char)client.read();
              }
              int bodyIndex = response.indexOf("\r\n\r\n");
              if (bodyIndex != -1) {
                String json = response.substring(bodyIndex + 4);
                json.trim();
                if (json.startsWith("[\"")) {
                  json.remove(0, 2);
                  int endQuote = json.indexOf("\"");
                  if (endQuote != -1) {
                    newLetter = json.substring(0, endQuote);
                  }
                }
              }
              client.stop();
            }
          }

          if (newLetter == "") {
            int index = random(0, 5);
            newLetter = fallbackWordsByLength[randomWordLength][index];
          }
          displayWord = newLetter;
          tft.setFont(&FreeSans9pt7b);
          tft.setTextSize(2);
          tft.getTextBounds(displayWord, 0, 0, &x1, &y1, &w, &h);
          int16_t centerX2_new = (tft.width() - w) / 2;
          int16_t centerY2_new = (tft.height() - h) / 2;
          tft.fillRect(0, centerY2_new - 20, tft.width(), h + 50, ILI9341_BLACK);
          tft.setCursor(centerX2_new, centerY2_new + 20);
          tft.setTextColor(ILI9341_WHITE);
          tft.print(displayWord);
          currentDecoded = "";
        }
      } else {
        unsigned long remainMs = letterTerminationDelay - elapsed;
        float remaining = ((float)remainMs) / 1000.0;
        String newTimerDisplay = String(remaining, 2);
        tft.setFont();
        tft.setTextSize(2);
        tft.setCursor(timerX, nextLetterY + 5);
        tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
        tft.setTextColor(ILI9341_RED);
        tft.print(newTimerDisplay);
        tft.setTextSize(1);
        tft.setCursor(timerX + 50, nextLetterY + 13);
        tft.setTextColor(ILI9341_WHITE);
        tft.print("sec(s)");
        lastTimerDisplay = newTimerDisplay;
      }
    }
  } else {
    if (lastTimerDisplay != "0.00" && now - lastTimerDisplayUpdate >= 100) {
      lastTimerDisplayUpdate = now;
      tft.setFont();
      tft.setTextSize(2);
      tft.setCursor(timerX, nextLetterY + 5);
      tft.fillRect(timerX, nextLetterY + 5, 50, 20, ILI9341_BLACK);
      tft.setTextColor(ILI9341_RED);
      tft.print("0.00");
      tft.setTextSize(1);
      tft.setCursor(timerX + 50, nextLetterY + 13);
      tft.setTextColor(ILI9341_WHITE);
      tft.print("sec(s)");
      lastTimerDisplay = "0.00";
    }
  }
}

void runDummyState(const char* stateName) {
  if (!screenDrawn) {
    resetState();
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10);
    tft.print(stateName);
    tft.print(" Screen");
    screenDrawn = true;
  }

  TSPoint p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    screenDrawn = false;
    currentState = STATE_MAIN_MENU;
  }
}
