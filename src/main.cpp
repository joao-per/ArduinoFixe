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
#include "NTP.hpp"

// UART ESP8266
HardwareSerial Serial1(PA10, PA9);

// Objetos globais
ExtMEM logs;
ExtMEM csv;

// DHT GLOBAL - COMO GRUPO 2!!!
DHT_Unified dht(DHTPIN, DHTTYPE);

// LEDs
LED ledStatus;
LED ledTemp;
LED ledWifi;
LED ledSD;

// WiFi e MQTT
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// NTP
NTPClient ntp;

// Temporização
unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;

// Estados
bool sdCardAvailable = false;
bool wifiConnected = false;
bool mqttConnected = false;

// Valores do sensor
float temperature = 0.0;
float humidity = 0.0;

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

// Ler sensor - ESTILO GRUPO 2
void readSensor() {
    sensors_event_t event;
    
    // Temperatura
    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
        temperature = event.temperature;
        
        // Log estilo Grupo 2
        dtostrf(temperature, 4, 1, floatString_temperature);
        logs.debug(floatString_temperature);
        
        char msg[64];
        String tempMsg = "Temperature = ";
        tempMsg += floatString_temperature;
        tempMsg += "C";
        logs.info(tempMsg.c_str());
    } else {
        logs.error("Error reading temperature!");
    }
    
    // Humidade
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
        humidity = event.relative_humidity;
        
        dtostrf(humidity, 4, 1, floatString_humidity);
        
        String humMsg = "Humidity = ";
        humMsg += floatString_humidity;
        humMsg += "%";
        logs.info(humMsg.c_str());
    } else {
        logs.error("Error reading humidity!");
    }
}

// Publicar dados dos sensores
void publishSensorData() {
    if (!mqttClient.connected()) return;
    
    char payload[20];
    
    // Publicar temperatura
    dtostrf(temperature, 4, 1, payload);
    mqttClient.publish(TOPIC_BASE "sensor1/temperatura", payload);
    
    // Publicar humidade
    dtostrf(humidity, 4, 1, payload);
    mqttClient.publish(TOPIC_BASE "sensor1/humidade", payload);
    
    // Publicar estado
    String statusJson = "{\"sensor1\":{\"status\":\"OK\",\"temperatura\":";
    statusJson += String(temperature, 1);
    statusJson += ",\"humidade\":";
    statusJson += String(humidity, 1);
    statusJson += "}}";
    mqttClient.publish(TOPIC_SENSOR_STATUS, statusJson.c_str());
    
    // Publicar estado do SD
    mqttClient.publish(TOPIC_SD_STATUS, sdCardAvailable ? "OK" : "NOK");
    
    logs.debug("Published MQTT data");
}

// Guardar CSV
void saveToCSV() {
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
    csvLine += floatString_temperature;
    
    csv.data(csvLine.c_str());
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
    
    // 6. Inicializar DHT - COMO GRUPO 2!
    logs.info("6. Initializing DHT sensor...");
    dht.begin();
    
    // Info do sensor
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    logs.debug(sensor.name);
    
    delay(2000); // Dar tempo ao sensor
    logs.info("DHT sensor ready");
    
    logs.info("System ready!");
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
    
    // Ler sensor - A CADA 5 SEGUNDOS
    if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentTime;
        
        logs.debug("Reading sensor...");
        readSensor();
        
        // Atualizar LED temperatura
        if (temperature >= TEMP_WARNING_HIGH) {
            ledTemp.off(); // LED verde apaga quando quente
        } else {
            ledTemp.on(); // LED verde aceso quando normal
        }
        
        // Guardar CSV
        saveToCSV();
    }
    
    // Publicar MQTT
    if (currentTime - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
        lastMqttPublish = currentTime;
        publishSensorData();
        
        // Atualização de estado
        logs.info("System status update");
    }
    
    delay(100); // Pequeno delay como Grupo 2
}