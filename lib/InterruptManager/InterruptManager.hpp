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
    void setupHardwareInterrupts() {
        // Configurar pinos dos botões
        pinMode(BTN_USER, INPUT_PULLUP);     // Botão user da placa
        pinMode(BTN_EMERGENCY, INPUT_PULLUP); // Botão externo emergência
        pinMode(BTN_MODE, INPUT_PULLUP);      // Botão externo modo
        
        // Anexar interrupções
        attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY), emergencyButtonISR, FALLING);
        attachInterrupt(digitalPinToInterrupt(BTN_MODE), modeButtonISR, FALLING);
        
        logger->info("Hardware interrupts configured");
    }
    
    // Configurar interrupção por timer (software)
    void setupTimerInterrupt() {
        // STM32L476 tem vários timers, usar TIM2
        timer2 = new HardwareTimer(TIM2);
        
        // Configurar para 1Hz (1 segundo)
        timer2->setOverflow(1, HERTZ_FORMAT);
        timer2->attachInterrupt(timerISR);
        timer2->resume();
        
        logger->info("Timer interrupt configured (1Hz)");
    }
    
    // Processar flags das interrupções
    void processInterrupts(SystemControl* control) {
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