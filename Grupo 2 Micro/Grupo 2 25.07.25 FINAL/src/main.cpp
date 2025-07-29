

#include <Arduino.h>
#include "extMem.hpp"
#include "sensor.hpp"
#include "set_rtc.hpp"
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include "led.hpp"
// Local Includes
#include "set_rtc.hpp"
// #include "config.hpp"

char time_string[20];
char msg1[10];
char msg2[10];
char msg3[10];
char msg4[10];

Led led;

// DHT_Unified dht(DHTPIN, DHTTYPE);

HardwareSerial Serial1(PA10, PA9); // RX, TX

// WiFi credentials
const char *ssid = "NOS_Internet_E345";
const char *password = "11070017";

// MQTT broker
const char *mqtt_server = "192.168.1.178"; // Your PC's IP
const int mqtt_port = 1883;

// Create WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void connectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    delay(5000);
  }
  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
}

void connectMQTT()
{
  mqttClient.setServer(mqtt_server, mqtt_port);

  while (!mqttClient.connected())
  {
    Serial.println("Connecting to MQTT...");

    String clientId = "STM32_SENDER_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("MQTT connected!");
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

// Variables
char floathave[10];

char msg[100];

/* Class Definition*/
struct configData config_data = {0};

ExtMEM logs;
ExtMEM csv;
ExtMEM asn;

Sensor sensor;

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);

  Serial.println("Initializing ESP8266...");
  led.iniciar();

  // Initialize AT library
  WiFi.init(Serial1);

  connectWiFi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  connectMQTT();
  Serial.begin(SERIAL_BAUDRATE);
  pinMode(LEDGREEN, OUTPUT);

  logs.initExtMem();
  logs.initFile("log");
  csv.initFile("csv");
  csv.data(CSV_HEADER);

  logs.debug("This is a DEBUG message");
  logs.info("This is a INFO message");
  logs.warning("This is a WARNING message");
  logs.error("This is a ERROR message");

  csv.data("1,2,3");
  csv.data("4,5,6");

  asn.readSN();

  // Initialize the RTC
  if (!initRTC())
  {
    Serial.println("RTC initialization failed!");
    return;
  }

  // set_rtc_time(2025, 7, 11, 12, 34, 56); // Set RTC to a specific date and time

  Serial.println("RTC Initialized!");

  delay(1000);
}

void loop()
{
  led.executar();
  sensor.begin();
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  delay(2000); // Give time to stabilize

  if (!mqttClient.connected())
  {
    connectMQTT();
  }

  mqttClient.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 5000)
  {
    lastMsg = millis();
    dtostrf(config_data.S1, 2, 2, msg1);
    mqttClient.publish("temperatura/s1", msg1);

    dtostrf(config_data.S2, 2, 2, msg2);
    mqttClient.publish("temperatura/s2", msg2);

    dtostrf(config_data.S3, 2, 2, msg3);
    mqttClient.publish("temperatura/s3", msg3);

    dtostrf(config_data.S4, 2, 2, msg4);
    mqttClient.publish("temperatura/s4", msg4);
  }
  delay(1000);
}
