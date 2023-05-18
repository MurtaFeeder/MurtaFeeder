#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <WiFiManager.h>
#include <ESP32Ping.h>

#define BUTTON_PIN 33
#define LONG_PRESS_TIME 5000

// Wifi loop
void wifiTask(void *pvParameters);

void initWifiManager();

#endif // WIFI_HANDLER_H