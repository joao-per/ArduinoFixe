#ifndef SYSTEM_CONTROL_HPP
#define SYSTEM_CONTROL_HPP

#include <Arduino.h>
#include <config.hpp>
#include "logs.hpp"

enum SystemMode {
    MODE_MANUAL,
    MODE_AUTOMATIC
};

class SystemControl {
private:
    SystemMode currentMode;
    bool emergencyState;
    float tempMin;
    float tempMax;
    bool actuatorsState;
    ExtMEM* logger;
    ExtMEM* csvLogger;
    unsigned long lastSaveTime;
    
public:
    SystemControl(ExtMEM* log, ExtMEM* csv) 
        : logger(log), csvLogger(csv) {
        currentMode = MODE_AUTOMATIC;
        emergencyState = false;
        tempMin = DEFAULT_TEMP_MIN;
        tempMax = DEFAULT_TEMP_MAX;
        actuatorsState = false;
        lastSaveTime = 0;
    }
    
    // Getters
    SystemMode getMode() { return currentMode; }
    bool isEmergency() { return emergencyState; }
    float getTempMin() { return tempMin; }
    float getTempMax() { return tempMax; }
    bool getActuatorsState() { return actuatorsState; }
    
    // Setters
    void setMode(SystemMode mode) {
        currentMode = mode;
        logger->info(mode == MODE_AUTOMATIC ? "Mode changed to AUTOMATIC" : "Mode changed to MANUAL");
    }
    
    void toggleMode() {
        setMode(currentMode == MODE_AUTOMATIC ? MODE_MANUAL : MODE_AUTOMATIC);
    }
    
    void setEmergency(bool state) {
        emergencyState = state;
        if (state) {
            logger->warning("EMERGENCY ACTIVATED!");
            actuatorsState = false; // Desligar tudo em emergência
        } else {
            logger->info("Emergency deactivated");
        }
    }
    
    void toggleEmergency() {
        setEmergency(!emergencyState);
    }
    
    void setTempMin(float temp) {
        tempMin = constrain(temp, 10.0, 30.0);
        char msg[50];
        sprintf(msg, "Temperature MIN set to: %.1f°C", tempMin);
        logger->info(msg);
    }
    
    void setTempMax(float temp) {
        tempMax = constrain(temp, 30.0, 60.0);
        char msg[50];
        sprintf(msg, "Temperature MAX set to: %.1f°C", tempMax);
        logger->info(msg);
    }
    
    // Controlo automático baseado na temperatura
    void processControl(float currentTemp) {
        if (emergencyState) {
            actuatorsState = false;
            return;
        }
        
        if (currentMode == MODE_AUTOMATIC) {
            bool previousState = actuatorsState;
            
            if (currentTemp >= tempMax) {
                actuatorsState = true;
                if (!previousState) {
                    logger->warning("Temperature HIGH! Activating ventilators");
                    saveActuatorStateToCSV("ON");
                }
            } else if (currentTemp <= tempMin) {
                actuatorsState = false;
                if (previousState) {
                    logger->info("Temperature normal. Deactivating ventilators");
                    saveActuatorStateToCSV("OFF");
                }
            }
        }
    }
    
    // Controlo manual dos atuadores
    void setActuators(bool state) {
        if (currentMode == MODE_MANUAL && !emergencyState) {
            actuatorsState = state;
            logger->info(state ? "Actuators manually ON" : "Actuators manually OFF");
            saveActuatorStateToCSV(state ? "ON" : "OFF");
        }
    }
    
    // Salvar estado dos atuadores em CSV
    void saveActuatorStateToCSV(const char* state) {
        char timestamp[30];
        char csvLine[100];
        
        DateTime now = get_rtc_datetime();
        sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                now.year, now.month, now.day,
                now.hours, now.minutes, now.seconds);
        
        // Salvar estado dos ventiladores
        sprintf(csvLine, "%s,vent1,%s", timestamp, state);
        csvLogger->data(csvLine);
        
        sprintf(csvLine, "%s,vent2,%s", timestamp, state);
        csvLogger->data(csvLine);
    }
    
    // Obter estado do sistema como string
    String getModeString() {
        return currentMode == MODE_AUTOMATIC ? "AUTO" : "MANUAL";
    }
    
    // Salvar configuração
    void saveConfig() {
        // Implementar salvamento em JSON no SD card
        char configData[200];
        sprintf(configData, "{\"mode\":\"%s\",\"tempMin\":%.1f,\"tempMax\":%.1f}",
                getModeString().c_str(), tempMin, tempMax);
        
        // TODO: Salvar no ficheiro de configuração
        logger->debug("Configuration saved");
    }
};

#endif // SYSTEM_CONTROL_HPP