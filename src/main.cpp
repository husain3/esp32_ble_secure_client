/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 * updated for NimBLE by H2zero
 */

/** NimBLE differences highlighted in comment blocks **/
#include <BLEDevice.h>

#include <map>
#include <string>
#include <list>

 #include <Arduino.h>
// #include "NimBLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("ABCD");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("1235");

static bool doConnect = true;
static bool connected = false;
static bool doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
	BLERemoteCharacteristic* pBLERemoteCharacteristic,
	uint8_t* pData,
	size_t length,
	bool isNotify) {
		Serial.print("Notify callback for characteristic ");
		Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
		Serial.print(" of data length ");
		Serial.println(length);
		Serial.print("data: ");
		Serial.println((char*)pData);
}

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class Monitor : public BLEClientCallbacks {
public:
	int16_t connection_id;
	int16_t rssi_average = 0;

	/* dBm to distance parameters; How to update distance_factor 1.place the
	 * phone at a known distance (2m, 3m, 5m, 10m) 2.average about 10 RSSI
	 * values for each of these distances, Set distance_factor so that the
	 * calculated distance approaches the actual distances, e.g. at 5m. */
	static constexpr float reference_power  = -50;
	static constexpr float distance_factor = 3.5;

	// uint8_t get_value() { return value++; }
	// esp_err_t get_rssi() { return esp_ble_gap_read_rssi(remote_addr); }

	static float get_distance(const int8_t rssi) {
		return pow(10, (reference_power - rssi)/(10*distance_factor));
	}
	
	void onConnect(BLEClient* pclient) {
		connection_id = pclient->getConnId();
	}

	void onDisconnect(BLEClient* pclient) {
		connected = false;
		Serial.println("onDisconnect");
	}
/***************** New - Security handled here ********************
****** Note: these are the same return values as defaults ********/
	uint32_t onPassKeyRequest(){
		Serial.println("Client PassKeyRequest");
		return 123456;
	}
	bool onConfirmPIN(uint32_t pass_key){
		Serial.print("The passkey YES/NO number: ");Serial.println(pass_key);
		return true;
	}

	void onPassKeyNotify(uint32_t pass_key) {

	}

	bool onSecurityRequest() {
		return true;
	}

	void onAuthenticationComplete(esp_ble_auth_cmpl_t desc){
		Serial.println("Starting BLE work!");
	}
/*******************************************************************/
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new Monitor());

	// if(!pClient->secureConnection()) {
	// 	Serial.println("UNABLE TO SECURE");
	// 	pClient->disconnect();
		
	// 	// Add an ignore flag if too many failed secure connect requests
	// 	// NimBLEDevice::addIgnored(currentDevice);
	// } else {
	// 	Serial.println("SECURITY ESTABLISHED");
	// }

	BLEAddress address("34:94:54:25:43:82");

    // Connect to the remove BLE Server.
    pClient->connect(address);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
		Serial.print("Failed to find our service UUID: ");
		Serial.println(serviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
		Serial.print("Failed to find our characteristic UUID: ");
		Serial.println(charUUID.toString().c_str());
		pClient->disconnect();
		return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
		std::string value = pRemoteCharacteristic->readValue();
		Serial.print("The characteristic value was: ");
		Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
		pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */

/*** Only a reference to the advertised device is passed now
	void onResult(BLEAdvertisedDevice advertisedDevice) { **/
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		Serial.print("BLE Advertised Device found: ");
		Serial.println(advertisedDevice.toString().c_str());

		// We have found a device, let us now see if it contains the service we are looking for.
		/********************************************************************************
		if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
		********************************************************************************/
		if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

			BLEDevice::getScan()->stop();
		/*******************************************************************
			 myDevice = new BLEAdvertisedDevice(advertisedDevice);
		*******************************************************************/
			// myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
			doConnect = true;
			doScan = true;

		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
	Serial.begin(115200);
	Serial.println("Starting Arduino BLE Client application...");
	BLEDevice::init("");

	// Retrieve a Scanner and set the callback we want to use to be informed when we
	// have detected a new device.  Specify that we want active scanning and start the
	// scan to run for 5 seconds.
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setInterval(1349);
	pBLEScan->setWindow(449);
	pBLEScan->setActiveScan(true);
	pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

	// If the flag "doConnect" is true then we have scanned for and found the desired
	// BLE Server with which we wish to connect.  Now we connect to it.  Once we are
	// connected we set the connected flag to be true.
	if (doConnect == true) {
		if (connectToServer()) {
			Serial.println("We are now connected to the BLE Server.");
		} else {
			Serial.println("We have failed to connect to the server; there is nothin more we will do.");
		}
		doConnect = false;
	}

	// If we are connected to a peer BLE Server, update the characteristic each time we are reached
	// with the current time since boot.
	if (connected) {
		String newValue = "Time since boot: " + String(millis()/1000);
		Serial.println("Setting new characteristic value to \"" + newValue + "\"");

		// Set the characteristic's value to be the array of bytes that is actually a string.
		/*** Note: write / read value now returns true if successful, false otherwise - try again or disconnect ***/
		pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
	} else if(doScan) {
		BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
	}

	delay(1000); // Delay a second between loops.
} // End of loop