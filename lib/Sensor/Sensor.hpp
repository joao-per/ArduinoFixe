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
            logger->info("Initializing DHT sensor...");
            dht->begin();
            initialized = true;
            delay(2000); // Delay inicial maior
            logger->info("DHT initialized");
        }
        
        // Ler sensor
        sensors_event_t event;
        
        // Ler temperatura
        dht->temperature().getEvent(&event);
        if (!isnan(event.temperature)) {
            temperature = event.temperature;
            logger->debug("Temp read OK");
        } else {
            logger->debug("Temp read FAIL");
        }
        
        // Ler humidade
        dht->humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            humidity = event.relative_humidity;
            logger->debug("Hum read OK");
        } else {
            logger->debug("Hum read FAIL");
        }
        
        // Log sempre, mesmo com valores zero
        char logMsg[100];
        char tempStr[10];
        char humStr[10];
        dtostrf(temperature, 4, 1, tempStr);
        dtostrf(humidity, 4, 1, humStr);
        
        // Criar mensagem estilo CSV para debug
        String logLine = "Temp: ";
        logLine += tempStr;
        logLine += "C, Hum: ";
        logLine += humStr;
        logLine += "%";
        
        logger->info(logLine.c_str());
        
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