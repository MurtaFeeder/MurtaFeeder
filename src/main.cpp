#include "WiFiManager.h"
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

void setup()
{
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // put your setup code here, to run once:

  //
  setupFeederPins();

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  wm.resetSettings();

  // Disable automatic redirection to Portal
  // wm.setCaptivePortalEnable(false);

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;

  String apName = "MurtaFeeder_";
  String chipId = String(WIFI_getChipId(), HEX);
  chipId.toUpperCase();
  apName += chipId;

  res = wm.autoConnect(apName.c_str(), "MurtaFeeder1234"); // password protected ap

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
  // put your main code here, to run repeatedly:
  getFeederStatus();
  delay(5000);
}