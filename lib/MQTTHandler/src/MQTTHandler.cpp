#include "MQTTHandler.h"

WiFiClient espClient;
PubSubClient client(espClient);

/* SHARED VARIABLES */
extern const char *lowerFeederName;
extern bool internetConnected;
extern QueueHandle_t dispenseQueue;
/* ------ */

char subTopic[41];

// Callback function for receiving MQTT messages
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
	char message[MAX_JSON_LENGTH];

	strncpy(message, (char *)payload, length);
	message[length] = '\0';

	Serial.printf("[MQTT CALLBACK] Message received on topic: %s\n%s\n", topic, message);

	StaticJsonDocument<96> doc;

	DeserializationError error = deserializeJson(doc, payload, length);

	if (error)
	{
		Serial.printf("[MQTT CALLACK] deserializeJson() failed: %s\n", error.c_str());
		return;
	}

	int actionID = doc["actionId"];

	switch (actionID)
	{
	case 1:
		int portions = doc["portions"];
		// dispense(portions, doc);
		DispenseMessage *dispenseMessage = new DispenseMessage;
		dispenseMessage->portions = portions;
		strncpy(dispenseMessage->message, message, MAX_JSON_LENGTH);
		xQueueSend(dispenseQueue, &dispenseMessage, portMAX_DELAY);
	}
}

// Connect to MQTT broker
void connectMQTT()
{
	Serial.print("[MQTT TASK] Connecting to MQTT broker...");
	while (!client.connected())
	{

		if (client.connect(lowerFeederName, MQTT_USER, MQTT_PSWD))
		{
			Serial.println("\n[MQTT TASK] Connected to MQTT broker!");
			client.subscribe(subTopic);
		}
		else
		{
			Serial.print("[MQTT TASK] Connecting to MQTT broker...");
			delay(5000);
		}
	}
}

void loadSubTopics()
{
	sprintf(subTopic, "murta/feeder/%s/action", lowerFeederName);
}

// MQTT task
void mqttTask(void *pvParameters)
{

	Serial.println("[MQTT TASK] STARTED");

	// load feeder name, sub & pub topics
	loadSubTopics();

	// Set MQTT server and port
	client.setServer(MQTT_SERVER, MQTT_PORT);

	// Set MQTT callback function
	client.setCallback(mqttCallback);

	delay(1000);

	while (1)
	{
		while (!internetConnected)
		{
			delay(2000);
		}
		if (!client.connected())
		{
			connectMQTT();
		}

		client.loop();
	}
}

void publishMessage(const char *topic, const char *message)
{
	client.publish(topic, message);
}