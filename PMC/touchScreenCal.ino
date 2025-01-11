// #include "SPI.h"
// #include "Adafruit_GFX.h"
// #include "Adafruit_ILI9341.h"
// #include "TouchScreen.h"

// #define TFT_CLK 13
// #define TFT_MOSI 11
// #define TFT_CS 10
// #define TFT_DC 9
// #define TFT_RST -1 

// #define YP A1
// #define XM A0
// #define YM 7
// #define XP 6

// float xCalM = 1.0, yCalM = 1.0;
// float xCalC = 0.0, yCalC = 0.0;

// int blockWidth = 20;
// int blockHeight = 20;
// int blockX = 0, blockY = 0;

// #define MINPRESSURE 10
// #define MAXPRESSURE 1000

// Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
// TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// unsigned long lastFrameTime = 0;
// const int frameInterval = 15;

// void calibrateTouchScreen() {
//   TSPoint p1, p2;

//   tft.setRotation(1); // Set the screen to 90° rotation
//   tft.fillScreen(ILI9341_BLACK);

//   // Draw first cross
//   tft.drawFastHLine(10, 20, 20, ILI9341_RED);
//   tft.drawFastVLine(20, 10, 20, ILI9341_RED);

//   while (true) {
//     TSPoint p = ts.getPoint();
//     if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
//       p1 = p;
//       break;
//     }
//   }

//   tft.fillScreen(ILI9341_BLACK);

//   // Draw second cross
//   tft.drawFastHLine(tft.width() - 30, tft.height() - 20, 20, ILI9341_RED);
//   tft.drawFastVLine(tft.width() - 20, tft.height() - 30, 20, ILI9341_RED);

//   while (true) {
//     TSPoint p = ts.getPoint();
//     if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
//       p2 = p;
//       break;
//     }
//   }

//   tft.fillScreen(ILI9341_BLACK);

//   // Calculate calibration values
//   xCalM = (float)(tft.width() - 40) / (p2.y - p1.y); // Note: X mapped to Y in setRotation(1)
//   xCalC = 20 - (p1.y * xCalM);
//   yCalM = (float)(tft.height() - 40) / (p2.x - p1.x); // Note: Y mapped to X in setRotation(1)
//   yCalC = 20 - (p1.x * yCalM);

//   Serial.print("xCalM = "); Serial.print(xCalM);
//   Serial.print(", xCalC = "); Serial.print(xCalC);
//   Serial.print(" | yCalM = "); Serial.print(yCalM);
//   Serial.print(", yCalC = "); Serial.println(yCalC);
// }

// void getCalibratedCoords(TSPoint p, int &calX, int &calY) {
//   calX = (p.y * xCalM) + xCalC; // Note: Swap X and Y for setRotation(1)
//   calY = (p.x * yCalM) + yCalC;

//   if (calX < 0) calX = 0;
//   if (calX > tft.width() - blockWidth) calX = tft.width() - blockWidth;
//   if (calY < 0) calY = 0;
//   if (calY > tft.height() - blockHeight) calY = tft.height() - blockHeight;
// }

// void moveBlock() {
//   TSPoint p = ts.getPoint();
//   if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
//     int calX, calY;
//     getCalibratedCoords(p, calX, calY);

//     if (calX != blockX || calY != blockY) {
//       tft.fillRect(blockX, blockY, blockWidth, blockHeight, ILI9341_BLACK);
//       blockX = calX;
//       blockY = calY;
//       tft.fillRect(blockX, blockY, blockWidth, blockHeight, ILI9341_RED);
//     }
//   }
// }

// void setup() {
//   Serial.begin(9600);
//   tft.begin();
//   tft.setRotation(1); // Rotate 90° clockwise
//   tft.fillScreen(ILI9341_BLACK);
//   calibrateTouchScreen();
// }

// void loop() {
//   if (millis() - lastFrameTime >= frameInterval) {
//     lastFrameTime = millis();
//     moveBlock();
//   }
// }
