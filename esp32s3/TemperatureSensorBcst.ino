/** 
 * NimBLEDevice library
 * https://github.com/h2zero/NimBLE-Arduino
 * 
 * Extended advertising settings are configured in "nimconfig.h"
 * for ESP32C3, ESP32S3, and ESP32H2
 *
 *   - CONFIG_BT_NIMBLE_EXT_ADV                 1
 *   - CONFIG_BT_NIMBLE_MAX_EXT_ADV_INSTANCES   1
 *   - CONFIG_BT_NIMBLE_MAX_EXT_ADV_DATA_LEN    31
 */
#include "NimBLEDevice.h"

/**
 * CayenneLPP library
 * https://github.com/ElectronicCats/CayenneLPP
 *
 * Encode/Decode measurements for wireless transmission
 */
#include <CayenneLPP.h>

/**
 * Deep sleep in between transmissions 
 * https://docs.espressif.com/projects/esp-idf/en/v5.2.2/esp32s3/api-reference/system/sleep_modes.html
 */
#include "esp_sleep.h"

#include <chrono>

using namespace std::chrono;

/** Node configuration **/
#define NNAME         "TEAM9"
#define LPPCHAN       0
#define LPPBUF_SIZE   4

#define ADV_FLAGS     BLE_HS_ADV_F_DISC_LTD   // limited discoverable mode
#define ADV_PHY       BLE_HCI_LE_PHY_1M       // 1 Mbps configuration

#define ADV_DURATION  minutes(1) // duration of advertising procedure
#define SLEEP_TIME    hours(1)   // duration of post-advertising deep sleep

/** NimBLE definitions & vars **/
class advCallbacks: public NimBLEExtAdvertisingCallbacks {
  
  void onStopped(NimBLEExtAdvertising* advi, int reason, uint8_t inst_id) {
    Serial.println("Stopped broadcasting. Entering deep sleep ...");
    esp_deep_sleep_start();
  }

};

NimBLEExtAdvertisement adv;
NimBLEExtAdvertising   *advi = nullptr;
advCallbacks           adv_callbacks;

void setup() {

  /* initialize serial communication */
  Serial.begin(115200);
  while (!Serial);

  /* measure internal temperature */
  float itemp = temperatureRead();

  /* pack the data in cayenne lpp format */
  CayenneLPP lpp(LPPBUF_SIZE);
  lpp.addTemperature(LPPCHAN, itemp);

  /* configure broadcaster environment */
  NimBLEDevice::init(NNAME);
  Serial.println("BLE environment initialized");
  Serial.print("Address: ");
  Serial.println(NimBLEDevice::toString().c_str());

  /* configure advertisement packet */
  adv.setConnectable(false);
  adv.setScannable(false);
  adv.setPrimaryPhy(ADV_PHY);
  adv.setSecondaryPhy(ADV_PHY);

  /* build advertisement packet */
  adv.setFlags(ADV_FLAGS); // 3B
  adv.setName(NNAME);      // 2B + sizeof(NNAME)

  /* add the cayenne payload as manufacturer data */
  adv.setManufacturerData(std::string(reinterpret_cast<char*>(lpp.getBuffer()), lpp.getSize()));

  /* configure advertising interface */
  if(!(advi = NimBLEDevice::getAdvertising())) {
    Serial.println("Failed to retrieve advertising interface");
    abort();
  }

  /* register callbacks */
  advi->setCallbacks(&adv_callbacks);

  /* register advertisement data */
  if (!(advi->setInstanceData(0, adv))) {
    Serial.print("Failed to register the advertisement data");
    abort();
  }

  /* start advertising */
  if (!(advi->start(0, duration_cast<milliseconds>(ADV_DURATION).count(), 0))) {
    Serial.print("Failed to start broadcasting");
    abort();
  }
  Serial.println("Broadcasting ...");

  /* configure sleep mode */
  esp_sleep_enable_timer_wakeup(
    duration_cast<microseconds>(SLEEP_TIME).count());
}

void loop() {}

