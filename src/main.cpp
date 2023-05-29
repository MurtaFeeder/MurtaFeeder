#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <ezButton.h>
#include "envs.h"

#define FEEDER_HEIGHT 20
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
#define TRIG_PIN 23
#define ECHO_PIN 22
#define BUTTON_PIN 33
#define BUTTON_DEBOUNCE 50
#define MIN_DISPENSE_PRESS_TIME 2000    // 2 seconds
#define MAX_DISPENSE_PRESS_TIME 4000    // 4 seconds
#define MAX_CONFIRM_DISPENSE_TIME 10000 // 10 seconds
#define LONG_PRESS_TIME 7000
#define SERVO_PIN 27
#define SERVO_STOP 90
#define SERVO_SPEED 5
#define LOW_LEVEL_INTERVAL 21600000 // 6 hours
#define POWER_LED 5
#define CONN_LED 18

#define MAX_JSON_LENGTH 200

String getFeederName(bool lowerCase)
{
  String apName = "MurtaFeeder_";
  String chipId = String(WIFI_getChipId(), HEX);
  chipId.toUpperCase();
  apName += chipId;

  if (lowerCase)
  {
    apName.toLowerCase();
  }

  return apName + "";
}

const String feederName = getFeederName(false);
const String lowerFeederName = getFeederName(true);

/* WIFI VARS */
WiFiManager wm;
/* -------- */

/* CONN LED */
int connLedState = LOW;
/* ------- */

/* STATUS VARS */
unsigned long lastStatusTime = 0;
/* low level */
unsigned long lastLowLevelTime = 0;
bool lowLevelWarningStarted = false;
/* --------- */

/* BUTTON  VARS*/
ezButton button(BUTTON_PIN);
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
unsigned long lastCountTime = 0;
bool countStarted = false;
/* ----- */

/* MQTT VARS */
WiFiClient espClient;
PubSubClient mqttClient(espClient);
String feederSubTopic;
String feederPubActionDoneTopic;
String feederPubStatusTopic;
String feederPubLowLevelTopic;
/* ------- */

/* DISPENSE TASK VARS */
Servo myServo;
QueueHandle_t dispenseQueue;
struct DispenseParams
{
  int portions;
  char message[MAX_JSON_LENGTH];
};
/* --------- */

void setupFeederPins()
{
  Serial.begin(115200);
  /* LEDS */
  pinMode(POWER_LED, OUTPUT);
  pinMode(CONN_LED, OUTPUT);
  /* PROXIMITY SENSOR */
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  /* ----- */
  /* SWTICH PUSH BUTTON */
  button.setDebounceTime(BUTTON_DEBOUNCE);
  button.setCountMode(COUNT_RISING);
  /* ----- */
  /* SERVO */
  myServo.attach(SERVO_PIN);
  /* ----- */

  /* TURN ON RED LIGHT */
  digitalWrite(POWER_LED, HIGH);
}

void initWifiManager()
{
  bool res;

  res = wm.autoConnect(feederName.c_str(), "MurtaFeeder1234");

  if (!res)
  {
#ifdef DEBUG
    Serial.println("Failed to connect");
#endif
    ESP.restart();
  }
  else
  {
#ifdef DEBUG
    Serial.println("WiFi Connected");
#endif
    // if you get here you have connected to the WiFi
  }
}

void dispenseTask(void *parameter)
{
  DispenseParams *params;

  while (true)
  {
    xQueueReceive(dispenseQueue, &(params), portMAX_DELAY);
    int portions = params->portions;

    while (portions > 0)
    {
      myServo.write(0);
      delay(700);
      myServo.write(SERVO_STOP);
      portions--;
      delay(250);
    }

    char doneMessage[MAX_JSON_LENGTH];

    strcpy(doneMessage, params->message);

    mqttClient.publish(feederPubActionDoneTopic.c_str(), doneMessage);
  }
  vTaskDelete(NULL);
}

void mqttConsumer(char *topic, byte *payload, unsigned int length)
{
  String message(payload, length);
  // NO TOPIC CHECK (at the moment), ONLY SUSCRIBED TO 1 TOPIC
  StaticJsonDocument<96> doc;

  DeserializationError error = deserializeJson(doc, payload, length);

  if (error)
  {
#ifdef DEBUG
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
#endif
    return;
  }

  int actionID = doc["actionId"];

  switch (actionID)
  {
  case 1:
    int portions = doc["portions"];

    DispenseParams *dispenseParams = new DispenseParams;
    dispenseParams->portions = portions;
    strcpy(dispenseParams->message, message.c_str());
    xQueueSend(dispenseQueue, &dispenseParams, portMAX_DELAY);
  }
}

void connectMQTT()
{
  while (!mqttClient.connected())
  {
#ifdef DEBUG
    Serial.print("[MQTT] Connecting to MQTT broker...");
#endif
    if (mqttClient.connect(feederName.c_str(), MQTT_USER, MQTT_PSWD))
    {
#ifdef DEBUG
      Serial.println("\n[MQTT] Connected to MQTT broker!");
#endif
      if (connLedState == LOW)
      {

        digitalWrite(CONN_LED, HIGH);
        connLedState = HIGH;
      }
      mqttClient.subscribe(feederSubTopic.c_str());
    }
    else
    {
      if (connLedState == HIGH)
      {
        connLedState = LOW;
      }
      else
      {
        // connLedState == LOW
        connLedState = HIGH;
      }
      digitalWrite(CONN_LED, connLedState);
    }
  }
}

void loadMQTTTopics()
{
  String baseTopic = "murta/feeder/" + lowerFeederName;

  feederSubTopic = baseTopic + "/action";
  feederPubActionDoneTopic = baseTopic + "/done";
  feederPubStatusTopic = baseTopic + "/status";
  feederPubLowLevelTopic = baseTopic + "/low";
#ifdef DEBUG
  Serial.printf("[MQTT] Topics Loaded:\n*Sub Topic:%s\n*Pub Action Done Topic:%s\n*Pub Status Topic:%s\n", feederSubTopic.c_str(), feederPubActionDoneTopic.c_str(), feederPubStatusTopic.c_str());
#endif
}

void initMQTT()
{
  loadMQTTTopics();
  mqttClient.setCallback(mqttConsumer);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
}

void setup()
{
  setupFeederPins();
  initWifiManager();
  initMQTT();

  dispenseQueue = xQueueCreate(10, sizeof(sizeof(DispenseParams *)));

  xTaskCreate(dispenseTask, "DispenseTask", 2048, NULL, 1, NULL);
}

void getFeederStatus()
{
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastStatusTime;

  // When low level notification is required, we can assume that the check will be done every 5 seconds
  if (timeDiff < 4900)
  {
    return;
  }

  lastStatusTime = millis();
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  if (duration >= 60000)
  {
    duration = 0;
  }

  float distanceCm = duration * SOUND_SPEED / 2;
  if (distanceCm > 30)
  {
    distanceCm = 0;
  }

  float capacity = (FEEDER_HEIGHT - distanceCm) / 20;

  if (capacity > 1)
  {
    capacity = 1;
  }
  else if (capacity < 0)
  {
    capacity = 0;
  }

  StaticJsonDocument<16> doc;
  doc["capacity"] = capacity;
  String json;
  serializeJson(doc, json);

  mqttClient.publish(feederPubStatusTopic.c_str(), json.c_str());

  const bool isLowLevel = capacity < 0.2;
  /* LOW LEVEL START */
  // First warning -> start process
  if (isLowLevel && !lowLevelWarningStarted && mqttClient.publish(feederPubLowLevelTopic.c_str(), json.c_str()))
  {
    lowLevelWarningStarted = true;
    lastLowLevelTime = currentTime;
    Serial.printf("Low level message sent %lu\n", currentTime);
  }
  // Process already started & still low
  else if (isLowLevel && lowLevelWarningStarted && (currentTime - lastLowLevelTime) >= LOW_LEVEL_INTERVAL && mqttClient.publish(feederPubLowLevelTopic.c_str(), json.c_str()))
  {
    lastLowLevelTime = currentTime;
    Serial.printf("Low level message sent %lu\n", currentTime);
  }
  // Cancel process
  else if (lowLevelWarningStarted && !isLowLevel)
  {
    lowLevelWarningStarted = false;
    lastLowLevelTime = 0;
  }
  /* LOW LEVEL END */
}

void resetConfirmState()
{
  button.resetCount();
  lastCountTime = 0;
  countStarted = false;
}

void handleButtonPress()
{
  unsigned long confirmDuration = millis() - lastCountTime;

  if (countStarted && confirmDuration > MAX_CONFIRM_DISPENSE_TIME)
  {
#ifdef DEBUG
    Serial.println("[BUTTON HANDLER] Too much to confirm. Reset counter.");
#endif
    resetConfirmState();
  }

  button.loop();
  if (button.isPressed())
  {
    pressedTime = millis();
  }
  if (button.isReleased())
  {
    releasedTime = millis();

    unsigned long pressDuration = releasedTime - pressedTime;

    /* DISPENSE PRESS */
    if (MIN_DISPENSE_PRESS_TIME <= pressDuration && pressDuration <= MAX_DISPENSE_PRESS_TIME && confirmDuration <= MAX_CONFIRM_DISPENSE_TIME)
    {
      int portions = (int)(button.getCount() - 1);
      if (portions != 0)
      {
#ifdef DEBUG
        Serial.printf("[BUTTON HANDLER] Dispense confirmed. Dispensing %d portions.\n", portions);
#endif
        DispenseParams *dispenseParams = new DispenseParams;
        dispenseParams->portions = portions;
        String message = "{\"actionId\":1,\"portions\":" + String(portions) + ",\"feederId\":\"" + lowerFeederName + "\",\"origin\":\"MANUAL\"}";
        strcpy(dispenseParams->message, message.c_str());
        xQueueSend(dispenseQueue, &dispenseParams, portMAX_DELAY);
      }
      resetConfirmState();
    }
    /* RESET PRESS */
    else if (pressDuration >= LONG_PRESS_TIME)
    {
      // if long press remove wifi credentials & restart
      wm.resetSettings();
      ESP.restart();
    }
    /* COUNT PRESS */
    else
    {
      lastCountTime = releasedTime;
      countStarted = true;

#ifdef DEBUG
      Serial.printf("[BUTTON HANDLER] Current counter %lu\n", button.getCount());
#endif
    }
  }
}

void loop()
{
  if (!mqttClient.connected())
  {
#ifdef DEBUG
    Serial.println("MQTT Disconnected : Trying connection");
#endif
    connectMQTT();
  }
  getFeederStatus();
  handleButtonPress();
  mqttClient.loop();
}