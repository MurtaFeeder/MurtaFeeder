#include <WiFiManager.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include "envs.h"
#include "dimensions.h"
#include "actions.h"

#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

const int TRIG_PIN = 13;
const int ECHO_PIN = 12;

const int SERVO_PIN = 27;
const int SERVO_STOP = 90;
int speed = 5;

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

Servo myServo;

String getFeederName()
{
  String apName = "MurtaFeeder_";
  String chipId = String(WIFI_getChipId(), HEX);
  chipId.toUpperCase();
  apName += chipId;

  return apName + "";
}

const String feederName = getFeederName();

void setupFeederPins()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);  // Sets the echoPin as an Input
  myServo.attach(SERVO_PIN);
}

void initWifiManager()
{
  WiFiManager wm;
  // wm.resetSettings();
  bool res;

  res = wm.autoConnect(feederName.c_str(), "MurtaFeeder1234");

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("WiFi Connected");
  }
}

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String feederTopic;

void dispense(int portions, StaticJsonDocument<64> event)
{

  while (portions > 0)
  {
    myServo.write(0);
    delay(660);
    myServo.write(SERVO_STOP);
    portions--;
    delay(250);
  }

  String topic = feederTopic + "/done";

  String message;
  serializeJson(event, message);

  mqttClient.publish(topic.c_str(), message.c_str());
}

void mqttConsumer(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // NO TOPIC CHECK (at the moment), ONLY SUSCRIBED TO 1 TOPIC
  StaticJsonDocument<64> doc;

  DeserializationError error = deserializeJson(doc, payload, length);

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  int actionID = doc["actionID"];

  switch (actionID)
  {
  case DISPENSE:
    int portions = doc["portions"];
    dispense(portions, doc);
  }
}

void connectMQTT()
{
  int attempts = 5;
  while (attempts > 0 && !mqttClient.connected())
  {
    if (!mqttClient.connect(feederName.c_str(), MQTT_USER, MQTT_PSWD))
    {
      attempts--;
      delay(5000);
    }
  }

  if (attempts == 0)
  {
    ESP.restart();
  }

  Serial.println("MQTT Connected");

  feederTopic = "murta/feeder/" + feederName;

  String topic = feederTopic + "/action";

  Serial.print("Suscribed to ");
  Serial.print(topic);

  boolean res = mqttClient.subscribe(topic.c_str());
  Serial.print(" (Sub Status:");
  Serial.print(res);
  Serial.println(")");

  mqttClient.setCallback(mqttConsumer);
}

void initMQTT()
{

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
}

void setup()
{
  delay(10000);
  setupFeederPins();
  initWifiManager();
  configTime(0, 0, "pool.ntp.org");
  initMQTT();
}

bool status_flag = true;

void getFeederStatus()
{
  if (!status_flag)
  {
    status_flag = true;
    return;
  }
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distanceCm = duration * SOUND_SPEED / 2;

  // Impossible to get more than 20cm (30 for margin)
  if (distanceCm > 30)
  {
    distanceCm = 0;
  }

  float capacity = (FEEDER_STORE_HEIGHT - distanceCm) / 20;

  StaticJsonDocument<32> doc;

  doc["timestamp"] = getTime();
  doc["capacity"] = capacity;

  String json;
  serializeJson(doc, json);

  String topic = feederTopic + "/status";

  Serial.print("Feeder Status sent to ");
  Serial.println(topic);
  mqttClient.publish(topic.c_str(), json.c_str());

  status_flag = false;
}

void loop()
{
  if (!mqttClient.connected())
  {
    Serial.println("MQTT Disconnected : Trying connection");
    connectMQTT();
  }
  mqttClient.loop();
  getFeederStatus();
  delay(2000);
}