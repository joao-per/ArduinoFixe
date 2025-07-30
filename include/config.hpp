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
#define LED_TEMP_GREEN PC12         // LED temperatura verde
#define LED_WIFI PB0                // LED estado WiFi
#define LED_SD PB1                  // LED estado SD

// SPI for SD Card
static constexpr int SCK_PIN = PA5;
static constexpr int MISO_PIN = PA6;
static constexpr int MOSI_PIN = PA7;
static constexpr int uSD_CS_PIN = PB6;

// ========== COMMUNICATION ==========
#define SERIAL_BAUD_RATE 115200
#define BUTTON_DEBOUNCE_DELAY 50    // milliseconds

// ========== SYSTEM PARAMETERS ==========
// Sensor variables
static const int SAMPLES_AVERAGE = 10;    // Number of samples for average calculation
static const int NUMBER_OF_SENSORS = 4;   // Number of sensors
static const float THIRTY_DEGREES = 30.0; // Thirty degrees Celsius

// Delays
static const int DELAY_WIFI_CONNECTION = 5000; // Delay for WiFi connection
static const int DELAY_MQTT_CONNECTION = 2000; // Delay for MQTT connection
static const int DELAY_TO_STABILIZE = 2000;    // Delay to stabilize

// Timer properties
static constexpr uint32_t TIMER3_PRESCALE = 7200;  // Prescale factor
static constexpr uint32_t TIMER3_OVERFLOW = 20000; // Overflow value

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

// Tópicos de controlo removidos

// ========== FILES & LOGS ==========
static const std::string CONFIG_FILENAME = "config.json";
static const std::string CONFIG_PATH = "/";
static const std::string LOG_FILENAME = "system";
static const std::string CSV_FILENAME = "temperatura";
static const char *CSV_HEADER = "timestamp;device;status;temperature";
static const std::string LOG_PATH = "";
static constexpr bool IS_RTC_ENABLED = true;
static constexpr bool IS_SERIAL_PRINT = true;
static constexpr bool IS_DEBUG_LOG = true;
static constexpr uint32_t MAX_FILE_SIZE = 1048576; // 1MB

// ========== THRESHOLDS ==========
#define TEMP_WARNING_HIGH 30.0      // °C - aviso de temperatura alta

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

struct sensorData // Structure to hold sensor data
{
    float temperatureSensors[4];        // Current temperature
    float temperatureAverageSensors[4]; // Average temperature reading
    float humiditySensors[4];           // Current humidity
    float humidityAverageSensors[4];    // Average humidity reading
};

extern configData config_data;
extern sensorData sensor_data;

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

// Arrays removidos

#endif // CONFIG_HPP