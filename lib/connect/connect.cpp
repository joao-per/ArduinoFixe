#include "connect.hpp"
#include "logs.hpp"

// Variáveis globais
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
server sv(SERVER_SSID, SERVER_PASSWORD, SERVER_IP, SERVER_PORT);

extern ExtMEM logs;

// Receber MQTT
void callback(char *topic, byte *payload, unsigned int length)
{
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  logs.info("MQTT message received");
  logs.debug(topic);
  logs.debug(message);
}

// Ligar WiFi
void connectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    logs.info("Connecting to WiFi...");
    WiFi.begin(sv.get_ssid(), sv.get_password());
    delay(5000);
  }

  logs.info("WiFi connected");
  logs.debug(WiFi.localIP().toString().c_str());
  logs.debug("Gateway:");
  logs.debug(WiFi.gatewayIP().toString().c_str());
  logs.debug("DNS:");
  logs.debug(WiFi.dnsIP().toString().c_str());
}

// Ligar MQTT
void connectMQTT()
{
  mqttClient.setServer(sv.get_ip(), sv.get_port());
  mqttClient.setCallback(callback);

  while (!mqttClient.connected())
  {
    logs.info("Connecting to MQTT...");

    String clientId = "STM32_RECEIVER_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      logs.info("MQTT connected!");
      mqttClient.subscribe("stm32/topic");
      logs.info("Subscribed to stm32/topic");
    }
    else
    {
      logs.error("MQTT failed. Retrying in 2s...");
      char rcStr[10];
      itoa(mqttClient.state(), rcStr, 10);
      logs.debug(rcStr);
      delay(2000);
    }
  }
}