#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "MQTTEnvs.h"

#define MAX_JSON_LENGTH 256

struct DispenseMessage
{
	int portions;
	char message[MAX_JSON_LENGTH];
};

// MQTT loop
void mqttTask(void *pvParameters);

// Publish MQTT message
void publishMessage(const char *topic, const char *message);

#endif // MQTT_HANDLER_H