#include "sensor.hpp"

ExtMEM logs_sensor;
DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;
char floatString[10];
char msg_storage[64];

static float temperatureReadings[10] = {0}; // Array para armazenar as 10 últimas leituras
static int temperatureIndex = 0;            // Índice circular
static float temperatureSum = 0.0;          // Somatório das temperaturas
static int readingsCount = 0;

Sensor::Sensor() {};

/*void Sensor::begin()
{

    dht.begin();

    char floatString[10];
    char msg[64];

    //Serial.println("ARCIP");

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
        Serial.println(F("Error reading temperature!"));
    }
    else   logs_sensor.initExtMem(); // Inisensor.begin();cializa o sistema de logs
    logs_sensor.initFile("sensor_logs"); // Cria ou abre o arquivo de logs
    {
        dtostrf(event.temperature, 2, 2, floatString);
        sprintf(msg, "Temperature = %sºC", floatString);
        logs_sensor.debug(msg);
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
        Serial.println(F("Error reading humidity!"));
    }
    else
    {
        dtostrf(event.relative_humidity, 2, 0, floatString);
        sprintf(msg, "Humidity = %s%%", floatString);
        logs_sensor.debug(msg);
    }

    delay(1000);
}
*/
void Sensor::begin()
{
    logs_sensor.initExtMem();            // Inicializa o sistema de logs
    logs_sensor.initFile("sensor_logs"); // Cria ou abre o arquivo de logs

    dht.begin();

    // Quantidade de leituras realizadas

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature))
    {
        // Remove a leitura mais antiga do somatório
        temperatureSum -= temperatureReadings[temperatureIndex];
        // Adiciona a nova leitura ao array e ao somatório
        temperatureReadings[temperatureIndex] = event.temperature;
        temperatureSum += event.temperature;

        // Atualiza o índice circular
        temperatureIndex = (temperatureIndex + 1) % 10;
        if (readingsCount < 10)
            readingsCount++;

        // Calcula a média das últimas 10 leituras (ou menos, se ainda não houver 10)
        float averageTemperature = temperatureSum / readingsCount;

        // Calcula os valores dos sensores
        config_data.S1 = averageTemperature;
        config_data.S2 = averageTemperature + 0.25;
        config_data.S3 = averageTemperature - 0.25;
        config_data.S4 = averageTemperature + (2 * 0.25);

        // Log dos sensores
        dtostrf(config_data.S1, 2, 2, floatString);
        sprintf(msg_storage, "%s", floatString);
        logs_sensor.debug(msg_storage);

        dtostrf(config_data.S2, 2, 2, floatString);
        sprintf(msg_storage, "%s", floatString);
        logs_sensor.debug(msg_storage);

        dtostrf(config_data.S3, 2, 2, floatString);
        sprintf(msg_storage, "%s", floatString);
        logs_sensor.debug(msg_storage);

        dtostrf(config_data.S4, 2, 2, floatString);
        sprintf(msg_storage, "%s", floatString);
        logs_sensor.debug(msg_storage);
    }

    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity))
    {
        dtostrf(event.relative_humidity, 2, 2, floatString);
        sprintf(msg_storage, "Humidity = %s%%", floatString);
        logs_sensor.debug(msg_storage);
    }

    delay(1000);
}