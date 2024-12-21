#ifndef MORSE_UTILS_H
#define MORSE_UTILS_H

#include <Arduino.h>

// Morse code lookup table
const char* morseTable[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", // A-J
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",   // K-T
  "..-", "...-", ".--", "-..-", "-.--", "--..",                        // U-Z
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----." // 0-9
};

// Function to get Morse code for a character
String charToMorse(char c) {
  if (c >= 'A' && c <= 'Z') {
    return morseTable[c - 'A']; // A-Z
  } else if (c >= 'a' && c <= 'z') {
    return morseTable[c - 'a']; // a-z
  } else if (c >= '0' && c <= '9') {
    return morseTable[c - '0' + 26]; // 0-9
  } else if (c == ' ') {
    return "/"; 
  } else {
    return ""; 
  }
}

// Convert a full string to Morse code
String textToMorse(const String& text) {

  if (text.isEmpty() || text == "") return "Invalid Text";

  String morseCode = "";

  for (size_t i = 0; i < text.length(); i++) {
    String code = charToMorse(text[i]);
    if (code.isEmpty()) {
      return "Invalid Text"; 
    }
    if (!morseCode.isEmpty()) {
      morseCode += " "; 
    }
    morseCode += code;
  }

  return morseCode;
}

// Blink Morse code on a pin with adjustable speed 
void blinkMorse(const String& morseCode, int pin, int wpm) {

  int dotDuration = 1200 / wpm; 
  int dashDuration = 3 * dotDuration;
  int symbolSpace = dotDuration;
  int letterSpace = 3 * dotDuration;
  int wordSpace = 7 * dotDuration;

  pinMode(pin, OUTPUT);
  
  for (char c : morseCode) {
    if (c == '.') {
      digitalWrite(pin, HIGH);
      delay(dotDuration);
      digitalWrite(pin, LOW);
      delay(symbolSpace);
    } else if (c == '-') {
      digitalWrite(pin, HIGH);
      delay(dashDuration);
      digitalWrite(pin, LOW);
      delay(symbolSpace);
    } else if (c == ' ') {
      delay(letterSpace); 
    } else if (c == '/') {
      delay(wordSpace); 
    }
  }
}

#endif // MORSE_UTILS_H
