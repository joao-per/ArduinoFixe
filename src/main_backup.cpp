/* Sistema de Arrefecimento - Sala de Máquinas
 * UFCD 5136 - Sistemas de Microcontroladores
 * STM32L476RG + ESP8266 + DHT11
 */

/* System libraries */
#include <Arduino.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

/* User libraries */
#include <config.hpp>
#include "set_rtc.hpp"
#include "GPI.hpp"
#include "connect.hpp"
#include "logs.hpp"
#include "SensorManager.hpp"
#include "SystemControl.hpp"
#include "InterruptManager.hpp"

// ========== UART para ESP8266 ==========
HardwareSerial Serial1(PA10, PA9); // RX, TX



// ========== OBJETOS GLOBAIS ==========
// Logs e armazenamento
ExtMEM logs;
ExtMEM csv;
ExtMEM asn;


// Sensores
DHT_Unified dht(DHTPIN, DHTTYPE);
SensorManager* sensorManager;

// Controlo do sistema
SystemControl* systemControl;
InterruptManager* interruptManager;

// LEDs
GPI ledStatus;
GPI ledSensor1;
GPI ledSensor2;
GPI ledSensor3;
GPI ledSensor4;
GPI ledWifi;
GPI ledSD;

// WiFi e MQTT (já definidos em connect.hpp)
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// ========== VARIÁVEIS GLOBAIS ==========
// Timings
unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastCsvSave = 0;

// Estados
bool sdCardAvailable = false;
bool wifiConnected = false;
bool mqttConnected = false;

// Buffers (definições das variáveis externas do config.hpp)
char floatString_temperature[10];
char floatString_humidity[10];
char msg_temperature[150];
char msg_humidity[150];
char time_string[20];
configData config_data = {0};

// ========== FUNÇÕES AUXILIARES ==========

// Inicializar todos os LEDs
void initializeLEDs() {
    ledStatus.init(LED_PIN);
    ledSensor1.init(LED_SENSOR1);
    ledSensor2.init(LED_SENSOR2);
    ledSensor3.init(LED_SENSOR3);
    ledSensor4.init(LED_SENSOR4);
    ledWifi.init(LED_WIFI);
    ledSD.init(LED_SD);
    
    // Teste inicial - piscar todos
    for (int i = 0; i < 3; i++) {
        ledStatus.on(); ledSensor1.on(); ledSensor2.on();
        ledSensor3.on(); ledSensor4.on(); ledWifi.on(); ledSD.on();
        delay(200);
        ledStatus.off(); ledSensor1.off(); ledSensor2.off();
        ledSensor3.off(); ledSensor4.off(); ledWifi.off(); ledSD.off();
        delay(200);
    }
}

// Atualizar LEDs baseado no estado do sistema
void updateLEDs() {
    // LED Status - pisca se tudo OK
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        ledStatus.toggle();
    }
    
    // LEDs dos sensores
    auto sensors = sensorManager->getAllSensors();
    ledSensor1.setState(sensors[0].state == SENSOR_OK);
    ledSensor2.setState(sensors[1].state == SENSOR_OK);
    ledSensor3.setState(sensors[2].state == SENSOR_OK);
    ledSensor4.setState(sensors[3].state == SENSOR_OK);
    
    // LED WiFi
    ledWifi.setState(wifiConnected && mqttConnected);
    
    // LED SD Card
    ledSD.setState(sdCardAvailable);
    
    // Em emergência, piscar LED status rapidamente
    if (systemControl->isEmergency()) {
        ledStatus.blink(200);
    }
}

// Publicar dados dos sensores via MQTT
void publishSensorData() {
    if (!mqttClient.connected()) return;
    
    auto sensors = sensorManager->getAllSensors();
    char topic[50];
    char payload[20];
    
    // Publicar temperatura e humidade de cada sensor
    for (const auto& sensor : sensors) {
        // Temperatura
        sprintf(topic, "%s%d%s", TOPIC_TEMP_PREFIX, sensor.id, TOPIC_TEMP_SUFFIX);
        dtostrf(sensor.temperature, 4, 1, payload);
        mqttClient.publish(topic, payload);
        
        // Humidade
        sprintf(topic, "%s%d%s", TOPIC_HUM_PREFIX, sensor.id, TOPIC_HUM_SUFFIX);
        dtostrf(sensor.humidity, 4, 1, payload);
        mqttClient.publish(topic, payload);
    }
    
    // Publicar estado dos sensores em JSON
    String statusJson = sensorManager->getStatusJSON();
    mqttClient.publish(TOPIC_SENSOR_STATUS, statusJson.c_str());
    
    // Publicar estado do SD Card
    mqttClient.publish(TOPIC_SD_STATUS, sdCardAvailable ? "OK" : "NOK");
    
    // Log
    char msg[100];
    float avgTemp = sensorManager->getAverageTemperature();
    sprintf(msg, "Published MQTT data. Avg temp: %.1f°C", avgTemp);
    logs.debug(msg);
}

// Callback MQTT melhorado
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Converter payload para string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.print("MQTT received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);
    
    // Processar comandos
    if (strcmp(topic, TOPIC_SETPOINT_MIN) == 0) {
        float temp = atof(message);
        systemControl->setTempMin(temp);
    }
    else if (strcmp(topic, TOPIC_SETPOINT_MAX) == 0) {
        float temp = atof(message);
        systemControl->setTempMax(temp);
    }
    else if (strcmp(topic, TOPIC_MODE) == 0) {
        if (strcmp(message, "AUTO") == 0) {
            systemControl->setMode(MODE_AUTOMATIC);
        } else if (strcmp(message, "MANUAL") == 0) {
            systemControl->setMode(MODE_MANUAL);
        }
    }
    else if (strcmp(topic, TOPIC_EMERGENCY) == 0) {
        systemControl->setEmergency(message[0] == '1');
    }
    else if (strcmp(topic, TOPIC_ACTUATOR_CMD) == 0) {
        if (systemControl->getMode() == MODE_MANUAL) {
            systemControl->setActuators(strcmp(message, "ON") == 0);
        }
    }
}

// Reconectar MQTT com subscrições
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.println("Connecting to MQTT...");
        
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("MQTT connected!");
            mqttConnected = true;
            
            // Subscrever a todos os tópicos de controlo
            mqttClient.subscribe(TOPIC_SETPOINT_MIN);
            mqttClient.subscribe(TOPIC_SETPOINT_MAX);
            mqttClient.subscribe(TOPIC_INTERVAL);
            mqttClient.subscribe(TOPIC_MODE);
            mqttClient.subscribe(TOPIC_EMERGENCY);
            mqttClient.subscribe(TOPIC_ACTUATOR_CMD);
            
            // Publicar mensagem de início
            mqttClient.publish(TOPIC_SYSTEM_LOG, "System started");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" trying again in 5 seconds...");
            mqttConnected = false;
            delay(5000);
        }
    }
}

// ========== SETUP ==========
void setup() {
    // Serial para debug
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   SISTEMA DE ARREFECIMENTO v1.0");
    Serial.println("   Sala de Máquinas - STM32L476RG");
    Serial.println("========================================\n");
    
    // 1. Inicializar LEDs
    Serial.println("1. Initializing LEDs...");
    initializeLEDs();
    
    // 2. Inicializar SD Card
    Serial.println("2. Initializing SD Card...");
    sdCardAvailable = logs.initExtMem();
    if (sdCardAvailable) {
        logs.initFile("log");
        csv.initFile("csv");
        csv.data(CSV_HEADER);
        asn.readSN();
        Serial.println("   ✓ SD Card OK");
    } else {
        Serial.println("   ✗ SD Card failed!");
    }
    
    // 3. Inicializar RTC
    Serial.println("3. Initializing RTC...");
    if (initRTC()) {
        Serial.println("   ✓ RTC OK");
        set_rtc_time(2025, 7, 17, 11, 01, 02);
    } else {
        Serial.println("   ✗ RTC failed!");
    }
    
    // 4. Inicializar WiFi
    Serial.println("4. Initializing WiFi...");
    Serial1.begin(SERIAL_BAUD_RATE);
    WiFi.init(Serial1);
    connectWiFi();
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    // 5. Inicializar MQTT
    Serial.println("5. Initializing MQTT...");
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    
    // 6. Inicializar sensores
    Serial.println("6. Initializing sensors...");
    dht.begin();
    sensorManager = new SensorManager(&dht, &logs, &csv);
    Serial.println("   ✓ DHT11 initialized");
    
    // 7. Inicializar sistema de controlo
    Serial.println("7. Initializing control system...");
    systemControl = new SystemControl(&logs, &csv);
    
    // 8. Inicializar interrupções
    Serial.println("8. Initializing interrupts...");
    interruptManager = new InterruptManager(&logs);
    interruptManager->setupHardwareInterrupts();
    interruptManager->setupTimerInterrupt();
    Serial.println("   ✓ Interrupts configured");
    
    // Log de início
    logs.info("System initialization complete");
    
    Serial.println("\n✓ System ready!\n");
    delay(1000);

}

// ========== LOOP PRINCIPAL ==========
void loop() {
    unsigned long currentTime = millis();
    
    // 1. Verificar conexões
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        connectWiFi();
        wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
    
    if (!mqttClient.connected()) {
        mqttConnected = false;
        reconnectMQTT();
    }
    mqttClient.loop();
    
    // 2. Processar interrupções
    interruptManager->processInterrupts(systemControl);
    
    // 3. Ler sensores (por timer ou intervalo)
    if (interruptManager->isTimerTriggered() || 
        (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL)) {
        lastSensorRead = currentTime;
        sensorManager->readAllSensors();
        
        // Processar controlo automático
        float avgTemp = sensorManager->getAverageTemperature();
        systemControl->processControl(avgTemp);
    }
    
    // 4. Publicar dados MQTT
    if (currentTime - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
        lastMqttPublish = currentTime;
        publishSensorData();
        
        // Publicar estado do sistema
        mqttClient.publish(TOPIC_MODE, systemControl->getModeString().c_str());
        mqttClient.publish(TOPIC_EMERGENCY, systemControl->isEmergency() ? "1" : "0");
    }
    
    // 5. Salvar dados em CSV (cada minuto)
    if (currentTime - lastCsvSave >= 60000) {
        lastCsvSave = currentTime;
        sensorManager->saveToCSV();
        
        // Log periódico
        char logMsg[150];
        sprintf(logMsg, "System status - Mode: %s, Emergency: %s, Actuators: %s, Avg Temp: %.1f°C",
                systemControl->getModeString().c_str(),
                systemControl->isEmergency() ? "ON" : "OFF",
                systemControl->getActuatorsState() ? "ON" : "OFF",
                sensorManager->getAverageTemperature());
        logs.info(logMsg);
    }
    
    // 6. Atualizar LEDs
    if (currentTime - lastStatusUpdate >= 100) {
        lastStatusUpdate = currentTime;
        updateLEDs();
    }
    
    // 7. Imprimir status no Serial (debug)
    static unsigned long lastSerialPrint = 0;
    if (IS_DEBUG_LOG && (currentTime - lastSerialPrint >= 10000)) {
        lastSerialPrint = currentTime;
        
        DateTime now = get_rtc_datetime();
        Serial.printf("\n[%02d:%02d:%02d] Status Report:\n", 
                      now.hours, now.minutes, now.seconds);
        Serial.printf("  Mode: %s | Emergency: %s\n",
                      systemControl->getModeString().c_str(),
                      systemControl->isEmergency() ? "YES" : "NO");
        Serial.printf("  Temp: %.1f°C (%.1f-%.1f) | Humidity: %.1f%%\n",
                      sensorManager->getAverageTemperature(),
                      systemControl->getTempMin(),
                      systemControl->getTempMax(),
                      sensorManager->getAverageHumidity());
        Serial.printf("  Actuators: %s | WiFi: %s | MQTT: %s | SD: %s\n",
                      systemControl->getActuatorsState() ? "ON" : "OFF",
                      wifiConnected ? "OK" : "NOK",
                      mqttConnected ? "OK" : "NOK",
                      sdCardAvailable ? "OK" : "NOK");
    }
}