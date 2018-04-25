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
#include "BLEDevice.h"

#include "BLEServer.h"
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
static BLEUUID    rssi1UUID("0d563a58-196a-48ce-ace2-dfec78acc814");
static BLEUUID    rssi2UUID("12345678-9ABC-DEF1-2345-6789ABCDEF12");

BLECharacteristic *rssi1;
BLECharacteristic *rssi2;

FreeRTOS::Semaphore m_semaphoreHttpTask   = FreeRTOS::Semaphore("HttpTask");

//class MyNotifyTask: public Task {
//	void run(void *data) {
//		uint8_t value = 0;
//		while(1) {
//			delay(2000);
//			ESP_LOGD(LOG_TAG, "*** NOTIFY: %d ***", value);
////			pCharacteristic->setValue("send");
//			pCharacteristic->setValue(&value, 1);
//			pCharacteristic->notify();
//			//pCharacteristic->indicate();
//			value++;
//		} // While 1
//	} // run
//}; // MyNotifyTask
//
//MyNotifyTask *pMyNotifyTask;

class CurlTestTask: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "SENDING HTTP REQ");

		rssi1->setValue("");
		rssi2->setValue("");

		m_semaphoreHttpTask.give();
//		ESP_LOGD(tag, "Testing curl ...");
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

class MyCallbacks: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pChar) {
		std::string value = pChar->getValue();
		std::string rssiValue1 = rssi1->getValue();
		std::string rssiValue2 = rssi2->getValue();
		if (value.length() > 0) {
			ESP_LOGD(LOG_TAG, "*********");
			ESP_LOGD(LOG_TAG, "New value: %s", value.c_str());
			ESP_LOGD(LOG_TAG, "*********");
		}

		if (pChar->getUUID().equals(rssi1->getUUID())) {
			ESP_LOGD(LOG_TAG, "New value for rssi1");
			if (rssiValue1.compare("send") == 0) {
				rssiValue1 = value;
				rssi1->setValue(value);
			}
		} else if (pChar->getUUID().equals(rssi2->getUUID())) {
			ESP_LOGD(LOG_TAG, "New value for rssi2");
			if (rssiValue2.compare("send") == 0) {
				rssiValue2 = value;
				rssi2->setValue(value);
			}
		}

		if(rssiValue1.compare("send") != 0 && rssiValue2.compare("send") != 0) {
//			m_semaphoreHttpTask.take("http");
//			if(rssiValue1.length() > 0 && rssiValue2.length() > 0) {
//				curlTestTask = new CurlTestTask();
//				curlTestTask->setStackSize(12000);
//				curlTestTask->start();
//			}
//			m_semaphoreHttpTask.wait("http");
			rssi1->setValue("send");
			rssi2->setValue("send");
		}
	}
};


class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
//		pMyNotifyTask->start();
		ESP_LOGI(LOG_TAG, "CONNECTED!");
		pServer->getAdvertising()->start();
	};

	void onDisconnect(BLEServer* pServer) {
//		pMyNotifyTask->stop();
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
	rssi1 = pService->createCharacteristic(
		rssi1UUID,
		BLECharacteristic::PROPERTY_READ   |
		BLECharacteristic::PROPERTY_WRITE  |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	rssi2 = pService->createCharacteristic(
		rssi2UUID,
		BLECharacteristic::PROPERTY_READ   |
		BLECharacteristic::PROPERTY_WRITE  |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
//	BLE2902* p2902Descriptor = new BLE2902();
//	p2902Descriptor->setNotifications(true);
	rssi1->addDescriptor(new BLE2902());
	rssi1->setCallbacks(new MyCallbacks());
	rssi1->setValue("send");

	rssi2->addDescriptor(new BLE2902());
	rssi2->setCallbacks(new MyCallbacks());
	rssi2->setValue("send");

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->addServiceUUID(pService->getUUID());
	pServer->getAdvertising()->start();
}

void app_main(void) {
	run();
}
