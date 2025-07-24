#include "InterruptManager.hpp"
#include "SystemControl.hpp"
#include "logs.hpp"

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

// InterruptManager method implementations
void InterruptManager::setupHardwareInterrupts() {
    // Configurar pinos dos botões
    pinMode(BTN_USER, INPUT_PULLUP);     // Botão user da placa
    pinMode(BTN_EMERGENCY, INPUT_PULLUP); // Botão externo emergência
    pinMode(BTN_MODE, INPUT_PULLUP);      // Botão externo modo
    
    // Anexar interrupções
    attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY), emergencyButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_MODE), modeButtonISR, FALLING);
    
    logger->info("Hardware interrupts configured");
}

void InterruptManager::setupTimerInterrupt() {
    // STM32L476 tem vários timers, usar TIM2
    timer2 = new HardwareTimer(TIM2);
    
    // Configurar para 1Hz (1 segundo)
    timer2->setOverflow(1, HERTZ_FORMAT);
    timer2->attachInterrupt(timerISR);
    timer2->resume();
    
    logger->info("Timer interrupt configured (1Hz)");
}

void InterruptManager::processInterrupts(SystemControl* control) {
    // Processar botão de emergência
    if (emergencyButtonPressed) {
        emergencyButtonPressed = false;
        control->toggleEmergency();
    }
    
    // Processar botão de modo
    if (modeButtonPressed) {
        modeButtonPressed = false;
        control->toggleMode();
    }
    
    // Flag do timer é processada no loop principal
}