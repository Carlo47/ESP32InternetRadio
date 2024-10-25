#include <Arduino.h>

void heartbeat(uint8_t pin, uint8_t nBeats, uint8_t t, uint8_t duty)
{
  duty = duty < 100 ? duty : 50;
  uint32_t module = 1000 * t / nBeats;
  uint32_t ms = module * duty / 100;
  digitalWrite(pin, millis() % module < ms ? HIGH : LOW);
}