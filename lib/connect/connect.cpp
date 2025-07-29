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
  int tentativas = 0;
  unsigned long startTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && tentativas < 3 && (millis() - startTime) < 6000)
  {
    WiFi.begin(sv.get_ssid(), sv.get_password());
    tentativas++;
    delay(2000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    logs.info("WiFi ligado");
  } else {
    logs.warning("WiFi falhou");
  }
}

// Ligar MQTT
void connectMQTT()
{
  mqttClient.setServer(sv.get_ip(), sv.get_port());
  mqttClient.setCallback(callback);

  while (!mqttClient.connected())
  {
    // A ligar MQTT

    String clientId = "STM32_RECEIVER_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      mqttClient.subscribe("stm32/topic");
    }
    else
    {
      // MQTT falhou
      delay(2000);
    }
  }
}