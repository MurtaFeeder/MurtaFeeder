#include "WiFiHandler.h"

/* SHARED VARIABLES */
extern const char *feederName;
extern bool internetConnected;
/* ------- */

WiFiManager wm;
IPAddress ip(8, 8, 8, 8);
int lastButtonState = HIGH;
int currentButtonState;
unsigned long pressedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

void initWifiManager()
{
	WiFi.mode(WIFI_STA);

	wm.setCaptivePortalEnable(false);

	// Attempt to connect to WiFi using previously saved credentials
	if (!wm.autoConnect(feederName, "MurtaFeeder1234"))
	{
	}

	// TURN OFF RED LIGHT

	// if you get here, you have connected to WiFi
	Serial.println("[WIFI TASK] WiFi Connected");
}

void handleButtonPress()
{
	currentButtonState = digitalRead(BUTTON_PIN);

	if (lastButtonState == HIGH && currentButtonState == LOW)
	{ // button is pressed
		pressedTime = millis();
		isPressing = true;
		isLongDetected = false;
	}
	else if (lastButtonState == LOW && currentButtonState == HIGH)
	{ // button is released
		isPressing = false;
	}

	if (isPressing == true && isLongDetected == false)
	{
		long pressDuration = millis() - pressedTime;

		if (pressDuration > LONG_PRESS_TIME)
		{
			Serial.println("[WIFI TASK] A long press is detected");
			isLongDetected = true;
		}
	}

	// save the the last state
	lastButtonState = currentButtonState;

	if (isLongDetected)
	{

		wm.resetSettings();
		Serial.println("[WIFI TASK] Reset detected");
		ESP.restart();
	}
}

void wifiTask(void *params)
{

	Serial.println("[WIFI TASK] STARTED");
	initWifiManager();
	delay(1000);

	while (1)
	{
		if (WiFi.status() != WL_CONNECTED)
		{
			Serial.println("[WIFI TASK] WiFi connection lost");
			internetConnected = false;
			delay(2000);
			continue;
		}

		if (!Ping.ping(ip, 4))
		{
			Serial.println("[WIFI TASK] No internet connection");
			internetConnected = false;
			delay(2000);
			continue;
		}

		internetConnected = true;

		handleButtonPress();
	}
}