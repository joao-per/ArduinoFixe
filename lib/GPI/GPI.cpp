#include "GPI.hpp"

GPI::GPI() {}

void GPI::init(uint8_t pin)
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _state = false;
}

void GPI::on()
{
  _state = true;
  digitalWrite(_pin, HIGH);
}

void GPI::off()
{
  _state = false;
  digitalWrite(_pin, LOW);
}

void GPI::toggle()
{
  _state = !_state;
  digitalWrite(_pin, _state ? HIGH : LOW);
}

void GPI::blink(unsigned long interval)
{
  unsigned long now = millis();
  if (now - _lastBlink >= interval)
  {
    _lastBlink = now;
    _blinkState = !_blinkState;
    digitalWrite(_pin, _blinkState ? HIGH : LOW);
  }
}

void GPI::buttonControl(uint8_t buttonPin)
{
  pinMode(buttonPin, INPUT_PULLUP);
  if (digitalRead(buttonPin) == LOW)
  {
    toggle();
    delay(300); // debounce simples
  }
}

bool GPI::isOn()
{
  return _state;
}