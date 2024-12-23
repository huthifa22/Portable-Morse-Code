// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "config.h"

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_OPTIONAL_PASS;    // Network password (use for WPA, or use as key for WEP)

void onTextMessageChange();

String textMessage;

void initProperties(){

  ArduinoCloud.addProperty(textMessage, READWRITE, ON_CHANGE, onTextMessageChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
