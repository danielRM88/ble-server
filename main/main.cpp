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
#include "BLEDevice.h"

#include "BLEServer.h"
#include "BLEAddress.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Task.h"


#include "sdkconfig.h"

static char LOG_TAG[] = "main";

extern "C" {
	void app_main(void);
}

static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

class CurlTestTask: public Task {
	void run(void *data) {
		ESP_LOGI(LOG_TAG, "TASK STARTED!");
		BLEServer *pServer = (BLEServer*)data;
		BLEAddress *addresses = pServer->getAddresses();

		for(int i=0; i<=BLEServer::NUMBER_OF_CLIENTS; i++) {
			ESP_LOGI(LOG_TAG, "ADDRESS %d: %s", i, addresses[i].toString().c_str());
		}

//		if (addresses[2].toString().length() > 0) {
//			ESP_LOGI(LOG_TAG, "SECOND ADDRESS: %s", addresses[2].toString().c_str());
//		}
//		RESTClient client;
//
//		/**
//		 * Test POST
//		 */
//
//		RESTTimings *timings = client.getTimings();
//
//		client.setURL("http://httpbin.org/post");
//		client.addHeader("Content-Type", "application/json");
//		client.post("hello world!");
//		ESP_LOGD(tag, "Result: %s", client.getResponse().c_str());
//		timings->refresh();
//		ESP_LOGD(tag, "timings: %s", timings->toString().c_str());
//
//		printf("Tests done\n");
		return;
	}
};

static CurlTestTask *curlTestTask;

class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		ESP_LOGI(LOG_TAG, "CONNECTED!");
		if(pServer->getConnectedCount() == BLEServer::NUMBER_OF_CLIENTS) {
			curlTestTask = new CurlTestTask();
			curlTestTask->setStackSize(12000);
			curlTestTask->start(pServer);
		}
//		pServer->getAdvertising()->start();
	};

	void onDisconnect(BLEServer* pServer) {
//		pMyNotifyTask->stop();
		ESP_LOGI(LOG_TAG, "DISCONNECTED!");
		curlTestTask->stop();
		ESP_LOGI(LOG_TAG, "TASK STOPPED!");
	}
};

static void run() {
//	pMyNotifyTask = new MyNotifyTask();
//	pMyNotifyTask->setStackSize(8000);

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

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
//	BLE2902* p2902Descriptor = new BLE2902();
//	p2902Descriptor->setNotifications(true);
	pCharacteristic->addDescriptor(new BLE2902());

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->addServiceUUID(pService->getUUID());
	pServer->getAdvertising()->start();
}

void app_main(void) {
	run();
}
