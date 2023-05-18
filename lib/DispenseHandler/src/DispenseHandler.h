#ifndef DISPENSE_HANDLER_H
#define DISPENSE_HANDLER_H

#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include "MQTTHandler.h"

#define SERVO_PIN 27
#define SERVO_STOP 90
#define SERVO_SPEED 5

void dispenseTask(void *params);

#endif // DISPENSE_HANDLER_H