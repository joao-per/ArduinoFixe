#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>

class LED {
private:
    int pin;
    bool state;
    
public:
    LED();
    void init(int ledPin);
    void setState(bool newState);
    void on();
    void off();
    void toggle();
    void blink(int duration = 500);
    bool getState();
};

#endif // LED_HPP