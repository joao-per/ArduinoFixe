#ifndef GPI_HPP
#define GPI_HPP

#include <Arduino.h>

class GPI {
  public:
    GPI();
    void init(uint8_t pin);
    void on();
    void off();
    void toggle();
    void blink(unsigned long interval);
    void buttonControl(uint8_t buttonPin);
    bool isOn();
    void setState(bool state);  // Novo método
    
  private:
    uint8_t _pin;
    bool _state;
    unsigned long _lastBlink;
    bool _blinkState;
};

// Implementação inline do novo método
inline void GPI::setState(bool state) {
    _state = state;
    digitalWrite(_pin, state ? HIGH : LOW);
}

#endif // GPI_HPP