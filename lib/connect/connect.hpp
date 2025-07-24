#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <WiFiEspAT.h>
#include <PubSubClient.h>

#include <config.hpp>
#include "server.hpp"

// External declarations of WiFi and MQTT clients
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;
extern server sv;

void callback(char *topic, byte *payload, unsigned int length);
void connectWiFi();
void connectMQTT();

#endif // CONNECT_HPP