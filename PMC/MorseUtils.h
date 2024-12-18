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
    return "/"; // Space between words
  } else {
    return ""; // Unsupported character
  }
}

// Convert a full string to Morse code
String textToMorse(const String& text) {
  String morseCode = "";
  for (size_t i = 0; i < text.length(); i++) {
    String code = charToMorse(text[i]);
    if (!code.isEmpty()) {
      if (!morseCode.isEmpty()) {
        morseCode += " "; 
      }
      morseCode += code;
    }
  }
  return morseCode;
}

#endif // MORSE_UTILS_H
