#include "WiFiManager.h"
#include "envs.h"
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

const int TRIG_PIN = 13;
const int ECHO_PIN = 12;

void setupFeederPins()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);  // Sets the echoPin as an Input
}

void initWifiManager()
{
  WiFiManager wm;
  wm.resetSettings();
  bool res;

  String apName = "MurtaFeeder_";
  String chipId = String(WIFI_getChipId(), HEX);
  chipId.toUpperCase();
  apName += chipId;

  res = wm.autoConnect(apName.c_str(), "MurtaFeeder1234");

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}

void connectMQTT()
{
}

void setup()
{
  setupFeederPins();
  initWifiManager();
  connectMQTT();
}

void getFeederStatus()
{
  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(ECHO_PIN, HIGH);
  Serial.print("Duration (long): ");
  Serial.println(duration);
  // Calculate the distance
  float distanceCm = duration * SOUND_SPEED / 2;

  // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
}

void loop()
{
  getFeederStatus();
  delay(5000);
}