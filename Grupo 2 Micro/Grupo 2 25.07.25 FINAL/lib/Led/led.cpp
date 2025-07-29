#include "led.hpp"
#include <Arduino.h>
#include "config.hpp"

Led::Led()
{
}

void Led::iniciar()
{
    pinMode(LEDGREEN, OUTPUT);
    digitalWrite(LEDGREEN, HIGH); // Liga o LED verde ao iniciar
}

void Led::executar()
{
    static unsigned long lastToggle = 0;
    static bool ledState = true;

    if (config_data.S1 > 30.0)
    {
        unsigned long now = millis();
        if (now - lastToggle >= 500)
        {
            ledState = !ledState;
            digitalWrite(LEDGREEN, ledState ? HIGH : LOW);
            lastToggle = now;
        }
    }
    else
    {
        digitalWrite(LEDGREEN, HIGH);
        ledState = true;
    }
}