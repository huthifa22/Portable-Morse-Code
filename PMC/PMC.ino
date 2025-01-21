#include <WiFiNINA.h>
#include "thingProperties.h"
#include "MorseUtils.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"
#include <vector>

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

char wifiSSID[32] = "";   
char wifiPassword[32] = ""; 
String textMessage = "";
std::vector<String> messageQueue;
int currentMessageIndex = -1; 

int WPM = 20;

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
  STATE_TOOLS,
  STATE_OTHER,
  STATE_INFO
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
      runEncodeState();
      break;

    case STATE_DECODE:
      runDummyState("Decode");
      break;

    case STATE_TOOLS:
      runDummyState("Tools");
      break;

    case STATE_OTHER:
      runDummyState("Other");
      break;

    case STATE_INFO:
      runDummyState("Info");
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
    tft.setTextSize(8);
    tft.setTextColor(tft.color565(139, 0, 0));

    const char* text = "PMC";
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int16_t centerX = (tft.width() - w) / 2;
    int16_t centerY = (tft.height() - h) / 2;

    tft.setCursor(centerX, centerY);
    tft.print(text);

    screenDrawn = true;
  }

  screenDrawn = false;
  delay(1000);
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
    if (calX > tft.width() / 4 - 50 && calX < tft.width() / 4 + 50 &&
        calY > (tft.height() / 4) + 50 && calY < (tft.height() / 4) + 100) {
      screenDrawn = false;
      currentState = STATE_WIFI_CREDENTIALS;
    }

    // Checking "No" button
    if (calX > (tft.width() * 3 / 4) - 50 && calX < (tft.width() * 3 / 4) + 50 &&
        calY > (tft.height() / 4) + 50 && calY < (tft.height() / 4) + 100) {
      screenDrawn = false;
      currentState = STATE_MAIN_MENU;
    }
  }
}

void runWiFiCredentialsScreen() {
    static String selectedBox = "SSID";
    static String ssid = "test";
    static String password = "test";
    static int ssidScrollOffset = 0;
    static int passwordScrollOffset = 0;

    if (!screenDrawn) {
        tft.fillScreen(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_WHITE);

        tft.setCursor(10, 10);
        tft.print("Wi-Fi Name:");
        tft.drawRect(150, 10, tft.width() - 160, 30, (selectedBox == "SSID") ? ILI9341_GREEN : ILI9341_WHITE);

        tft.setCursor(10, 50);
        tft.print("Password:");
        tft.drawRect(150, 50, tft.width() - 160, 30, (selectedBox == "Password") ? ILI9341_GREEN : ILI9341_WHITE);

        int maxSSIDChars = (tft.width() - 160) / 12;
        ssidScrollOffset = max(0, (int)ssid.length() - maxSSIDChars);
        tft.setCursor(155, 15);
        tft.print(ssid.substring(ssidScrollOffset));

        int maxPasswordChars = (tft.width() - 160) / 12;
        passwordScrollOffset = max(0, (int)password.length() - maxPasswordChars);
        tft.setCursor(155, 55);
        for (int i = passwordScrollOffset; i < password.length(); ++i) {
            tft.print("*");
        }

        // "EXIT" button
        int exitButtonX = 5;
        int exitButtonY = 80;
        int exitButtonWidth = 60;
        int exitButtonHeight = 25;
        int textWidth = 12 * 4;
        int textHeight = 16;
        int16_t textX = exitButtonX + (exitButtonWidth - textWidth) / 2;
        int16_t textY = exitButtonY + (exitButtonHeight - textHeight) / 2 + 2;

        tft.fillRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_RED);
        tft.setCursor(textX, textY);
        tft.setTextColor(ILI9341_WHITE);
        tft.print("EXIT");
        tft.drawRect(exitButtonX, exitButtonY, exitButtonWidth, exitButtonHeight, ILI9341_WHITE);
        drawKeyboard();
        screenDrawn = true;
    }

    TSPoint p = ts.getPoint();
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        int calX = (p.y * xCalM) + xCalC;
        int calY = (p.x * yCalM) + yCalC;

        char key = getKeyFromCoords(calX, calY);
        if (key) {
            if (key == '<') {
                if (selectedBox == "SSID" && ssid.length() > 0) {
                    ssid.remove(ssid.length() - 1);
                    ssidScrollOffset = max(0, (int)ssid.length() - 13);
                } else if (selectedBox == "Password" && password.length() > 0) {
                    password.remove(password.length() - 1);
                    passwordScrollOffset = max(0, (int)password.length() - 13);
                }
            } else if (key == '>') {
                strcpy(wifiSSID, ssid.c_str());
                strcpy(wifiPassword, password.c_str());
                currentState = STATE_CONNECTION;
                screenDrawn = false;
            } else {
                if (selectedBox == "SSID" && ssid.length() < 32) {
                    ssid += key;
                    ssidScrollOffset = max(0, (int)ssid.length() - 13);
                } else if (selectedBox == "Password" && password.length() < 32) {
                    password += key;
                    passwordScrollOffset = max(0, (int)password.length() - 13);
                }
            }

            delay(80);

            tft.drawRect(150, 10, tft.width() - 160, 30, (selectedBox == "SSID") ? ILI9341_GREEN : ILI9341_WHITE);
            tft.drawRect(150, 50, tft.width() - 160, 30, (selectedBox == "Password") ? ILI9341_GREEN : ILI9341_WHITE);

            tft.fillRect(151, 11, tft.width() - 162, 28, ILI9341_BLACK);
            tft.setCursor(155, 15);
            tft.print(ssid.substring(ssidScrollOffset));

            tft.fillRect(151, 51, tft.width() - 162, 28, ILI9341_BLACK);
            tft.setCursor(155, 55);
            for (int i = passwordScrollOffset; i < password.length(); ++i) {
                tft.print("*");
            }
        }

        if (calX > 150 && calX < tft.width() - 10 && calY > 10 && calY < 40) {
            if (selectedBox != "SSID") {
                selectedBox = "SSID";
                tft.drawRect(150, 10, tft.width() - 160, 30, ILI9341_GREEN);
                tft.drawRect(150, 50, tft.width() - 160, 30, ILI9341_WHITE);
            }
        }

        if (calX > 150 && calX < tft.width() - 10 && calY > 50 && calY < 80) {
            if (selectedBox != "Password") {
                selectedBox = "Password";
                tft.drawRect(150, 50, tft.width() - 160, 30, ILI9341_GREEN);
                tft.drawRect(150, 10, tft.width() - 160, 30, ILI9341_WHITE);
            }
        }

        // Check if "EXIT" button is pressed
        int exitButtonX = 10;
        int exitButtonY = 90;
        int exitButtonWidth = 100;
        int exitButtonHeight = 40;

        if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth &&
            calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
            screenDrawn = false;
            currentState = STATE_MAIN_MENU;
        }
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
                keys = "!@#$%^&*()_+-=[]{}|;:'\",.<>?/\\`~";
                break;
        }
    }

    int xStart = 5, yStart = tft.height() - (5 + 4 * (keyHeight + keySpacing));
    int index = 0;

    tft.fillRect(xStart, yStart, tft.width(), tft.height() - yStart, ILI9341_BLACK);

    // Draw keys
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < keysPerRow && index < strlen(keys); ++col) {
            int x = xStart + col * (keyWidth + keySpacing);
            int y = yStart + row * (keyHeight + keySpacing);

            tft.fillRect(x, y, keyWidth, keyHeight, ILI9341_BLUE);
            tft.setCursor(x + 5, y + 5);
            tft.setTextColor(ILI9341_WHITE);
            tft.setTextSize(2);
            tft.print(keys[index]);
            ++index;
        }
    }

    // Draw space button
    int spaceX = xStart + 6 * (keyWidth + keySpacing);
    int spaceY = yStart + 3 * (keyHeight + keySpacing);
    tft.fillRect(spaceX, spaceY, keyWidth, keyHeight, ILI9341_GRAY);

    // Draw symbol toggle button; U = Uppercase, L = Lowercase, S = Symbol
    if (currentState != STATE_ENCODE) {
    int symbolX = xStart + 7 * (keyWidth + keySpacing);
    tft.fillRect(symbolX, spaceY, keyWidth, keyHeight, ILI9341_YELLOW);
    tft.setCursor(symbolX + 5, spaceY + 5);
    if (currentKeyboardState == STATE_UPPERCASE) {
        tft.print("U");
    } else if (currentKeyboardState == STATE_LOWERCASE) {
        tft.print("L");
    } else {
        tft.print("S");
    }
    }

    // Draw backspace button
    int backspaceX = xStart + 8 * (keyWidth + keySpacing);
    tft.fillRect(backspaceX, spaceY, keyWidth, keyHeight, ILI9341_RED);
    tft.setCursor(backspaceX + 5, spaceY + 5);
    tft.print("<");

    // Draw enter button
    int enterX = xStart + 9 * (keyWidth + keySpacing);
    tft.fillRect(enterX, spaceY, keyWidth, keyHeight, ILI9341_GREEN);
    tft.setCursor(enterX + 5, spaceY + 5);
    tft.print(">");
}

char getKeyFromCoords(int calX, int calY) {
    int xStart = 5, yStart = tft.height() - (5 + 4 * (keyHeight + keySpacing));

    const char* keys;
    switch (currentKeyboardState) {
        case STATE_UPPERCASE:
            keys = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
            break;
        case STATE_LOWERCASE:
            keys = "1234567890qwertyuiopasdfghjklzxcvbnm";
            break;
        case STATE_SYMBOLS:
            keys = "!@#$%^&*()_+-=[]{}|;:'\",.<>?/\\`~";
            break;
    }

    // Detect standard keys
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < keysPerRow && row * keysPerRow + col < strlen(keys); ++col) {
            int x = xStart + col * (keyWidth + keySpacing);
            int y = yStart + row * (keyHeight + keySpacing);

            if (calX > x && calX < x + keyWidth && calY > y && calY < y + keyHeight) {
                return keys[row * keysPerRow + col];
            }
        }
    }

    // Detect spacebar
    int spaceX = xStart + 6 * (keyWidth + keySpacing);
    int spaceY = yStart + 3 * (keyHeight + keySpacing);
    if (calX > spaceX && calX < spaceX + keyWidth && calY > spaceY && calY < spaceY + keyHeight) {
        return ' ';
    }

    // Detect symbol toggle
    int symbolX = xStart + 7 * (keyWidth + keySpacing);
    if (calX > symbolX && calX < symbolX + keyWidth && calY > spaceY && calY < spaceY + keyHeight) {
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

    // Detect backspace
    int backspaceX = xStart + 8 * (keyWidth + keySpacing);
    if (calX > backspaceX && calX < backspaceX + keyWidth && calY > spaceY && calY < spaceY + keyHeight) {
        return '<';
    }

    // Detect enter
    int enterX = xStart + 9 * (keyWidth + keySpacing);
    if (calX > enterX && calX < enterX + keyWidth && calY > spaceY && calY < spaceY + keyHeight) {
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
        const char* labels[] = {"Guide", "Encode", "Decode", "Tools", "Other", "Info"};

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

        if (calX > mailButtonX && calX < mailButtonX + mailButtonWidth &&
            calY > mailButtonY && calY < mailButtonY + mailButtonHeight) {
            screenDrawn = false;
            currentState = STATE_MAIL;
        }

        if (calX > wifiButtonX && calX < wifiButtonX + wifiButtonWidth &&
            calY > wifiButtonY && calY < wifiButtonY + wifiButtonHeight) {
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
                    case 3: currentState = STATE_TOOLS; break;
                    case 4: currentState = STATE_OTHER; break;
                    case 5: currentState = STATE_INFO; break;
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
  if (textMessage.isEmpty() || textMessage == "" || textMessage == "Message Denied." ||
      textMessage.startsWith("PMC received:") || textMessage == "\x1B") {
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

    if (currentMessageIndex < 0) {
        currentMessageIndex = 0;
    }

    if (!screenDrawn) {
        tft.fillScreen(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(10, 10);
        tft.print("Message:");
        tft.setCursor(10, 40);
        tft.print(messageQueue[currentMessageIndex]);

        int buttonWidth = 100;
        int buttonHeight = 40;
        int topButtonY = tft.height() - 120;
        int bottomButtonY = tft.height() - 60;

        int acceptX = 10;
        int denyX = tft.width() - (buttonWidth + 10);
        int prevX = 10;
        int nextX = tft.width() - (buttonWidth + 10);

        tft.fillRect(acceptX, topButtonY, buttonWidth, buttonHeight, ILI9341_GREEN);
        tft.setCursor(acceptX + 20, topButtonY + 10);
        tft.setTextColor(ILI9341_BLACK);
        tft.print("Accept");

        tft.fillRect(denyX, topButtonY, buttonWidth, buttonHeight, ILI9341_RED);
        tft.setCursor(denyX + 20, topButtonY + 10);
        tft.print("Deny");

        if (messageQueue.size() > 1) {
            tft.fillRect(prevX, bottomButtonY, buttonWidth, buttonHeight, ILI9341_BLUE);
            tft.setCursor(prevX + 20, bottomButtonY + 10);
            tft.print("Prev");

            tft.fillRect(nextX, bottomButtonY, buttonWidth, buttonHeight, ILI9341_BLUE);
            tft.setCursor(nextX + 20, bottomButtonY + 10);
            tft.print("Next");
        }

        screenDrawn = true;
    }

    TSPoint p = ts.getPoint();
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        int calX = (p.y * xCalM) + xCalC;
        int calY = (p.x * yCalM) + yCalC;

        int buttonWidth = 100;
        int buttonHeight = 40;
        int topButtonY = tft.height() - 120;
        int bottomButtonY = tft.height() - 60;

        int acceptX = 10;
        int denyX = tft.width() - (buttonWidth + 10);
        int prevX = 10;
        int nextX = tft.width() - (buttonWidth + 10);

        if (calX > acceptX && calX < acceptX + buttonWidth &&
            calY > topButtonY && calY < topButtonY + buttonHeight) {
            String morseCode = textToMorse(messageQueue[currentMessageIndex]);

            //Light Morse Code on Pin D2
            blinkMorse(morseCode, 2, WPM);
            Serial.println("Acknowledgment sent to cloud.");
            textMessage = "PMC received: \"" + messageQueue[currentMessageIndex] + "\" | Morse: " + morseCode;
            ArduinoCloud.update();
            messageQueue.erase(messageQueue.begin() + currentMessageIndex);

            if (currentMessageIndex >= messageQueue.size()) {
                currentMessageIndex--;
            }
            screenDrawn = false;
            if (messageQueue.empty()) {
                currentState = STATE_MAIN_MENU;
            }
        }

        if (calX > denyX && calX < denyX + buttonWidth &&
            calY > topButtonY && calY < topButtonY + buttonHeight) {
            Serial.println("Message Denied");
            textMessage = "Message Denied.";
            ArduinoCloud.update();
            messageQueue.erase(messageQueue.begin() + currentMessageIndex);
            if (currentMessageIndex >= messageQueue.size()) {
                currentMessageIndex--;
            }
            screenDrawn = false;
            if (messageQueue.empty()) {
                currentState = STATE_MAIN_MENU;
            }
        }

        if (messageQueue.size() > 1 && calX > prevX && calX < prevX + buttonWidth &&
            calY > bottomButtonY && calY < bottomButtonY + buttonHeight) {
            if (currentMessageIndex > 0) {
                currentMessageIndex--;
                screenDrawn = false;
            }
        }

        if (messageQueue.size() > 1 && calX > nextX && calX < nextX + buttonWidth &&
            calY > bottomButtonY && calY < bottomButtonY + buttonHeight) {
            if (currentMessageIndex < messageQueue.size() - 1) {
                currentMessageIndex++;
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

            if (calX > okButtonX && calX < okButtonX + okButtonWidth &&
                calY > okButtonY && calY < okButtonY + okButtonHeight) {
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
            int colX[] = {10, colWidth + 10, 2 * colWidth + 10};
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
            int colX[] = {10, colWidth + 10};
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
        if (calX > toggleButtonX && calX < toggleButtonX + toggleButtonWidth &&
            calY > toggleButtonY && calY < toggleButtonY + toggleButtonHeight) {
            screenDrawn = false;
            isShowingNumbers = !isShowingNumbers;
            return;
        }

        int exitButtonX = tft.width() - 110;
        int exitButtonY = tft.height() - 40;
        int exitButtonWidth = 100;
        int exitButtonHeight = 30;
        if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth &&
            calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
            screenDrawn = false;
            currentState = STATE_MAIN_MENU;
            return;
        }
    }
}

void runEncodeState() {
    static String inputText = "";
    static String morseText = "";
    const int inputBoxHeight = 30;
    const int outputBoxHeight = 30;
    const int textAreaY = 10;
    const int morseAreaY = textAreaY + inputBoxHeight + 10;
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
        tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
        tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
        tft.setCursor(10, textAreaY + 8);
        tft.setTextSize(2);
        if (topPlaceholderActive && inputText.length() == 0) {
            tft.setTextColor(ILI9341_LIGHTGREY);
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
            tft.setTextColor(ILI9341_LIGHTGREY);
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
        int exitButtonX = 5;
        int exitButtonY = 85;
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
                morseText = textToMorse(inputText);
                blinkMorse(morseText, 2, WPM);
                isTopBoxSelected = false;
                if (bottomPlaceholderActive && morseText.length() > 0) {
                    bottomPlaceholderActive = false;
                }
            } else if (isTopBoxSelected) {
                if (topPlaceholderActive) {
                    topPlaceholderActive = false;
                    inputText = "";
                }
                if (key == '<') {
                    if (!inputText.isEmpty()) {
                        inputText.remove(inputText.length() - 1);
                    }
                } else if (inputText.length() < maxInputChars) {
                    inputText += key;
                }
                morseText = textToMorse(inputText);
            }
            inputScrollOffset = max(0, (int)inputText.length() - visibleChars);
            tft.fillRect(6, textAreaY + 1, boxWidth - 2, inputBoxHeight - 2, ILI9341_BLACK);
            tft.setCursor(10, textAreaY + 8);
            tft.setTextSize(2);
            if (topPlaceholderActive && inputText.length() == 0) {
                tft.setTextColor(ILI9341_LIGHTGREY);
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
                tft.setTextColor(ILI9341_LIGHTGREY);
                tft.print("Encoded Message");
            } else {
                tft.setTextColor(ILI9341_WHITE);
                tft.print(morseText.substring(morseScrollOffset, morseScrollOffset + visibleChars));
            }
        }
        int exitButtonX = 5;
        int exitButtonY = 85;
        int exitButtonWidth = 60;
        int exitButtonHeight = 25;
        if (calX > exitButtonX && calX < exitButtonX + exitButtonWidth &&
            calY > exitButtonY && calY < exitButtonY + exitButtonHeight) {
            screenDrawn = false;
            currentState = STATE_MAIN_MENU;
        }
        uint16_t topColor = isTopBoxSelected ? ILI9341_GREEN : ILI9341_WHITE;
        uint16_t bottomColor = isTopBoxSelected ? ILI9341_WHITE : ILI9341_GREEN;
        tft.drawRect(5, textAreaY, boxWidth, inputBoxHeight, topColor);
        tft.drawRect(5, morseAreaY, boxWidth, outputBoxHeight, bottomColor);
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
