#include "WiFiManager.h"

const char S_nonetworksJSON[] PROGMEM = "{\"message\": \"No networks found. Refresh to scan again.\"}";

class MyWifiManager : public WiFiManager
{
private:
	void setupHTTPServer();
	String getScanJSON();
	void handleAPIWifi();
	void handleAPISaveWifi();
};