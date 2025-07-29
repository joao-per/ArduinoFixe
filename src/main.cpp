// Sistema de Arrefecimento - STM32L476RG

#include <Arduino.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <config.hpp>
#include "set_rtc.hpp"
#include "LED.hpp"
#include "connect.hpp"
#include "logs.hpp"
#include "SimpleSensor.hpp"
#include "NTP.hpp"

// UART ESP8266
HardwareSerial Serial1(PA10, PA9);

// Objetos globais
ExtMEM logs;
ExtMEM csv;

// Sensor
DHT_Unified dht(DHTPIN, DHTTYPE);
SimpleSensor* sensor;

// NTP
NTPClient ntp;

// LEDs
LED ledStatus;
LED ledTemp;
LED ledWifi;
LED ledSD;

// WiFi e MQTT
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// Temporização
unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;

// Estados
bool sdCardAvailable = false;
bool wifiConnected = false;
bool mqttConnected = false;

// Buffers
char floatString_temperature[10];
char floatString_humidity[10];
char msg_temperature[150];
char msg_humidity[150];
char time_string[20];
configData config_data = {0};

// Callback MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    logs.info("MQTT received message");
    logs.debug(topic);
    logs.debug(message);
}

// Reconectar MQTT
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        logs.info("Connecting to MQTT...");
        
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            mqttConnected = true;
        } else {
            // MQTT falhou
            mqttConnected = false;
            delay(5000);
        }
    }
}

// Publicar dados dos sensores
void publishSensorData() {
    if (!mqttClient.connected()) return;
    
    char payload[20];
    
    // Publicar temperatura
    dtostrf(sensor->getTemperature(), 4, 1, payload);
    mqttClient.publish(TOPIC_BASE "sensor1/temperatura", payload);
    
    // Publicar humidade
    dtostrf(sensor->getHumidity(), 4, 1, payload);
    mqttClient.publish(TOPIC_BASE "sensor1/humidade", payload);
    
    // Publicar estado
    String statusJson = "{\"sensor1\":{\"status\":\"OK\",\"temperatura\":";
    statusJson += String(sensor->getTemperature(), 1);
    statusJson += ",\"humidade\":";
    statusJson += String(sensor->getHumidity(), 1);
    statusJson += "}}";
    mqttClient.publish(TOPIC_SENSOR_STATUS, statusJson.c_str());
    
    // Publicar estado do SD
    mqttClient.publish(TOPIC_SD_STATUS, sdCardAvailable ? "OK" : "NOK");
    
    // MQTT publicado
}

// Setup
void setup() {
    // Porta série
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    logs.info("Sistema de Arrefecimento STM32L476RG");
    
    // Inicializar LEDs
    ledStatus.init(LED_PIN);
    ledTemp.init(LED_TEMP_GREEN);
    ledWifi.init(LED_WIFI);
    ledSD.init(LED_SD);
    
    // Inicializar SD Card
    sdCardAvailable = logs.initExtMem();
    if (sdCardAvailable) {
        logs.initFile("log");
        csv.initFile("csv");
        ledSD.on();
        // SD OK
    } else {
        // SD falhou
        ledSD.off();
    }
    
    // Inicializar RTC
    if (initRTC()) {
        // RTC OK
        setRTCToCompileTime();
    } else {
        // RTC falhou
    }
    
    // Inicializar WiFi
    Serial1.begin(SERIAL_BAUD_RATE);
    WiFi.init(Serial1);
    connectWiFi();
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    ledWifi.setState(wifiConnected);
    
    // Atualizar hora via NTP se WiFi ligado
    if (wifiConnected) {
        if (ntp.updateTime()) {
            logs.info("Hora atualizada via NTP");
        }
    }
    
    // Inicializar MQTT
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    
    // Inicializar sensor
    dht.begin();
    sensor = new SimpleSensor(&dht, &logs);
    
    logs.info("Sistema pronto");
    delay(1000);
}

// Loop principal
void loop() {
    unsigned long currentTime = millis();
    
    // Verificar ligações
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        ledWifi.off();
        connectWiFi();
        wifiConnected = (WiFi.status() == WL_CONNECTED);
        ledWifi.setState(wifiConnected);
    }
    
    if (!mqttClient.connected()) {
        mqttConnected = false;
        reconnectMQTT();
    }
    mqttClient.loop();
    
    // Piscar LED estado
    static unsigned long lastBlink = 0;
    if (currentTime - lastBlink > 1000) {
        lastBlink = currentTime;
        ledStatus.toggle();
    }
    
    // Ler sensor
    if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentTime;
        
        sensor->readSensor();
        
        // Atualizar LED temperatura
        if (sensor->getTemperature() >= TEMP_WARNING_HIGH) {
            ledTemp.off(); // LED verde apaga quando quente
        } else {
            ledTemp.on(); // LED verde aceso quando normal
        }
        
        // Guardar CSV
        sensor->saveToCSV(&csv);
    }
    
    // Publicar MQTT
    if (currentTime - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
        lastMqttPublish = currentTime;
        publishSensorData();
        
        // Atualização de estado
        logs.info("System status update");
        char tempStr[10];
        dtostrf(sensor->getTemperature(), 4, 1, tempStr);
        logs.debug(tempStr);
    }
}