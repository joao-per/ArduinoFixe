#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <WiFiEspAT.h>
#include <PubSubClient.h>

#include <config.hpp>
#include "server.hpp"

// Create WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

server sv(SERVER_SSID, SERVER_PASSWORD, SERVER_IP, SERVER_PORT);

void callback(char *topic, byte *payload, unsigned int length);
void connectWiFi();
void connectMQTT();

#endif // CONNECT_HPP