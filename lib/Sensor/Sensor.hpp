#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include "config.hpp"
#include "logs.hpp"

class Sensor {
private:
    DHT_Unified* dht;
    ExtMEM* logger;
    float temperature;
    float humidity;
    bool initialized;
    
public:
    Sensor() : temperature(0.0), humidity(0.0), initialized(false) {
        dht = new DHT_Unified(DHTPIN, DHTTYPE);
    }
    
    void setLogger(ExtMEM* log) {
        logger = log;
    }
    
    void begin() {
        // Inicializar DHT a cada chamada
        if (!initialized) {
            dht->begin();
            initialized = true;
            delay(2000); // Delay inicial maior
        }
        
        // Ler sensor
        sensors_event_t event;
        
        dht->temperature().getEvent(&event);
        if (!isnan(event.temperature)) {
            temperature = event.temperature;
        }
        
        dht->humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            humidity = event.relative_humidity;
        }
        
        // Log
        if (temperature > 0 && humidity > 0) {
            char tempStr[10];
            dtostrf(temperature, 4, 1, tempStr);
            logger->debug(tempStr);
        }
        
        delay(1000); // Delay após leitura como Grupo 2
    }
    
    float getTemperature() { return temperature; }
    float getHumidity() { return humidity; }
    
    void saveToCSV(ExtMEM* csv) {
        if (temperature == 0) return;
        
        String csvLine = "";
        DateTime now = get_rtc_datetime();
        
        // Timestamp
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
        dtostrf(temperature, 4, 1, tempStr);
        csvLine += tempStr;
        
        csv->data(csvLine.c_str());
    }
};

#endif