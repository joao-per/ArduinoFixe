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
    if (WiFi.status() != WL_CONNECTED) {
        mqttConnected = false;
        return;
    }
    
    int tentativas = 0;
    while (!mqttClient.connected() && tentativas < 3) {
        logs.info("Connecting to MQTT...");
        
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            logs.info("MQTT connected!");
            mqttConnected = true;
            mqttClient.publish(TOPIC_SYSTEM_LOG, "System started");
            break;
        } else {
            logs.error("MQTT failed. Retrying...");
            char rcStr[10];
            itoa(mqttClient.state(), rcStr, 10);
            logs.debug(rcStr);
            mqttConnected = false;
            tentativas++;
            if (tentativas < 3) delay(2000);
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
    
    logs.debug("Published MQTT data");
}

// Setup
void setup() {
    // Porta série
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    logs.info("========================================");
    logs.info("   SISTEMA DE ARREFECIMENTO");
    logs.info("   STM32L476RG");
    logs.info("========================================");
    
    // 1. Inicializar LEDs
    logs.info("1. Initializing LEDs...");
    ledStatus.init(LED_PIN);
    ledTemp.init(LED_TEMP_GREEN);
    ledWifi.init(LED_WIFI);
    ledSD.init(LED_SD);
    
    // 2. Inicializar SD Card
    logs.info("2. Initializing SD Card...");
    sdCardAvailable = logs.initExtMem();
    if (sdCardAvailable) {
        logs.initFile("log");
        csv.initFile("csv");
        ledSD.on();
        logs.info("SD Card OK");
    } else {
        logs.error("SD Card failed!");
        ledSD.off();
    }
    
    // 3. Inicializar RTC
    logs.info("3. Initializing RTC...");
    if (initRTC()) {
        logs.info("RTC OK");
        setRTCToCompileTime();
    } else {
        logs.error("RTC failed!");
    }
    
    // 4. Inicializar WiFi
    logs.info("4. Initializing WiFi...");
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
    
    // 5. Inicializar MQTT
    logs.info("5. Initializing MQTT...");
    if (wifiConnected) {
        mqttClient.setCallback(mqttCallback);
        connectMQTT();
    } else {
        logs.warning("Skipping MQTT - No WiFi connection");
    }
    
    // 6. Inicializar sensor
    logs.info("6. Initializing sensor...");
    dht.begin();
    delay(1000); // Dar tempo ao sensor para estabilizar
    sensor = new SimpleSensor(&dht, &logs);
    
    // Teste inicial do sensor
    sensor_t sensorInfo;
    dht.temperature().getSensor(&sensorInfo);
    logs.debug(sensorInfo.name);
    logs.info("DHT11 initialized");
    
    logs.info("System ready!");
    
    // Primeira leitura do sensor após 2 segundos
    delay(2000);
    sensor->readSensor();
    logs.info("First sensor reading completed");
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