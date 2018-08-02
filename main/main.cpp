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

uint32_t disconnectedCount;

static BLEUUID 	  serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

static void run() {
	disconnectedCount = 0;
	// Create the BLE Device
	BLEDevice::init("ESP32");

	// Create the BLE Server
	BLEServer* pServer = BLEDevice::createServer();

	// Create the BLE Service
	BLEService *pService = pServer->createService(serviceUUID);

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
