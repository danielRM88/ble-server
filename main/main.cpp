/*
 * Test the REST API client.
 * This application leverages LibCurl.  You must make that package available
 * as well as "enable it" from "make menuconfig" and C++ Settings -> libCurl present.
 *
 * You may also have to include "posix_shims.c" in your compilation to provide resolution
 * for Posix calls expected by libcurl that aren't present in ESP-IDF.
 *
 * See also:
 * * https://github.com/nkolban/esp32-snippets/issues/108
 *
 */
#include <esp_log.h>
#include <string>
#include <sstream>
#include <array>

#include <curl/curl.h>
#include <RESTClient.h>
#include <WiFi.h>
#include <WiFiEventHandler.h>

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEAddress.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include "Task.h"
#include "GeneralUtils.h"
#include "sdkconfig.h"

static char LOG_TAG[] = "main";
static const int BUILT_IN_LED = 2;
static const int ON = 1;
static const int OFF = 1;

static int connected = 0;

//#define WIFI_SSID "Hall_Of_Residence"
//#define WIFI_PASSWORD "hofr6971"

#define WIFI_SSID "Daniel's iPhone"
#define WIFI_PASSWORD "daniel'sPASSWORD!?"

extern "C" {
	void app_main(void);
}

static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

static int taskRunning = 0;
static int restarting = 0;

static WiFi *wifi;

class CurlTestTask: public Task {
	void run(void *data) {
		ESP_LOGI(LOG_TAG, "TASK STARTED!");
		BLEServer *pServer = (BLEServer*)data;
		BLEAddress *addresses = pServer->getAddresses();
		RESTClient client;

		while(1) {
			if(connected == 1) {

				for(int i=0; i<=BLEServer::NUMBER_OF_CLIENTS; i++) {
					ESP_LOGI(LOG_TAG, "ADDRESS %d: %s", i, addresses[i].toString().c_str());

					pServer->m_semaphoreRssiCmplEvt.take("getRssi");
					esp_err_t rc = ::esp_ble_gap_read_rssi(*addresses[i].getNative());
					if (rc != ESP_OK) {
						ESP_LOGE(LOG_TAG, "<< getRssi: esp_ble_gap_read_rssi: rc=%d %s", rc, GeneralUtils::errorToString(rc));
					} else {
						int rssiValue = pServer->m_semaphoreRssiCmplEvt.wait("getRssi");
						ESP_LOGI(LOG_TAG, "RSSI: %d", rssiValue);
					}
				}

				client.setURL("http://172.20.10.3:3000/post");
				client.addHeader("Content-Type", "application/json");
				client.post("hello world!");

				GPIO_OUTPUT_SET( BUILT_IN_LED, OFF );
				FreeRTOS::sleep(500);
				GPIO_OUTPUT_SET( BUILT_IN_LED, ON );
			} else {
				ESP_LOGI(LOG_TAG, "Attempting reconnect");
				wifi->connectAP(WIFI_SSID, WIFI_PASSWORD);
			}
		}

		return;
	}
};

static CurlTestTask *curlTestTask;

class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		ESP_LOGI(LOG_TAG, "CONNECTED!");
		GPIO_OUTPUT_SET( BUILT_IN_LED, ON );
		FreeRTOS::sleep(500);
		GPIO_OUTPUT_SET( BUILT_IN_LED, OFF );
		if(pServer->getConnectedCount() == BLEServer::NUMBER_OF_CLIENTS) {
			curlTestTask = new CurlTestTask();
			curlTestTask->setStackSize(12000);
			curlTestTask->start(pServer);
			taskRunning = 1;
		}
	};

	void onDisconnect(BLEServer* pServer) {
		ESP_LOGI(LOG_TAG, "DISCONNECTED!");
		if(taskRunning == 1) {
			curlTestTask->stop();
			ESP_LOGI(LOG_TAG, "TASK STOPPED!");
			taskRunning = 0;
		}
	}
};

class MyWiFiEventHandler: public WiFiEventHandler {

	esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
		ESP_LOGI(LOG_TAG, "MyWiFiEventHandler(Class): staGotIp");

		connected = 1;
		ESP_LOGI(LOG_TAG, "Connected: %d", connected);

		return ESP_OK;
	}
};

static void run() {

	MyWiFiEventHandler *eventHandler = new MyWiFiEventHandler();
	wifi = new WiFi();
	wifi->setWifiEventHandler(eventHandler);
	wifi->connectAP(WIFI_SSID, WIFI_PASSWORD);

	// Create the BLE Device
	BLEDevice::init("ESP32");

	// Create the BLE Server
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(serviceUUID);

	// Create a BLE Characteristic
	BLECharacteristic *pCharacteristic = pService->createCharacteristic(
		charUUID,
		BLECharacteristic::PROPERTY_READ   |
		BLECharacteristic::PROPERTY_WRITE  |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	pCharacteristic->addDescriptor(new BLE2902());

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->addServiceUUID(pService->getUUID());
//	BLEDevice::setPower(ESP_PWR_LVL_P1);
	pServer->getAdvertising()->start();
}

void app_main(void) {
	run();
}
