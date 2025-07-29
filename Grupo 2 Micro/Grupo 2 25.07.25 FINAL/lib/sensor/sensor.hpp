#include "extMem.hpp"
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include "config.hpp"

class Sensor {
public:
    Sensor();
    void begin();
    void ler();
    //DHT_Unified dht(DHTPIN, DHTTYPE);

private:
    
};
