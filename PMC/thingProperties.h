#pragma once

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

void onTextMessageChange();

extern String textMessage;

inline void initProperties() {
  ArduinoCloud.addProperty(textMessage, READWRITE, ON_CHANGE, onTextMessageChange);
}
