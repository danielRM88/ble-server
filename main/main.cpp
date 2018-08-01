/**
 * Create a BLE server that, once we receive a connection, will send periodic notifications.
 * The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8
 *
 * The design of creating the BLE server is:
 * 1. Create a BLE Server
 * 2. Create a BLE Service
 * 3. Create a BLE Characteristic on the Service
 * 4. Create a BLE Descriptor on the characteristic
 * 5. Start the service.
 * 6. Start advertising.
 *
 * A connect hander associated with the server starts a background task that performs notification
 * every couple of seconds.
 *
 * @author: Neil Kolban, July 2017
 *
 */
#include "sdkconfig.h"

#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "BLEDevice.h"

#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Task.h"
#include "GeneralUtils.h"

extern "C" {
	void app_main(void);
}

static char LOG_TAG[] = "BLEServer";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

//static BLEUUID 	  serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID 	  serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");
//static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

BLECharacteristic *rssi1;
BLEServer *pServer;

class MyNotifyTask: public Task {
	void run(void *data) {
		BLEAddress* pAddress = (BLEAddress*)data;
		std::ostringstream stringStream;
		int rssiValue = 0;
		while(1) {
			delay(1000);
			rssiValue = 0;
			stringStream.str("");
			stringStream.clear();
			pServer->m_semaphoreRssiCmplEvt.take("getRssi");
			esp_err_t rc = ::esp_ble_gap_read_rssi(*pAddress->getNative());
			if (rc != ESP_OK) {
				ESP_LOGE(LOG_TAG, "<< getRssi: esp_ble_gap_read_rssi: rc=%d %s", rc, GeneralUtils::errorToString(rc));
			} else {
				rssiValue = pServer->m_semaphoreRssiCmplEvt.wait("getRssi");
				ESP_LOGI(LOG_TAG, "RSSI: %d", rssiValue);
				stringStream << rssiValue;
			}
			ESP_LOGI(LOG_TAG, "*** NOTIFY: %s ***", stringStream.str().c_str());
			rssi1->setValue(stringStream.str());
			rssi1->notify();
			//pCharacteristic->indicate();
//			value++;
		} // While 1
	} // run
}; // MyNotifyTask

MyNotifyTask *pMyNotifyTask;

class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer, BLEAddress* address) {
		pServer->getAdvertising()->stop();
		pMyNotifyTask->start(address);
	};

	void onDisconnect(BLEServer* pServer) {
		pMyNotifyTask->stop();
		pServer->getAdvertising()->start();
	}
};

static void run() {
	pMyNotifyTask = new MyNotifyTask();
	pMyNotifyTask->setStackSize(8000);

	// Create the BLE Device
	BLEDevice::init("ESP32");

	// Create the BLE Server
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(serviceUUID);

	// Create a BLE Characteristic
	rssi1 = pService->createCharacteristic(
		charUUID,
		BLECharacteristic::PROPERTY_READ   |
		BLECharacteristic::PROPERTY_WRITE  |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
	BLE2902* p2902Descriptor = new BLE2902();
	p2902Descriptor->setNotifications(true);
	rssi1->addDescriptor(p2902Descriptor);
	rssi1->setNotifyProperty(true);

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->addServiceUUID(pService->getUUID());

	// Start advertising
	pServer->getAdvertising()->start();
}

void app_main(void) {
	run();
}
