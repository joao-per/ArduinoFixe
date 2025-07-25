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
GPI ledGreen;
GPI ledRed;
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
bool systemInitialized = false;

// Buffers (definições das variáveis externas do config.hpp)
char floatString_temperature[10];
char floatString_humidity[10];
char msg_temperature[150];
char msg_humidity[150];
char time_string[20];
configData config_data = {0};

// ========== FUNÇÕES AUXILIARES ==========

// Test pin availability
void testPin(int pin, const char* pinName) {
    Serial.print("Testing pin ");
    Serial.print(pinName);
    Serial.print(": ");
    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(100);
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    
    Serial.println("OK");
}

// Test only safe pins for RED LED
void testRedLedPins() {
    Serial.println("=== TESTING POTENTIAL RED LED PINS ===");
    
    // Test PC15 first (current config)
    Serial.println("Testing PC15 (current RED LED pin):");
    testPin(PC15, "PC15");
    
    // Test some safe alternative pins
    Serial.println("Testing alternative pins:");
    testPin(PC14, "PC14");
    testPin(PC13, "PC13");
    testPin(PB2, "PB2");
    testPin(PB3, "PB3");
    testPin(PB4, "PB4");
    testPin(PB5, "PB5");
    
    Serial.println("=== RED LED PIN TEST COMPLETE ===");
}

// Inicializar todos os LEDs
void initializeLEDs() {
    Serial.println("   Initializing LEDs...");
    
    // Initialize with direct pin control for temperature LEDs (INVERTED LOGIC)
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);  // HIGH = OFF for these LEDs
    digitalWrite(LED_RED, HIGH);    // HIGH = OFF for these LEDs
    Serial.println("   Temperature LEDs (PC12, PC15) initialized - INVERTED LOGIC");
    
    ledStatus.init(LED_PIN);
    ledSensor1.init(LED_SENSOR1);
    ledSensor2.init(LED_SENSOR2);
    ledSensor3.init(LED_SENSOR3);
    ledSensor4.init(LED_SENSOR4);
    ledWifi.init(LED_WIFI);
    ledSD.init(LED_SD);
    
    // Teste inicial - piscar LEDs de temperatura
    Serial.println("   Testing temperature LEDs...");
    Serial.println("   GREEN LED (PC12) test:");
    for (int i = 0; i < 3; i++) {
        Serial.println("     GREEN ON");
        digitalWrite(LED_GREEN, LOW);   // LOW = ON (inverted)
        delay(500);
        Serial.println("     GREEN OFF");
        digitalWrite(LED_GREEN, HIGH);  // HIGH = OFF (inverted)
        delay(500);
    }
    
    Serial.println("   RED LED (PC15) test - EXTENSIVE TESTING:");
    
    // Test 1: Normal inverted logic (LOW = ON)
    Serial.println("   Test 1: LOW = ON, HIGH = OFF");
    for (int i = 0; i < 2; i++) {
        Serial.println("     RED LOW (should be ON)");
        digitalWrite(LED_RED, LOW);
        delay(1000);
        Serial.println("     RED HIGH (should be OFF)");
        digitalWrite(LED_RED, HIGH);
        delay(1000);
    }
    
    // Test 2: Try normal logic (HIGH = ON)
    Serial.println("   Test 2: HIGH = ON, LOW = OFF");
    for (int i = 0; i < 2; i++) {
        Serial.println("     RED HIGH (trying ON)");
        digitalWrite(LED_RED, HIGH);
        delay(1000);
        Serial.println("     RED LOW (trying OFF)");
        digitalWrite(LED_RED, LOW);
        delay(1000);
    }
    
    Serial.println("   RED LED test complete - did you see any red light?");
    
    // Set initial state - GREEN ON (temperature assumed <30°C at startup)
    digitalWrite(LED_GREEN, LOW);   // LOW = GREEN ON
    digitalWrite(LED_RED, HIGH);    // HIGH = RED OFF
    Serial.println("   LED initialization complete - GREEN LED should be ON");
}

// Atualizar LEDs baseado no estado do sistema
void updateLEDs() {
    // LED Status - pisca se tudo OK
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        ledStatus.toggle();
    }
    
    // LEDs de temperatura (verde <30°C, vermelho >=30°C) - Direct pin control
    // Only update if system is initialized and we have valid temperature readings
    if (systemInitialized) {
        float avgTemp = sensorManager->getAverageTemperature();
        static bool wasAbove30 = false;
        
        // Only update LEDs if we have a valid temperature reading (not 0.0)
        if (avgTemp > 0.0) {
            if (avgTemp >= 30.0) {
                Serial.println("LED CONTROL: Setting RED ON, GREEN OFF");
                digitalWrite(LED_GREEN, HIGH);  // HIGH = GREEN OFF (inverted)
                digitalWrite(LED_RED, LOW);     // LOW = RED ON (inverted)
                if (!wasAbove30) {
                    logs.warning("Temperature WARNING: reached 30°C or above");
                    wasAbove30 = true;
                }
            } else {
                Serial.println("LED CONTROL: Setting GREEN ON, RED OFF");
                digitalWrite(LED_GREEN, LOW);   // LOW = GREEN ON (inverted)
                digitalWrite(LED_RED, HIGH);    // HIGH = RED OFF (inverted)
                if (wasAbove30) {
                    logs.info("Temperature back to normal (<30°C)");
                    wasAbove30 = false;
                }
            }
        }
        // If avgTemp is 0.0 (no valid reading yet), keep GREEN LED on
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
    
    // 1. Test RED LED pins first
    Serial.println("1. Testing RED LED pins...");
    testRedLedPins();
    
    // 2. Inicializar LEDs
    Serial.println("2. Initializing LEDs...");
    initializeLEDs();
    
    // 3. Inicializar SD Card
    Serial.println("3. Initializing SD Card...");
    sdCardAvailable = logs.initExtMem();
    if (sdCardAvailable) {
        logs.initFile("log");
        csv.initFile("csv");
        csv.data(CSV_HEADER);
        // asn.readSN(); // Commented out - no config.json file available
        Serial.println("   ✓ SD Card OK (config file skipped)");
    } else {
        Serial.println("   ✗ SD Card failed!");
    }
    
    // 4. Inicializar RTC
    Serial.println("4. Initializing RTC...");
    if (initRTC()) {
        Serial.println("   ✓ RTC OK");
        setRTCToCompileTime(); // Set RTC to compilation time (closest to real time)
    } else {
        Serial.println("   ✗ RTC failed!");
    }
    
    // 5. WiFi/MQTT - SKIPPED
    Serial.println("5. WiFi/MQTT initialization - SKIPPED");
    wifiConnected = false;
    mqttConnected = false;
    
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
    systemInitialized = true;
    
    Serial.println("\n✓ System ready!\n");
    delay(1000);

}

// ========== LOOP PRINCIPAL ==========
void loop() {
    unsigned long currentTime = millis();
    
    // 1. Network connections - SKIPPED
    
    // 2. Processar interrupções
    interruptManager->processInterrupts(systemControl);
    
    // 3. Process sensors and show debug (every 2 seconds) - only after initialization
    if (systemInitialized && (interruptManager->isTimerTriggered() || 
        (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL))) {
        lastSensorRead = currentTime;
        
        sensorManager->readAllSensors();
        
        // Processar controlo automático
        float avgTemp = sensorManager->getAverageTemperature();
        systemControl->processControl(avgTemp);
    }
    
    // 4. MQTT publishing - SKIPPED
    
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
    
    // 7. Imprimir status no Serial (debug) - apenas uma vez
    static unsigned long lastSerialPrint = 0;
    static bool statusReportShown = false;
    if (IS_DEBUG_LOG && !statusReportShown && (currentTime > 15000)) { // After 15 seconds
        statusReportShown = true;
        
        DateTime now = get_rtc_datetime();
        char statusMsg[300];
        sprintf(statusMsg, "[%02d/%02d/%04d %02d:%02d:%02d] [INFO] Status Report: Mode: %s | Emergency: %s | Temp: %.1f°C | Humidity: %.1f%% | Actuators: %s | WiFi: %s | MQTT: %s | SD: %s",
                now.day, now.month, now.year, now.hours, now.minutes, now.seconds,
                systemControl->getModeString().c_str(),
                systemControl->isEmergency() ? "YES" : "NO",
                sensorManager->getAverageTemperature(),
                sensorManager->getAverageHumidity(),
                systemControl->getActuatorsState() ? "ON" : "OFF",
                wifiConnected ? "OK" : "NOK",
                mqttConnected ? "OK" : "NOK",
                sdCardAvailable ? "OK" : "NOK");
        
        Serial.println(statusMsg);
        logs.info("Status Report generated");
    }
}