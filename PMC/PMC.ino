#define BLYNK_TEMPLATE_ID "TMPL24b5q4mCt"
#define BLYNK_TEMPLATE_NAME "PMC"

#include "config.h"  
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED);
  Serial.println("Wi-Fi Connected!");

  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Connected to Blynk!");
}

void loop() {
  Blynk.run();
}
