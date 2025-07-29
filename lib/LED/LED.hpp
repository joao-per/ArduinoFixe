#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>

class LED {
private:
    int pin;
    bool state;
    
public:
    LED() : pin(-1), state(false) {}
    
    void init(int ledPin) {
        pin = ledPin;
        pinMode(pin, OUTPUT);
        setState(false);
    }
    
    void setState(bool newState) {
        if (pin < 0) return;
        state = newState;
        digitalWrite(pin, state ? HIGH : LOW);
    }
    
    void on() {
        setState(true);
    }
    
    void off() {
        setState(false);
    }
    
    void toggle() {
        setState(!state);
    }
    
    void blink(int duration = 500) {
        on();
        delay(duration);
        off();
        delay(duration);
    }
    
    bool getState() {
        return state;
    }
};

#endif // LED_HPP