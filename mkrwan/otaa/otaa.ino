#include <MKRWAN.h>

#define APPEUI  "0000000000000000"
#define APPKEY  "E1452DB043A1E33DAC85597489EFBFBC"

#define CONN_MS     1000
#define BLINKERR_MS 500

LoRaModem modem;

void blinkErr() {
  bool led_state = false;

  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, (led_state ^= true));
    delay(BLINKERR_MS);
  }
}

void setup() {

  // serial configuration
  Serial.begin(115200);
  while (!Serial);
  
  // specify the LoRaWAN frequency plan
  if (!modem.begin(EU868)) {
    Serial.println("LoRaModem::begin() failed");
    blinkErr();
  };
  Serial.println("This is device '" + modem.deviceEUI() + "'");

}

void loop() {

  if (!modem) {
    Serial.print("No connection ...");
    
    if (!modem.joinOTAA(APPEUI, APPKEY)) {  
      Serial.println(" OTAA failed");
      delay(CONN_MS);
    } 
    else {
      Serial.println(" OTAA completed");
      Serial.println("DevAddr: " + modem.getDevAddr());
      Serial.println("NwkSKey: " + modem.getNwkSKey());
      Serial.println("AppSKey: " + modem.getAppSKey());
    }
    
  }

}
