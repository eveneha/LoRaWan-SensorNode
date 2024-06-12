#include <MKRWAN.h>

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

  // print diagnostic
  Serial.println("Module Version    : " + modem.version());
  Serial.println("DevEUI            : " + modem.deviceEUI());

  Serial.print("Adaptive Data Rate: ");
  Serial.println(modem.getADR() ? "on" : "off");
  
  Serial.print("Data Rate         : DR");
  Serial.println(modem.getDataRate());

}

void loop() {}
