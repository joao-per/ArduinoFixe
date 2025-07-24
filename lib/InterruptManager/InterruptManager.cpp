#include "InterruptManager.hpp"
#include "logs.hpp"
#include "SystemControl.hpp"

// Global variable definitions
volatile bool emergencyButtonPressed = false;
volatile bool modeButtonPressed = false;
volatile bool timerFlag = false;
volatile unsigned long lastEmergencyPress = 0;
volatile unsigned long lastModePress = 0;

// Timer handler
HardwareTimer* timer2 = NULL;

// ISR implementations
void emergencyButtonISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastEmergencyPress > BUTTON_DEBOUNCE_DELAY) {
        emergencyButtonPressed = true;
        lastEmergencyPress = currentTime;
    }
}

void modeButtonISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastModePress > BUTTON_DEBOUNCE_DELAY) {
        modeButtonPressed = true;
        lastModePress = currentTime;
    }
}

void timerISR() {
    timerFlag = true;
}