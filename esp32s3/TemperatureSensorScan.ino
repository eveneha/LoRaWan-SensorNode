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

#include <string>
#include <sstream>
#include <iomanip>

/** node configuration **/
#define BCST_ADDRESS_HEX    0xdcda0c4b3435
#define SCAN_WINDOW_MS      11.25
#define SCAN_FILTER_MODE    CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE // packets from the same address are reported when data changes
#define SCAN_FILTER_POLICY  BLE_HCI_SCAN_FILT_USE_WL               // packets are processed from white list only

#define WKUP_PIN            1
#define WKUP_PULSE_MS       1000
#define WKUP_DELAY_MS       2500
#define TX1_PIN             20
#define RX1_PIN             21
#define MAXFPAYLOAD         51

/** NimBLE definitions & vars **/
class advDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
  
  void onResult(NimBLEAdvertisedDevice* adv_device) {

    /* address */
    Serial.print("@ ");
    Serial.print(adv_device->getAddress().toString().c_str());
    
    /* name */
    if (adv_device->haveName())
      Serial.print((" (" + adv_device->getName()).c_str());
    
    /* rssi */
    if (adv_device->haveRSSI()) {
      Serial.print("/");
      Serial.print(adv_device->getRSSI());
      Serial.print(" dBm");
    }
    Serial.println(")");    

    /* full payload */
    Serial.print("  - payload : ");
    Serial.println(
      _ltvToString(adv_device->getPayload(), adv_device->getAdvLength()).c_str());
    
    /* manufacturer data carries the measurements in cayenne lpp format */
    if (!adv_device->haveManufacturerData()) {
      Serial.println("  (E) Missing manufacturer specific data ...");
      return;
    }

    JsonDocument js;
    JsonArray jsa = js.to<JsonArray>();
    CayenneLPP lpp(0);

    /* print decoded cayenne lpp payload */
    std::string mfr_data = adv_device->getManufacturerData();
    lpp.decode(reinterpret_cast<uint8_t*>(&mfr_data[0]), mfr_data.size(), jsa);
    Serial.print("  - mfr data: ");
    serializeJson(js[0], Serial);
    Serial.println("");

    /* wake up mkrwan */
    digitalWrite(WKUP_PIN, HIGH);
    delay(WKUP_PULSE_MS);
    digitalWrite(WKUP_PIN, LOW);
    delay(WKUP_DELAY_MS);

    /* forward size, payload */
    Serial1.write(mfr_data.size());
    Serial1.write(&mfr_data[0], mfr_data.size());
    Serial1.flush();
    Serial.println("  Sent!");
  }

  private:

    std::string _ltvToString(const uint8_t *v, const size_t l) {
      std::ostringstream oss;
      oss << std::uppercase << std::setfill('0');
      
      oss << '{';
      for (size_t i = 0; i < l;) {
        size_t item_l = v[i];
        if (i + item_l >= l)
          return {};
        
        // start item
        oss << '('
            << std::setw(2) << std::hex << +v[i] 
            << ',';

        for (size_t j = 1; j <= item_l; j++)
          oss << std::setw(2) << std::hex << +v[i+j];
        
        oss << ')';

        // next item
        i += item_l + 1;
        if (i < l) 
          oss << ',';

      }
      oss << '}';

      return oss.str();
    }

};

NimBLEScan *scanp = nullptr;
advDeviceCallbacks adv_device_callbacks;

void setup() {

  /* initialize serial communication */
  Serial.begin(115200);
  while (!Serial);

  /* initialize interface to mkrwan */
  pinMode(WKUP_PIN, OUTPUT);
  Serial1.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);
  while(!Serial1);
  
  /* configure observer environment */
  NimBLEDevice::setScanFilterMode(SCAN_FILTER_MODE);
  NimBLEDevice::init("");
  NimBLEDevice::whiteListAdd(NimBLEAddress(BCST_ADDRESS_HEX));

  /* configure scanning interface */
  scanp = NimBLEDevice::getScan();

  /* register callbacks */
  scanp->setAdvertisedDeviceCallbacks(&adv_device_callbacks, true);

  /* configure scanner parameters */
  scanp->setInterval(SCAN_WINDOW_MS);
  scanp->setWindow(SCAN_WINDOW_MS);
  scanp->setFilterPolicy(SCAN_FILTER_POLICY);
  scanp->setDuplicateFilter(true);
  scanp->setLimitedOnly(true);
  scanp->setMaxResults(0); // callbacks only

  /* start conitnuous scanning */
  if (!(scanp->start(0, nullptr))) {
    Serial.println("Failed to start scanning");
    abort();
  }

}

void loop() {}
