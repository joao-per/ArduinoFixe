#include "LED.hpp"

LED::LED() : pin(-1), state(false) {
}

void LED::init(int ledPin) {
    pin = ledPin;
    pinMode(pin, OUTPUT);
    setState(false);
}

void LED::setState(bool newState) {
    if (pin < 0) return;
    state = newState;
    digitalWrite(pin, state ? HIGH : LOW);
}

void LED::on() {
    setState(true);
}

void LED::off() {
    setState(false);
}

void LED::toggle() {
    setState(!state);
}

void LED::blink(int duration) {
    on();
    delay(duration);
    off();
    delay(duration);
}

bool LED::getState() {
    return state;
}