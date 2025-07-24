#ifndef SENSOR_MANAGER_HPP
#define SENSOR_MANAGER_HPP

#include <Arduino.h>
#include <vector>
#include <DHT.h>
#include <DHT_U.h>
#include "logs.hpp"
#include <config.hpp>

// Estados dos sensores
enum SensorState {
    SENSOR_OK,
    SENSOR_NOK,
    SENSOR_DISCONNECTED
};

// Estrutura para dados de cada sensor
struct SensorReading {
    uint8_t id;
    float temperature;
    float humidity;
    SensorState state;
    unsigned long lastUpdate;
    String name;
};

class SensorManager {
private:
    DHT_Unified* dht;  // Único sensor físico
    std::vector<SensorReading> sensors;
    const uint8_t NUM_VIRTUAL_SENSORS = 4;
    float tempOffset[4] = {0.0, 0.5, -0.3, 0.2}; // Offsets para simular 4 sensores
    float humOffset[4] = {0.0, 2.0, -1.5, 1.0};
    ExtMEM* logger;
    ExtMEM* csvLogger;
    
public:
    SensorManager(DHT_Unified* dhtSensor, ExtMEM* log, ExtMEM* csv) 
        : dht(dhtSensor), logger(log), csvLogger(csv) {
        // Inicializar 4 sensores virtuais
        for (uint8_t i = 0; i < NUM_VIRTUAL_SENSORS; i++) {
            SensorReading sensor;
            sensor.id = i + 1;
            sensor.temperature = 0.0;
            sensor.humidity = 0.0;
            sensor.state = SENSOR_DISCONNECTED;
            sensor.lastUpdate = 0;
            sensor.name = "sensor" + String(i + 1);
            sensors.push_back(sensor);
        }
    }
    
    // Ler todos os sensores (simula 4 a partir de 1)
    void readAllSensors() {
        sensors_event_t event;
        
        // Ler temperatura real
        dht->temperature().getEvent(&event);
        float baseTemp = event.temperature;
        
        // Ler humidade real
        dht->humidity().getEvent(&event);
        float baseHum = event.relative_humidity;
        
        // Verificar se leitura é válida
        if (isnan(baseTemp) || isnan(baseHum)) {
            logger->error("Failed to read from DHT sensor!");
            // Marcar todos como NOK
            for (auto& sensor : sensors) {
                sensor.state = SENSOR_NOK;
            }
            return;
        }
        
        // Simular 4 sensores com pequenas variações
        for (uint8_t i = 0; i < NUM_VIRTUAL_SENSORS; i++) {
            sensors[i].temperature = baseTemp + tempOffset[i] + (random(-10, 10) / 10.0);
            sensors[i].humidity = baseHum + humOffset[i] + (random(-20, 20) / 10.0);
            sensors[i].state = SENSOR_OK;
            sensors[i].lastUpdate = millis();
            
            // Limitar valores
            sensors[i].temperature = constrain(sensors[i].temperature, -10, 50);
            sensors[i].humidity = constrain(sensors[i].humidity, 0, 100);
            
            // Simular falha ocasional (2% chance)
            if (random(100) < 2) {
                sensors[i].state = SENSOR_NOK;
            }
        }
        
        char msg[150];
        if (isnan(baseTemp)) {
            sprintf(msg, "DHT sensor not connected - Base temp: NaN°C, Humidity: NaN%%");
        } else {
            sprintf(msg, "Sensors read successfully. Base temp: %.1f°C, Humidity: %.1f%%", baseTemp, baseHum);
        }
        logger->debug(msg);
    }
    
    // Obter dados de um sensor específico
    SensorReading getSensor(uint8_t id) {
        if (id > 0 && id <= NUM_VIRTUAL_SENSORS) {
            return sensors[id - 1];
        }
        return SensorReading(); // Retornar vazio se ID inválido
    }
    
    // Obter todos os sensores
    std::vector<SensorReading> getAllSensors() {
        return sensors;
    }
    
    // Calcular temperatura média
    float getAverageTemperature() {
        float sum = 0;
        int validCount = 0;
        
        for (const auto& sensor : sensors) {
            if (sensor.state == SENSOR_OK) {
                sum += sensor.temperature;
                validCount++;
            }
        }
        
        return (validCount > 0) ? (sum / validCount) : 0.0;
    }
    
    // Calcular humidade média
    float getAverageHumidity() {
        float sum = 0;
        int validCount = 0;
        
        for (const auto& sensor : sensors) {
            if (sensor.state == SENSOR_OK) {
                sum += sensor.humidity;
                validCount++;
            }
        }
        
        return (validCount > 0) ? (sum / validCount) : 0.0;
    }
    
    // Converter estado para string
    String stateToString(SensorState state) {
        switch(state) {
            case SENSOR_OK: return "OK";
            case SENSOR_NOK: return "NOK";
            case SENSOR_DISCONNECTED: return "DISCONNECTED";
            default: return "UNKNOWN";
        }
    }
    
    // Salvar dados em CSV (formato especificado)
    void saveToCSV() {
        char timestamp[30];
        char csvLine[150];
        
        // Obter timestamp
        DateTime now = get_rtc_datetime();
        sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                now.year, now.month, now.day,
                now.hours, now.minutes, now.seconds);
        
        // Salvar cada sensor
        for (const auto& sensor : sensors) {
            sprintf(csvLine, "%s,%s,%s,%.1f",
                    timestamp,
                    sensor.name.c_str(),
                    stateToString(sensor.state).c_str(),
                    sensor.temperature);
            
            csvLogger->data(csvLine);
        }
    }
    
    // Obter estado JSON para MQTT
    String getStatusJSON() {
        String json = "{\"sensores\":[";
        
        for (size_t i = 0; i < sensors.size(); i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"id\":" + String(sensors[i].id) + ",";
            json += "\"status\":\"" + stateToString(sensors[i].state) + "\",";
            json += "\"temperatura\":" + String(sensors[i].temperature, 1) + ",";
            json += "\"humidade\":" + String(sensors[i].humidity, 1);
            json += "}";
        }
        
        json += "]}";
        return json;
    }
};

#endif // SENSOR_MANAGER_HPP