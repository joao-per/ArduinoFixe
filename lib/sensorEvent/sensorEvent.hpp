#ifndef SENSOREVENT_HPP
#define SENSOREVENT_HPP

// Bibliotecas do framework
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>

// Includes locais
#include "logs.hpp"
#include "config.hpp"

class sensorEvent {
public:
    // Atributos públicos
    sensorEvent();

    void initSensor(); // Inicializar sensor

    void getTemperature(); // Obter temperatura do sensor

    void getHumidity(); // Obter humidade do sensor

    void getTemperatureAverage(); // Obter temperatura média dos sensores

    void getHumidityAverage(); // Obter humidade média dos sensores

private:
    // Atributos privados
    void writeTemperatureAverage(); // Escrever dados de temperatura média nos logs

    void writeHumidityAverage(); // Escrever dados de humidade média nos logs
};

extern ExtMEM logs;

#endif // SENSOREVENT_HPP