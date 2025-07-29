#ifndef SIMPLE_SENSOR_HPP
#define SIMPLE_SENSOR_HPP

#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include "logs.hpp"
#include <config.hpp>

class SimpleSensor {
private:
    DHT_Unified* dht;
    ExtMEM* logger;
    float lastTemperature;
    float lastHumidity;
    
public:
    SimpleSensor(DHT_Unified* dhtSensor, ExtMEM* log) 
        : dht(dhtSensor), logger(log), lastTemperature(0.0), lastHumidity(0.0) {}
    
    void readSensor() {
        sensors_event_t event;
        
        // Ler temperatura
        dht->temperature().getEvent(&event);
        if (!isnan(event.temperature)) {
            lastTemperature = event.temperature;
        } else {
            logger->error("Failed to read temperature from DHT sensor!");
        }
        
        // Ler humidade
        dht->humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            lastHumidity = event.relative_humidity;
        } else {
            logger->error("Failed to read humidity from DHT sensor!");
        }
        
        // Registar valores
        logger->debug("Sensor reading completed");
        char tempStr[10];
        dtostrf(lastTemperature, 4, 1, tempStr);
        logger->debug(tempStr);
    }
    
    float getTemperature() { return lastTemperature; }
    float getHumidity() { return lastHumidity; }
    
    // Guardar em CSV
    void saveToCSV(ExtMEM* csv) {
        // Criar linha CSV
        String csvLine = "";
        DateTime now = get_rtc_datetime();
        
        // Adicionar timestamp
        csvLine += String(now.year) + "-";
        if (now.month < 10) csvLine += "0";
        csvLine += String(now.month) + "-";
        if (now.day < 10) csvLine += "0";
        csvLine += String(now.day) + "T";
        if (now.hours < 10) csvLine += "0";
        csvLine += String(now.hours) + ":";
        if (now.minutes < 10) csvLine += "0";
        csvLine += String(now.minutes) + ":";
        if (now.seconds < 10) csvLine += "0";
        csvLine += String(now.seconds) + "Z";
        
        csvLine += ",sensor1,OK,";
        char tempStr[10];
        dtostrf(lastTemperature, 4, 1, tempStr);
        csvLine += tempStr;
        
        csv->data(csvLine.c_str());
    }
};

#endif // SIMPLE_SENSOR_HPP