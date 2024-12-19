#include <WiFiNINA.h>
#include "thingProperties.h" 
#include "config.h" 
#include "MorseUtils.h"

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);

    for (int i = 0; i < 5; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Wi-Fi Connected!");
        return; 
      }
      delay(1000); 
      Serial.print(".");
    }
    Serial.println("\nStill trying to connect to Wi-Fi...");
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();
  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  Serial.println("Connected to Arduino IoT Cloud!");

}

void loop() {
   ArduinoCloud.update();
}

void onTextMessageChange() {
  textMessage.trim();
    if (textMessage.isEmpty() || textMessage == "" || 
      textMessage.startsWith("PMC received:") || textMessage == "\x1B") {
      return;
    }

    Serial.print("Message received: ");
    Serial.println(textMessage);

    String morseCode = textToMorse(textMessage);
    Serial.print("Morse Code: ");
    Serial.println(morseCode);

    textMessage = "PMC received: \"" + textMessage + "\" | Morse: " + morseCode;

    Serial.println("Acknowledgment sent to cloud.");
}
