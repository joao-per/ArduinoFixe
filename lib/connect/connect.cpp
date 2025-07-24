#include "connect.hpp"

// Global variable definitions
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
server sv(SERVER_SSID, SERVER_PASSWORD, SERVER_IP, SERVER_PORT);

// MQTT Receive
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// MQTT Connect WiFi
void connectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(sv.get_ssid(), sv.get_password());
    delay(5000);
  }

  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
}

// MQTT Connect
void connectMQTT()
{
  mqttClient.setServer(sv.get_ip(), sv.get_port());
  mqttClient.setCallback(callback);

  while (!mqttClient.connected())
  {
    Serial.println("Connecting to MQTT...");

    String clientId = "STM32_RECEIVER_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("MQTT connected!");
      mqttClient.subscribe("stm32/topic");
      Serial.println("Subscribed to stm32/topic");
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" trying again in 2 seconds...");
      delay(2000);
    }
  }
}