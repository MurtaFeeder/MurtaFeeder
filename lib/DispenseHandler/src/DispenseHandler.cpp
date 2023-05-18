

#include "DispenseHandler.h"

/* SHARED VARIABLES */
extern QueueHandle_t dispenseQueue;
extern Servo myServo;
extern char *lowerFeederName;
/* -------- */
char actionPubTopic[39];

void dispense(int portions)
{
	while (portions > 0)
	{
		myServo.write(0);
		delay(1000);
		myServo.write(SERVO_STOP);
		portions--;
		delay(250);
	}
	Serial.printf("[DISPENSE HANDLE] Portions dispensed: %d\n", portions);
}

void initActionPubTopic()
{
	sprintf(actionPubTopic, "murta/feeder/%s/done", lowerFeederName);
}

void dispenseTask(void *params)
{

	Serial.println("[DISPENSE TASK] STARTED");
	initActionPubTopic();

	DispenseMessage *message;

	while (1)
	{
		xQueueReceive(dispenseQueue, &(message), portMAX_DELAY);
		int portions = message->portions;
		dispense(portions);
		publishMessage(actionPubTopic, message->message);
	}
}
