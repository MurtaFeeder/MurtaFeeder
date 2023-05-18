#ifndef STATUS_HANDLER_H
#define STATUS_HANDLER_H

#include <ArduinoJson.h>

#define PROX_TRIG_PIN 13
#define PROX_ECHO_PIN 12
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
#define FEEDER_STORE_HEIGHT 25

void statusTask(void *params);

#endif // STATUS_HANDLER_H