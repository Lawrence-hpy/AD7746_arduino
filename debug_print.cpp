#include <Arduino.h>

extern "C" void debug_print(const char* msg) {
  Serial.println(msg);
}

extern "C" void debug_print_hex(uint32_t val) {
  Serial.print("0x");
  Serial.println(val, HEX);
}