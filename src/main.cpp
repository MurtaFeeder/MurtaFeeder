#include <Arduino.h>
#include "WiFiHandler.h"
#include "MQTTHandler.h"
#include "StatusHandler.h"
#include "DispenseHandler.h"

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

/* GLOBAL VARIABLES */
String feederNameString = getFeederName(false);
String feederNameLowerString = getFeederName(true);
const char *feederName = feederNameString.c_str();
const char *lowerFeederName = feederNameLowerString.c_str();

bool internetConnected = false;
Servo myServo;
/* ------- */

void setupFeederPins()
{
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PROX_TRIG_PIN, OUTPUT);
  pinMode(PROX_ECHO_PIN, INPUT);
  myServo.attach(SERVO_PIN);
}

TaskHandle_t mqttTaskHandle;
TaskHandle_t wifiTaskHandle;
TaskHandle_t statusTaskHandle;
TaskHandle_t dispenseTaskHandle;
QueueHandle_t dispenseQueue;

void setup()
{
  setupFeederPins();
  dispenseQueue = xQueueCreate(5, sizeof(struct DispenseMessage *));

  vTaskStartScheduler();

  xTaskCreatePinnedToCore(statusTask, "Status Task", 4096, NULL, 1, &statusTaskHandle, 1);
  xTaskCreatePinnedToCore(dispenseTask, "Dispense Task", 4096, NULL, 1, &dispenseTaskHandle, 1);
  xTaskCreatePinnedToCore(wifiTask, "WIFI Task", 4096, NULL, 1, &wifiTaskHandle, 0);
  xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 5120, NULL, 1, &mqttTaskHandle, 1);
}

void loop() {}