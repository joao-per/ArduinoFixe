#ifndef CONFIG_HPP
#define CONFIG_HPP

// Setup Libraries
#include <Arduino.h>
#include <string>

// Forward declarations
class ExtMEM;

// ========== PINOUT STM32L476RG ==========
// DHT11 Sensor
#define DHTPIN PC1                  // Sensor pin
#define DHTTYPE DHT11               // Sensor type

// LEDs
#define LED_PIN PA5                 // LED principal
#define LED_TEMP_GREEN PC12         // LED verde (temperatura OK)
#define LED_TEMP_RED PC15           // LED vermelho (temperatura alta)
#define LED_SENSOR1 PA8             // LED sensor 1
#define LED_SENSOR2 PA9             // LED sensor 2  
#define LED_SENSOR3 PA10            // LED sensor 3
#define LED_SENSOR4 PA11            // LED sensor 4
#define LED_WIFI PB0                // LED estado WiFi
#define LED_SD PB1                  // LED estado SD

// Buttons
#define BTN_USER PC13               // Botão USER da placa
#define BTN_EMERGENCY PA0           // Botão emergência (externo)
#define BTN_MODE PA1                // Botão modo (externo)

// SPI for SD Card
static constexpr int SCK_PIN = PA5;
static constexpr int MISO_PIN = PA6;
static constexpr int MOSI_PIN = PA7;
static constexpr int uSD_CS_PIN = PB6;

// ========== COMMUNICATION ==========
#define SERIAL_BAUD_RATE 115200
#define BUTTON_DEBOUNCE_DELAY 50    // milliseconds

// ========== SYSTEM PARAMETERS ==========
#define NUM_SENSORS 4               // Número de sensores (virtuais)
#define NUM_READINGS 10             // Número de leituras para média
#define SENSOR_READ_INTERVAL 2000   // ms entre debug output (every 2 seconds)
#define PHYSICAL_READ_INTERVAL 100  // ms between physical sensor reads (10x per second)
#define MQTT_PUBLISH_INTERVAL 5000  // ms entre publicações
#define DEFAULT_TEMP_MIN 20.0       // °C
#define DEFAULT_TEMP_MAX 45.0       // °C

// ========== WIFI & MQTT ==========
#define SERVER_SSID "NOS_Internet_E345"
#define SERVER_PASSWORD "11070017"
#define SERVER_IP "192.168.1.178"
#define SERVER_PORT 1883
#define MQTT_CLIENT_ID "STM32_Cooling_System"

// MQTT Topics - Publicação
#define TOPIC_BASE "sala_maquinas/"
#define TOPIC_TEMP_PREFIX TOPIC_BASE "sensor"
#define TOPIC_TEMP_SUFFIX "/temperatura"
#define TOPIC_HUM_PREFIX TOPIC_BASE "sensor"
#define TOPIC_HUM_SUFFIX "/humidade"
#define TOPIC_SENSOR_STATUS TOPIC_BASE "sensores/estado"
#define TOPIC_SD_STATUS TOPIC_BASE "memoria/estado"
#define TOPIC_SYSTEM_LOG TOPIC_BASE "sistema/log"

// MQTT Topics - Subscrição
#define TOPIC_SETPOINT_MIN TOPIC_BASE "config/temp_min"
#define TOPIC_SETPOINT_MAX TOPIC_BASE "config/temp_max"
#define TOPIC_INTERVAL TOPIC_BASE "config/intervalo"
#define TOPIC_MODE TOPIC_BASE "controlo/modo"
#define TOPIC_EMERGENCY TOPIC_BASE "controlo/emergencia"
#define TOPIC_ACTUATOR_CMD TOPIC_BASE "controlo/atuadores"

// ========== FILES & LOGS ==========
static const std::string CONFIG_FILENAME = "config.json";
static const std::string CONFIG_PATH = "/";
static const std::string LOG_FILENAME = "system";
static const std::string CSV_FILENAME = "temperatura";
static const char *CSV_HEADER = "timestamp,device,status,temperature";
static const std::string LOG_PATH = "";
static constexpr bool IS_RTC_ENABLED = true;
static constexpr bool IS_SERIAL_PRINT = true;
static constexpr bool IS_DEBUG_LOG = true;
static constexpr uint32_t MAX_FILE_SIZE = 1048576; // 1MB

// ========== TIMER CONFIG ==========
#define TIME_FREQUENCY 1            // 1 Hz (1 segundo)
#define PRESCALE_FACTOR 8000

// ========== THRESHOLDS ==========
#define TEMP_CRITICAL_HIGH 50.0     // °C - temperatura crítica
#define TEMP_WARNING_HIGH 30.0      // °C - aviso de temperatura alta
#define HUMIDITY_LOW 20.0           // % - humidade baixa
#define HUMIDITY_HIGH 80.0          // % - humidade alta

// ========== RTC ==========
static constexpr bool SET_RTC_BOOT = true;

// ========== GLOBAL OBJECTS DECLARATIONS ==========
// Estas declarações devem estar no main.cpp ou num ficheiro separado
extern ExtMEM logs;
extern ExtMEM csv;
extern ExtMEM asn;

// Estruturas de dados
struct configData {
    char asn[15];
    char sn[15];
    char hw[5];
    char fw[5];
};

extern configData config_data;

// DateTime is now defined in set_rtc.hpp

// Buffers globais
extern char floatString_temperature[10];
extern char floatString_humidity[10];
extern char msg_temperature[150];
extern char msg_humidity[150];
extern char time_string[20];

// Variáveis de tempo
extern unsigned long currentMillis;
extern unsigned long lastRead;
extern const unsigned long interval;

// Arrays para médias
extern float tempReadings[NUM_READINGS];
extern float humReadings[NUM_READINGS];
extern int readingIndex;
extern int readingCount;

#endif // CONFIG_HPP