#ifndef INTERRUPT_MANAGER_HPP
#define INTERRUPT_MANAGER_HPP

#include <Arduino.h>
#include <config.hpp>

// Forward declaration
class ExtMEM;
class SystemControl;

// Variáveis globais para ISR (devem ser volatile)
extern volatile bool emergencyButtonPressed;
extern volatile bool modeButtonPressed;
extern volatile bool timerFlag;
extern volatile unsigned long lastEmergencyPress;
extern volatile unsigned long lastModePress;

// Timer handler
extern HardwareTimer* timer2;

// Function declarations for ISRs
void emergencyButtonISR();
void modeButtonISR();
void timerISR();

class InterruptManager {
private:
    ExtMEM* logger;
    
public:
    InterruptManager(ExtMEM* log) : logger(log) {}
    
    // Configurar interrupções de hardware (botões)
    void setupHardwareInterrupts();
    
    // Configurar interrupção por timer (software)
    void setupTimerInterrupt();
    
    // Processar flags das interrupções
    void processInterrupts(SystemControl* control);
    
    // Verificar se o timer disparou
    bool isTimerTriggered() {
        if (timerFlag) {
            timerFlag = false;
            return true;
        }
        return false;
    }
    
    // Desabilitar todas as interrupções (para operações críticas)
    void disableAll() {
        detachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY));
        detachInterrupt(digitalPinToInterrupt(BTN_MODE));
        if (timer2) timer2->pause();
    }
    
    // Reabilitar todas as interrupções
    void enableAll() {
        attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY), emergencyButtonISR, FALLING);
        attachInterrupt(digitalPinToInterrupt(BTN_MODE), modeButtonISR, FALLING);
        if (timer2) timer2->resume();
    }
};

#endif // INTERRUPT_MANAGER_HPP