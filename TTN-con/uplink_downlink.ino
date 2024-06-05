#include <MKRWAN.h>
#include <vector>

#define APPEUI      "0000000000000000"
#define APPKEY      "E1452DB043A1E33DAC85597489EFBFBC"

#define MAXFPAYLOAD 51      // targeting EU868 DR0
#define xstr(s)     str(s)
#define str(s)      #s 

#define BLINKERR_MS 500
#define CONN_MS     1000

LoRaModem modem;
std::vector<uint8_t> tx_payload;

const char header[] = "Enter '\\n' terminated string to send: ";

void blinkErr() {
  bool led_state = false;

  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, (led_state ^= true));
    delay(BLINKERR_MS);
  }
}

void printHex8(uint8_t b) {
  Serial.print(b >> 4, HEX);
  Serial.print(b & 0xF, HEX);
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

  // set to "continuous downlink" (class C)
  if (!modem.configureClass(CLASS_C)) {
    Serial.println("LoRaModem::configureClass() failed");
    blinkErr();
  }
  Serial.println("Device configured in class 'C'");

  // prepare payload buffers: max bytes 
  tx_payload.reserve(MAXFPAYLOAD);

}

void loop() {
  int b;

  // handles disconnections
  if (!modem) {
    Serial.print("No connection ...");
    
    if (!modem.joinOTAA(APPEUI, APPKEY)) {
      Serial.println(" OTAA failed");
      delay(CONN_MS);
      return;
    }

    Serial.println(" OTAA completed");
    Serial.println("Device activated as '" + modem.getDevAddr() + "'");

    // start asking for data to send
    Serial.println(header);

  }

  // uplink: build payload
  while ((b = Serial.read()) > 0) {
    int err;

    // non-terminator characters are buffered so long as there is space
    // and discarded otherwise
    // terminator-only messages are directly discarded
    if (b != '\n') {
      if (tx_payload.size() < MAXFPAYLOAD)
        tx_payload.push_back(b);
      else {
        Serial.println("Exceeded maximum tx frame payload (" xstr(MAXFPAYLOAD) " B)");
        Serial.println("The payload will be truncated");

        // consume remaining input, exits with b < 0
        while ((b = Serial.read()) > 0);
      }
    }
    else if (tx_payload.empty()) {
      Serial.println("Empty payload discarded");
      Serial.println(header);
    }
    
    // tx_payload full or found terminator of a valid message
    if (b < 0 || (b == '\n' && !tx_payload.empty())) {

      Serial.print("Sending :");
      for (const auto & b : tx_payload) {
        Serial.print(" ");
        printHex8(b);
      }
      Serial.print("\nSize (B): ");
      Serial.println(tx_payload.size());

      // attempt uplink transmission
      modem.beginPacket();      
      if (!modem.write(tx_payload.data(), tx_payload.size()))
        Serial.println("LoRaModem::write(): tx buffer full");
      else if ((err = modem.endPacket()) <= 0) { // unconfirmed
        Serial.print("LoRaModem::endPacket(): uplink transmission failed (err = ");
        Serial.print(err);
        Serial.println(")");
      } else
        Serial.println("Sent");
      
      // prepare next uplink
      Serial.println(header);
      tx_payload.clear();
    }
    
  } // uplink

  // downlink
  if (modem.available()) {
    Serial.println("Downlink event ...");

    // clear rx byte count
    b = 0;

    Serial.print("Received:");
    do {
      Serial.print(" ");
      printHex8(modem.read());
      b++;
    } while (modem.available());

    Serial.print("\nSize (B): ");
    Serial.println(b);

    // prepare next downlink
    if (!Serial.available())
      Serial.println(header);

  } // downlink

}
