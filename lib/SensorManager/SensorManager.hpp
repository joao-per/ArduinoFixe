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
    
    // Buffer for 10 physical readings
    float tempReadings[10];
    float humReadings[10];
    int readingIndex;
    bool bufferFull;
    unsigned long lastPhysicalRead;
    
public:
    SensorManager(DHT_Unified* dhtSensor, ExtMEM* log, ExtMEM* csv) 
        : dht(dhtSensor), logger(log), csvLogger(csv) {
        // Initialize reading buffer
        readingIndex = 0;
        bufferFull = false;
        lastPhysicalRead = 0;
        
        // Clear reading buffers
        for (int i = 0; i < 10; i++) {
            tempReadings[i] = 0.0;
            humReadings[i] = 0.0;
        }
        
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
    
    // Update physical readings buffer (called every 100ms)
    void updatePhysicalReadings() {
        unsigned long currentTime = millis();
        if (currentTime - lastPhysicalRead < PHYSICAL_READ_INTERVAL) {
            return; // Not time yet
        }
        lastPhysicalRead = currentTime;
        
        sensors_event_t event;
        
        // Read physical DHT sensor
        dht->temperature().getEvent(&event);
        float currentTemp = event.temperature;
        
        dht->humidity().getEvent(&event);
        float currentHum = event.relative_humidity;
        
        // Check if reading is valid
        if (isnan(currentTemp) || isnan(currentHum)) {
            return; // Skip invalid readings
        }
        
        // Store in circular buffer
        tempReadings[readingIndex] = currentTemp;
        humReadings[readingIndex] = currentHum;
        
        readingIndex++;
        if (readingIndex >= 10) {
            readingIndex = 0;
            bufferFull = true;
        }
    }
    
    // Calculate median from 10 readings
    float calculateMedianTemp() {
        if (!bufferFull && readingIndex < 3) {
            return 0.0; // Not enough readings yet
        }
        
        int count = bufferFull ? 10 : readingIndex;
        float sortedTemps[10];
        
        // Copy and sort temperatures
        for (int i = 0; i < count; i++) {
            sortedTemps[i] = tempReadings[i];
        }
        
        for (int i = 0; i < count-1; i++) {
            for (int j = i+1; j < count; j++) {
                if (sortedTemps[i] > sortedTemps[j]) {
                    float temp = sortedTemps[i];
                    sortedTemps[i] = sortedTemps[j];
                    sortedTemps[j] = temp;
                }
            }
        }
        
        // Return median
        if (count % 2 == 0) {
            return (sortedTemps[count/2-1] + sortedTemps[count/2]) / 2.0;
        } else {
            return sortedTemps[count/2];
        }
    }
    
    float calculateMedianHum() {
        if (!bufferFull && readingIndex < 3) {
            return 0.0;
        }
        
        int count = bufferFull ? 10 : readingIndex;
        float sortedHums[10];
        
        for (int i = 0; i < count; i++) {
            sortedHums[i] = humReadings[i];
        }
        
        for (int i = 0; i < count-1; i++) {
            for (int j = i+1; j < count; j++) {
                if (sortedHums[i] > sortedHums[j]) {
                    float temp = sortedHums[i];
                    sortedHums[i] = sortedHums[j];
                    sortedHums[j] = temp;
                }
            }
        }
        
        if (count % 2 == 0) {
            return (sortedHums[count/2-1] + sortedHums[count/2]) / 2.0;
        } else {
            return sortedHums[count/2];
        }
    }
    
    // Simple sensor reading (back to working version)
    void readAllSensors() {
        sensors_event_t event;
        
        // Debug: Check if DHT object exists
        if (!dht) {
            logger->debug("ERROR: DHT object is NULL!");
            return;
        }
        
        // Read physical DHT sensor
        dht->temperature().getEvent(&event);
        float baseTemp = event.temperature;
        
        dht->humidity().getEvent(&event);
        float baseHum = event.relative_humidity;
        
        // Debug: Show raw values with explicit NaN check
        char rawMsg[150];
        if (isnan(baseTemp) && isnan(baseHum)) {
            sprintf(rawMsg, "Raw DHT - Temp: NaN, Hum: NaN (both invalid)");
        } else if (isnan(baseTemp)) {
            sprintf(rawMsg, "Raw DHT - Temp: NaN, Hum: %.2f (temp invalid)", baseHum);
        } else if (isnan(baseHum)) {
            sprintf(rawMsg, "Raw DHT - Temp: %.2f, Hum: NaN (hum invalid)", baseTemp);
        } else {
            sprintf(rawMsg, "Raw DHT - Temp: %.2f, Hum: %.2f (both valid)", baseTemp, baseHum);
        }
        logger->debug(rawMsg);
        
        // Check if readings are valid
        if (isnan(baseTemp) || isnan(baseHum)) {
            logger->debug("DHT Status - Temp: FAIL, Hum: FAIL");
            logger->error("Failed to read from DHT sensor!");
            return;
        }
        
        logger->debug("DHT readings are valid, creating virtual sensors...");
        
        // T1 = Real DHT reading 
        sensors[0].temperature = baseTemp;
        sensors[0].humidity = baseHum;
        sensors[0].state = SENSOR_OK;
        sensors[0].lastUpdate = millis();
        
        logger->debug("T1 sensor created, generating T2/T3/T4...");
        
        // T2, T3, T4 = Derived from T1 with small variations
        for (uint8_t i = 1; i < NUM_VIRTUAL_SENSORS; i++) {
            sensors[i].temperature = sensors[0].temperature + tempOffset[i] + (random(-5, 6) / 10.0);
            sensors[i].humidity = sensors[0].humidity + humOffset[i] + (random(-10, 11) / 10.0);
            sensors[i].state = SENSOR_OK;
            sensors[i].lastUpdate = millis();
            
            // Constrain values to realistic ranges
            sensors[i].temperature = constrain(sensors[i].temperature, -10, 50);
            sensors[i].humidity = constrain(sensors[i].humidity, 0, 100);
        }
        
        logger->debug("All virtual sensors created, calculating median...");
        
        // Calculate median from all 4 sensors
        float temps[NUM_VIRTUAL_SENSORS];
        for (uint8_t i = 0; i < NUM_VIRTUAL_SENSORS; i++) {
            temps[i] = sensors[i].temperature;
        }
        for (int i = 0; i < NUM_VIRTUAL_SENSORS-1; i++) {
            for (int j = i+1; j < NUM_VIRTUAL_SENSORS; j++) {
                if (temps[i] > temps[j]) {
                    float temp = temps[i];
                    temps[i] = temps[j];
                    temps[j] = temp;
                }
            }
        }
        float medianTemp = (temps[1] + temps[2]) / 2.0;
        
        logger->debug("About to display final sensor values...");
        
        // Log individual sensors and median  
        char msg[200];
        sprintf(msg, "Sensors: T1=%.1f°C T2=%.1f°C T3=%.1f°C T4=%.1f°C | Median=%.1f°C", 
                sensors[0].temperature, sensors[1].temperature, sensors[2].temperature, sensors[3].temperature, medianTemp);
        logger->debug(msg);
        
        logger->debug("Finished displaying sensor values!");
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