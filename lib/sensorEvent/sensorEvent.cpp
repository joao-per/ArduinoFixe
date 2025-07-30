// Includes locais
#include "sensorEvent.hpp"

// Definição de classes
DHT_Unified dht(DHTPIN, DHTTYPE);
sensors_event_t event;

// Definição de variáveis
char convertFloatStringTemp[50];    // String para dados de temperatura convertidos
char convertFloatStringHum[10];     // String para dados de humidade convertidos
char resultTemp[200];               // String para dados de temperatura resultantes
char resultHum[200];                // String para dados de humidade resultantes
float sumTemperatureAverageSensors; // Variável para soma da temperatura média
float sumHumidityAverageSensors;    // Variável para soma da humidade média
float offsetTempSensors = 0.25;     // Offset para sensores de temperatura
float offsetHumSensors = 2;         // Offset para sensores de humidade

// Estruturas globais
struct sensorData sensor_data = {0};

// Construtor da classe
sensorEvent::sensorEvent() {
}

void sensorEvent::initSensor() { // Inicializar o sensor DHT
    dht.begin();
}

void sensorEvent::getTemperature() { // Obter temperatura do sensor
    dht.temperature().getEvent(&event); // Ler dados de temperatura

    if (isnan(event.temperature)) { // Verificar falha na leitura
        for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
            sensor_data.temperatureSensors[i] = NAN;
        }
        logs.error("Falha na leitura da temperatura!");
    } else {
        for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
            sensor_data.temperatureSensors[i] = event.temperature;
        }
    }
}

void sensorEvent::getHumidity() { // Obter humidade do sensor
    dht.humidity().getEvent(&event);

    if (isnan(event.relative_humidity)) {
        for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
            sensor_data.humiditySensors[i] = NAN;
        }
        logs.error("Falha na leitura da humidade!");
    } else {
        for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
            sensor_data.humiditySensors[i] = event.relative_humidity;
        }
    }
}

void sensorEvent::getTemperatureAverage() { // Obter temperatura média dos sensores
    sumTemperatureAverageSensors = 0;

    for (int i = 1; i <= SAMPLES_AVERAGE; i++) {
        getTemperature();
        sumTemperatureAverageSensors += sensor_data.temperatureSensors[0];
    }

    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        sensor_data.temperatureAverageSensors[i] = sumTemperatureAverageSensors / SAMPLES_AVERAGE; // Calcular temperatura média
    }

    sensor_data.temperatureAverageSensors[1] += offsetTempSensors;                                                 // Ajustar temperatura para sensor 2
    sensor_data.temperatureAverageSensors[2] -= offsetTempSensors;                                                 // Ajustar temperatura para sensor 3
    sensor_data.temperatureAverageSensors[3] = sensor_data.temperatureAverageSensors[3] + (2 * offsetTempSensors); // Ajustar temperatura para sensor 4

    writeTemperatureAverage(); // Escrever dados de temperatura média nos logs
}

void sensorEvent::getHumidityAverage() { // Obter humidade média dos sensores
    sumHumidityAverageSensors = 0;

    for (int i = 1; i <= SAMPLES_AVERAGE; i++) {
        getHumidity();
        sumHumidityAverageSensors += sensor_data.humiditySensors[0];
    }

    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        sensor_data.humidityAverageSensors[i] = sumHumidityAverageSensors / SAMPLES_AVERAGE; // Calcular humidade média
    }

    sensor_data.humidityAverageSensors[1] += offsetHumSensors;                                              // Ajustar humidade para sensor 2
    sensor_data.humidityAverageSensors[2] -= offsetHumSensors;                                              // Ajustar humidade para sensor 3
    sensor_data.humidityAverageSensors[3] = sensor_data.humidityAverageSensors[3] + (2 * offsetHumSensors); // Ajustar humidade para sensor 4

    writeHumidityAverage(); // Escrever dados de humidade média nos logs
}

void sensorEvent::writeTemperatureAverage() { // Escrever dados de temperatura média nos logs
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        dtostrf(sensor_data.temperatureAverageSensors[i], 4, 2, convertFloatStringTemp);
        String tempMsg = "Temperatura Média Sensor ";
        tempMsg += String(i + 1);
        tempMsg += " = ";
        tempMsg += convertFloatStringTemp;
        tempMsg += " C";
        logs.info(tempMsg.c_str());
    }
}

void sensorEvent::writeHumidityAverage() { // Escrever dados de humidade média nos logs
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        dtostrf(sensor_data.humidityAverageSensors[i], 2, 0, convertFloatStringHum);
        String humMsg = "Humidade Média Sensor ";
        humMsg += String(i + 1);
        humMsg += " = ";
        humMsg += convertFloatStringHum;
        humMsg += "%";
        logs.info(humMsg.c_str());
    }
}