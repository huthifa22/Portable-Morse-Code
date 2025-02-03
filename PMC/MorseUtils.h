#ifndef MORSE_UTILS_H
#define MORSE_UTILS_H

#include <Arduino.h>

// Morse code lookup table
const char* morseTable[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",                    // A-J
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",                      // K-T
  "..-", "...-", ".--", "-..-", "-.--", "--..",                                             // U-Z
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."  // 0-9
};

// Function to get Char for single Morse code
char decodeSingleMorse(const String& code) {
  for (int i = 0; i < 36; i++) {
    if (code == morseTable[i]) {
      if (i < 26) {
        return (char)('A' + i);
      } else {
        return (char)('0' + (i - 26));
      }
    }
  }
  return '?';
}

// Function to get Morse code for a character
String charToMorse(char c) {
  if (c >= 'A' && c <= 'Z') {
    return morseTable[c - 'A'];  // A-Z
  } else if (c >= 'a' && c <= 'z') {
    return morseTable[c - 'a'];  // a-z
  } else if (c >= '0' && c <= '9') {
    return morseTable[c - '0' + 26];  // 0-9
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

// Convert Morse Code to Text
String morseToText(int pinNumber, const String& message, int timeGap) {
  String result;
  String current;
  pinMode(pinNumber, INPUT_PULLUP);
  
  for (int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);
    if (c == '.' || c == '-') {
      current += c;
    } else if (c == ' ') {
      if (!current.isEmpty()) {
        result += decodeSingleMorse(current);
        current = "";
      }
    } else if (c == '/') {
      if (!current.isEmpty()) {
        result += decodeSingleMorse(current);
        current = "";
      }
      result += " ";
    }
  }
  if (!current.isEmpty()) {
    result += decodeSingleMorse(current);
  }
  return result;
}

// Blink and Hear Morse code on pins with adjustable speed and frequency
void audioAndLightMorse(const String& morseCode, int audioPin, int lightPin, int wpm, int frequency) {
  int dotDuration = 1200 / wpm;
  int dashDuration = 3 * dotDuration;
  int symbolSpace = dotDuration;
  int letterSpace = 3 * dotDuration;
  int wordSpace = 7 * dotDuration;

  pinMode(audioPin, OUTPUT);
  pinMode(lightPin, OUTPUT);

  for (char c : morseCode) {
    if (c == '.') {
      tone(audioPin, frequency, dotDuration);
      digitalWrite(lightPin, HIGH);
      delay(dotDuration);
      digitalWrite(lightPin, LOW);
      delay(symbolSpace);
    } else if (c == '-') {
      tone(audioPin, frequency, dashDuration);
      digitalWrite(lightPin, HIGH);
      delay(dashDuration);
      digitalWrite(lightPin, LOW);
      delay(symbolSpace);
    } else if (c == ' ') {
      delay(letterSpace);
    } else if (c == '/') {
      delay(wordSpace);
    }
  }
}

#endif  // MORSE_UTILS_H
