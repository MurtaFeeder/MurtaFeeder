

#include "StatusHandler.h"
#include "MQTTHandler.h"

extern bool internetConnected;

extern const char *lowerFeederName;
char statusPubTopic[41];

void getFeederStatus()
{
	digitalWrite(PROX_TRIG_PIN, LOW);
	delayMicroseconds(2);
	digitalWrite(PROX_TRIG_PIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(PROX_TRIG_PIN, LOW);

	long duration = pulseIn(PROX_ECHO_PIN, HIGH);
	if (duration >= 60000)
	{
		duration = 0;
	}

	float distanceCm = duration * SOUND_SPEED / 2;
	if (distanceCm > 30)
	{
		distanceCm = 0;
	}

	float capacity = (FEEDER_STORE_HEIGHT - distanceCm) / 20;

	if (capacity > 1)
	{
		capacity = 1;
	}

	if (capacity < 0)
	{
		capacity = 0;
	}

	StaticJsonDocument<16> doc;

	doc["capacity"] = capacity;

	size_t bufferSize = measureJson(doc) + 1; // Get the actual size of the JSON data
	char *jsonBuffer = new char[bufferSize];	// Allocate a buffer of appropriate size

	serializeJson(doc, jsonBuffer, bufferSize);

	publishMessage(statusPubTopic, jsonBuffer);
}

void initStatusPubTopic()
{
	sprintf(statusPubTopic, "murta/feeder/%s/status", lowerFeederName);
}

void statusTask(void *params)
{

	Serial.println("[STATUS TASK] STARTED");

	initStatusPubTopic();

	while (1)
	{
		while (!internetConnected)
		{
			delay(2000);
		}

		getFeederStatus();

		delay(3000);
	}
}
