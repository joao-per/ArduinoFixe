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

// CSV filename
char csvFilename[30] = "Status.csv";

// ========== FUNÇÕES AUXILIARES ==========

// Save CSV data directly to SD card
void saveCSVData() {
    extern SdFat sd;
    extern SdFile file;
    
    if (!sdCardAvailable) return;
    
    // Get current sensors
    auto sensors = sensorManager->getAllSensors();
    
    // Get timestamp
    DateTime now = get_rtc_datetime();
    char timestamp[30];
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            now.year, now.month, now.day,
            now.hours, now.minutes, now.seconds);
    
    // Debug: Check sensor values before writing
    Serial.println("DEBUG: Sensor values before CSV write:");
    for (const auto& sensor : sensors) {
        Serial.print("  Sensor ");
        Serial.print(sensor.id);
        Serial.print(": temp=");
        Serial.print(sensor.temperature);
        Serial.print(", state=");
        Serial.println(sensor.state == SENSOR_OK ? "OK" : "NOK");
    }
    
    // Write each sensor to CSV
    if (file.open(csvFilename, O_RDWR | O_CREAT | O_APPEND)) {
        for (const auto& sensor : sensors) {
            char csvLine[150];
            
            // Use dtostrf for temperature like we do in sensor debug
            char tempStr[10];
            dtostrf(sensor.temperature, 4, 1, tempStr);
            
            sprintf(csvLine, "%s,sensor%d,%s,%s",
                    timestamp,
                    sensor.id,
                    sensor.state == SENSOR_OK ? "OK" : "NOK",
                    tempStr);
            
            file.println(csvLine);
            Serial.print("  Written: ");
            Serial.println(csvLine);
        }
        file.close();
        Serial.println("CSV data written to SD card");
    } else {
        Serial.println("Failed to open CSV file for writing");
    }
}

// Read and display CSV file contents
void displayCSVContents() {
    extern SdFat sd;
    extern SdFile file;
    
    if (!sdCardAvailable) {
        Serial.println("SD card not available");
        return;
    }
    
    Serial.println("=== CSV FILE CONTENTS ===");
    
    if (file.open(csvFilename, O_READ)) {
        int lineCount = 0;
        char line[200];
        
        while (file.fgets(line, sizeof(line)) && lineCount < 20) { // Show max 20 lines
            Serial.print("Line ");
            Serial.print(lineCount + 1);
            Serial.print(": ");
            Serial.print(line);
            lineCount++;
        }
        
        if (lineCount >= 20) {
            Serial.println("... (showing first 20 lines only)");
        }
        
        file.close();
        Serial.print("Total lines displayed: ");
        Serial.println(lineCount);
    } else {
        Serial.println("Failed to open CSV file for reading");
    }
    
    Serial.println("=== END CSV CONTENTS ===");
}

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
    testPin(LED_TEMP_RED, "LED_TEMP_RED (PB2)");
    
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
    pinMode(LED_TEMP_GREEN, OUTPUT);
    pinMode(LED_TEMP_RED, OUTPUT);
    digitalWrite(LED_TEMP_GREEN, HIGH);  // HIGH = OFF for these LEDs
    digitalWrite(LED_TEMP_RED, HIGH);    // HIGH = OFF for these LEDs
    Serial.println("   Bicolor LED (PC12=GREEN, PC15=RED) initialized - TESTING ALL COMBINATIONS");
    
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
        digitalWrite(LED_TEMP_GREEN, LOW);   // LOW = ON (inverted)
        delay(500);
        Serial.println("     GREEN OFF");
        digitalWrite(LED_TEMP_GREEN, HIGH);  // HIGH = OFF (inverted)
        delay(500);
    }
    
    Serial.println("=== BICOLOR LED TEST - All possible combinations ===");
    
    // OFF state
    Serial.println("1. LED OFF: PC12=HIGH, PC15=HIGH");
    digitalWrite(LED_TEMP_GREEN, HIGH);
    digitalWrite(LED_TEMP_RED, HIGH);
    delay(3000);
    
    // GREEN only
    Serial.println("2. GREEN ONLY: PC12=LOW, PC15=HIGH");
    digitalWrite(LED_TEMP_GREEN, LOW);
    digitalWrite(LED_TEMP_RED, HIGH);
    delay(3000);
    
    // RED only (attempt 1)
    Serial.println("3. RED ATTEMPT 1: PC12=HIGH, PC15=LOW");
    digitalWrite(LED_TEMP_GREEN, HIGH);
    digitalWrite(LED_TEMP_RED, LOW);
    delay(3000);
    
    // Both LOW (might be RED or mixed)
    Serial.println("4. BOTH LOW: PC12=LOW, PC15=LOW");
    digitalWrite(LED_TEMP_GREEN, LOW);
    digitalWrite(LED_TEMP_RED, LOW);
    delay(3000);
    
    Serial.println("=== Which of these 4 states showed RED color? ===");
    
    // Set initial state - GREEN ON (temperature assumed <30°C at startup)
    digitalWrite(LED_TEMP_GREEN, LOW);   // LOW = GREEN ON
    digitalWrite(LED_TEMP_RED, HIGH);    // HIGH = RED OFF
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
                digitalWrite(LED_TEMP_GREEN, HIGH);  // HIGH = GREEN OFF (inverted)
                digitalWrite(LED_TEMP_RED, LOW);     // LOW = RED ON (inverted)
                if (!wasAbove30) {
                    logs.warning("Temperature WARNING: reached 30°C or above");
                    wasAbove30 = true;
                }
            } else {
                digitalWrite(LED_TEMP_GREEN, LOW);   // LOW = GREEN ON (inverted)
                digitalWrite(LED_TEMP_RED, HIGH);    // HIGH = RED OFF (inverted)
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
        // Initialize LOG file
        logs.initFile("log");
        Serial.println("   ✓ Log file initialized");
        
        // Initialize CSV file directly
        extern SdFat sd;
        extern SdFile file;
        
        // Create CSV file and write header
        if (file.open(csvFilename, O_RDWR | O_CREAT | O_APPEND)) {
            file.println(CSV_HEADER);
            file.close();
            Serial.print("   ✓ CSV file created: ");
            Serial.println(csvFilename);
        } else {
            Serial.println("   ✗ CSV file creation failed!");
        }
        
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
    
    // 5. Inicializar WiFi
    Serial.println("5. Initializing WiFi...");
    Serial1.begin(SERIAL_BAUD_RATE);
    WiFi.init(Serial1);
    connectWiFi();
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    // 6. Inicializar MQTT
    Serial.println("6. Initializing MQTT...");
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    
    // 7. Inicializar sensores
    Serial.println("7. Initializing sensors...");
    dht.begin();
    sensorManager = new SensorManager(&dht, &logs, &csv);
    Serial.println("   ✓ DHT11 initialized");
    
    // 8. Inicializar sistema de controlo
    Serial.println("8. Initializing control system...");
    systemControl = new SystemControl(&logs, &csv);
    
    // 9. Inicializar interrupções
    Serial.println("9. Initializing interrupts...");
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
    
    // 3. Process sensors and show debug (every 2 seconds) - only after initialization
    if (systemInitialized && (interruptManager->isTimerTriggered() || 
        (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL))) {
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
    
    // 5. Salvar dados em CSV (cada 6 segundos)
    if (currentTime - lastCsvSave >= 6000) {
        lastCsvSave = currentTime;
        
        Serial.println("=== SAVING CSV DATA ===");
        saveCSVData();
        Serial.println("CSV data saved");
        
        // List SD card files every 30 seconds
        static unsigned long lastFileList = 0;
        if (currentTime - lastFileList >= 30000) {
            lastFileList = currentTime;
            Serial.println("=== SD CARD FILES ===");
            
            // List files on SD card
            SdFile root;
            if (root.open("/")) {
                Serial.println("Files on SD card:");
                while (true) {
                    SdFile entry;
                    if (!entry.openNext(&root, O_READ)) break;
                    
                    char name[64];
                    entry.getName(name, sizeof(name));
                    Serial.print("  ");
                    Serial.print(name);
                    Serial.print(" (");
                    Serial.print(entry.fileSize());
                    Serial.println(" bytes)");
                    entry.close();
                }
                root.close();
            }
            Serial.println("=== END FILE LIST ===");
            
            // Also display CSV contents every 30 seconds
            displayCSVContents();
        }
        
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