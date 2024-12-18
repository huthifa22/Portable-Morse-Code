#include <WiFiNINA.h>
#include "thingProperties.h" 
#include "config.h" 
#include "MorseUtils.h"

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS); 
  while (WiFi.status() != WL_CONNECTED);
  Serial.println("Wi-Fi Connected!");

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  Serial.println("Connected to Arduino IoT Cloud!");

}

void loop() {
   ArduinoCloud.update();
}

void onTextMessageChange() {
    Serial.print("Message received: ");
    Serial.println(textMessage);

    String morseCode = textToMorse(textMessage);
    Serial.print("Morse Code: ");
    Serial.println(morseCode);

    textMessage = "PMC received: \"" + textMessage + "\" | Morse: " + morseCode;

    Serial.println("Acknowledgment sent to cloud.");
}
