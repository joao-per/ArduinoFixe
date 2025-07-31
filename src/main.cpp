// Bibliotecas do framework
#include <Arduino.h>        // Biblioteca principal do Arduino
#include <WiFiEspAT.h>      // Biblioteca WiFi
#include <PubSubClient.h>   // Biblioteca MQTT
#include <HardwareSerial.h> // Biblioteca HardwareSerial

// Includes locais
#include "sensorEvent.hpp" // Biblioteca de eventos do sensor
#include "LED.hpp"         // Biblioteca LED
#include "config.hpp"      // Configuração
#include "logs.hpp"        // Logs
#include "connect.hpp"     // Funções de ligação
#include "set_rtc.hpp"     // RTC

// Variáveis
char tempStr[100];     // String para dados de temperatura
char localIpStr[50];   // String para endereço IP local
char gatewayIpStr[50]; // String para endereço IP do gateway
char dnsIpStr[50];     // String para endereço IP do DNS
char mqttMsg[100];     // String para mensagens MQTT
char pubMsg[100];      // String para mensagens publicadas

// Inicialização de estruturas
struct configData config_data = {0}; // Inicialização da estrutura de dados de configuração
extern struct sensorData sensor_data; // Estrutura de dados do sensor de sensorEvent.cpp

// Criar clientes WiFi e MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Definição de classes
HardwareTimer *timer3 = new HardwareTimer(TIM3); // Criar instância HardwareTimer para TIM3
HardwareSerial Serial1(PA10, PA9);               // Criar instância HardwareSerial para Serial1

ExtMEM logs; // Classe de logs
ExtMEM csv;  // Classe CSV
ExtMEM asn;  // Classe ASN

sensorEvent sensor; // Classe de eventos do sensor

LED greenLed; // Classe LED verde
LED redLed;   // Classe LED vermelho

uint32_t delayMS; // Variável para atraso em milissegundos

void sendTemperature() { // Função para ler temperatura e enviar para MQTT (se disponível)
    sensor.getTemperatureAverage(); // Obter temperatura média dos sensores
    
    logs.info(""); // Linha em branco
    logs.info("=== LEITURA DE TEMPERATURA ===");
    logs.info(""); // Linha em branco

    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        dtostrf(sensor_data.temperatureAverageSensors[i], 2, 2, tempStr);
        String tempMsg = "Sensor " + String(i + 1) + " temperatura: " + String(tempStr) + "C";
        logs.debug(tempMsg.c_str());
        
        // Escrever no CSV
        String csvLine = String(millis()) + ";" + String(i + 1) + ";OK;" + String(tempStr);
        csv.data(csvLine.c_str());
    }
    
    // Enviar para MQTT se disponível
    if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
        for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
            dtostrf(sensor_data.temperatureAverageSensors[i], 2, 2, tempStr);
            String topic = "sensor" + String(i + 1) + "/temp";
            mqttClient.publish(topic.c_str(), tempStr);
        }
    }
}

void connectWiFi() { // Função para ligar ao WiFi
    int attempts = 0;
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && attempts < 3 && (millis() - startTime) < 6000) { // Verificar se WiFi está ligado
        logs.info("A ligar ao WiFi...");
        WiFi.begin(SERVER_SSID, SERVER_PASSWORD); // Iniciar ligação WiFi
        attempts++;
        delay(DELAY_WIFI_CONNECTION);
    }

    if (WiFi.status() == WL_CONNECTED) {
        String localIpMsg = "WiFi ligado: " + WiFi.localIP().toString();
        logs.info(localIpMsg.c_str());

        String gatewayMsg = "Gateway: " + WiFi.gatewayIP().toString();
        logs.info(gatewayMsg.c_str());

        String dnsMsg = "DNS: " + WiFi.dnsIP().toString();
        logs.info(dnsMsg.c_str());
    } else {
        logs.warning("Falha na ligação WiFi após 3 tentativas");
    }
}

void connectMQTT() { // Função para ligar ao broker MQTT
    if (WiFi.status() != WL_CONNECTED) {
        logs.warning("Não é possível ligar MQTT - Sem WiFi");
        return;
    }
    
    mqttClient.setServer(SERVER_IP, SERVER_PORT);

    int attempts = 0;
    while (!mqttClient.connected() && attempts < 3) { // Verificar se cliente MQTT está ligado
        logs.info("A ligar ao MQTT...");

        String clientId = "STM32_SENDER_";
        clientId += String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str())) {
            logs.info("MQTT ligado!");
            break;
        } else {
            String mqttMsgStr = "Falha na ligação MQTT, rc=" + String(mqttClient.state()) + " a tentar novamente em 2 segundos...";
            logs.error(mqttMsgStr.c_str());
            attempts++;
            if (attempts < 3) delay(DELAY_MQTT_CONNECTION);
        }
    }
}

void setup() { // Função de configuração
    // Série
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    logs.info("========================================");
    logs.info("   SISTEMA DE ARREFECIMENTO - GRUPO 4");
    logs.info("   STM32L476RG");
    logs.info("========================================");

    logs.initExtMem();    // Inicializar memória externa
    logs.initFile("log"); // Inicializar ficheiro de log
    csv.initFile("csv");  // Inicializar ficheiro CSV
    csv.data(CSV_HEADER); // Escrever cabeçalho CSV

    asn.readSN(); // Ler número de série do cartão SD

    sensor.initSensor(); // Inicializar sensor

    greenLed.init(LED_TEMP_GREEN); // Inicializar LED verde
    greenLed.on();                 // Ligar LED verde

    // RTC
    if (initRTC()) {
        logs.info("RTC OK");
        setRTCToCompileTime();
    } else {
        logs.error("Falha no RTC!");
    }

    Serial1.begin(SERIAL_BAUD_RATE);               // Inicializar Serial1 para comunicação
    WiFi.init(Serial1);                            // Inicializar WiFi com Serial1
    connectWiFi();                                 // Ligar ao WiFi
    mqttClient.setServer(SERVER_IP, SERVER_PORT);  // Definir servidor MQTT
    connectMQTT();                                 // Ligar ao broker MQTT
    
    logs.info("Sistema pronto!");

    // Configuração do timer para MQTT automático (APÓS conexões)
    timer3->setPrescaleFactor(TIMER3_PRESCALE); // Definir prescaler do timer
    timer3->setOverflow(TIMER3_OVERFLOW);       // Definiroverflow do timer
    timer3->attachInterrupt(sendTemperature);   // Anexar interrupção para enviar temperatura
    timer3->resume();
    
    logs.info("Timer de leituras iniciado!");
}

void loop() { // Função de ciclo principal
    // WiFi/MQTT em background - não bloqueia se falhar
    if (WiFi.status() != WL_CONNECTED) {
        // Não tenta reconectar constantemente - só no setup
    }
    
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
        // Só tenta MQTT se WiFi estiver ligado
        connectMQTT();
    }

    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    delay(DELAY_TO_STABILIZE); // Dar tempo para estabilizar

    // Controlo LED baseado na temperatura do sensor 1
    if (sensor_data.temperatureAverageSensors[0] > THIRTY_DEGREES) {
        greenLed.blink(); // Piscar LED verde se temperatura exceder 30 graus
    } else {
        greenLed.on(); // Ligar LED verde se temperatura abaixo de 30 graus
    }
}