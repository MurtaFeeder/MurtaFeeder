#include "MyWifiManager.h"

// OVERRIDING WifiManger
void MyWifiManager::setupHTTPServer()
{
#ifdef WM_DEBUG_LEVEL
	DEBUG_WM(F("Starting Web Portal"));
#endif

	if (_httpPort != 80)
	{
#ifdef WM_DEBUG_LEVEL
		DEBUG_WM(DEBUG_VERBOSE, F("http server started with custom port: "), _httpPort); // @todo not showing ip
#endif
	}

	server.reset(new WM_WebServer(_httpPort));
	// This is not the safest way to reset the webserver, it can cause crashes on callbacks initilized before this and since its a shared pointer...

	if (_webservercallback != NULL)
	{
#ifdef WM_DEBUG_LEVEL
		DEBUG_WM(DEBUG_VERBOSE, F("[CB] _webservercallback calling"));
#endif
		_webservercallback(); // @CALLBACK
	}
	// @todo add a new callback maybe, after webserver started, callback cannot override handlers, but can grab them first

	/* Setup httpd callbacks, web pages: root, wifi config pages, SO captive portal detectors and not found. */

	// G macro workaround for Uri() bug https://github.com/esp8266/Arduino/issues/7102
	server->on(WM_G(R_root), std::bind(&WiFiManager::handleRoot, this));
	server->on(WM_G(R_wifi), std::bind(&WiFiManager::handleWifi, this, true));
	server->on(WM_G(R_wifinoscan), std::bind(&WiFiManager::handleWifi, this, false));
	server->on(WM_G(R_wifisave), std::bind(&WiFiManager::handleWifiSave, this));
	server->on(WM_G(R_info), std::bind(&WiFiManager::handleInfo, this));
	server->on(WM_G(R_param), std::bind(&WiFiManager::handleParam, this));
	server->on(WM_G(R_paramsave), std::bind(&WiFiManager::handleParamSave, this));
	server->on(WM_G(R_restart), std::bind(&WiFiManager::handleReset, this));
	server->on(WM_G(R_exit), std::bind(&WiFiManager::handleExit, this));
	server->on(WM_G(R_close), std::bind(&WiFiManager::handleClose, this));
	server->on(WM_G(R_erase), std::bind(&WiFiManager::handleErase, this, false));
	server->on(WM_G(R_status), std::bind(&WiFiManager::handleWiFiStatus, this));

	// SELF CREATED OPTIONS
	server->on(WM_G("/api/wifi"), std::bind(&MyWifiManager::handleAPIWifi, this));

	server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));

	server->on(WM_G(R_update), std::bind(&WiFiManager::handleUpdate, this));
	server->on(WM_G(R_updatedone), HTTP_POST, std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));

	server->begin(); // Web server start
#ifdef WM_DEBUG_LEVEL
	DEBUG_WM(DEBUG_VERBOSE, F("HTTP server started"));
#endif
}