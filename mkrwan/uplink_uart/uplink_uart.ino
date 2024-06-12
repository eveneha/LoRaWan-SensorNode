/**
 * Support library for MKR WAN 1300/1310
 * https://www.arduino.cc/reference/en/libraries/mkrwan/
 */
#include <MKRWAN.h>
#include "keys.h"

/**
 * CayenneLPP library
 * https://github.com/ElectronicCats/CayenneLPP
 *
 * Encode/Decode measurements for wireless transmission
 */
#include <CayenneLPP.h>

/**
 * Power save primitives features for SAMD and nRF52 32bit boards
 * https://www.arduino.cc/reference/en/libraries/arduino-low-power/
 */
#include <ArduinoLowPower.h>

#include <chrono>
using namespace std::chrono;

#define MAXFPAYLOAD   51      // targeting EU868 DR0
#define xstr(s)       _str(s)
#define _str(s)       #s 

#define WKUP_PIN      8       // gpio wakeup

#define RXWINDOW_TIME std::chrono::milliseconds(2000 + 1319) // RX2D + time-on-air(DR0, max)
#define CONN_TIME     std::chrono::seconds(15)
#define BLINKERR_MS   500

LoRaModem modem;

/* payload decoder */
CayenneLPP lpp(0);

/* payload buffer */
uint8_t payl[MAXFPAYLOAD];
uint8_t payl_size;

/* GPIO interrupt handler */
void wkupHandler () {
  detachInterrupt(digitalPinToInterrupt(WKUP_PIN));
}

void blinkErr() {
  bool led_state = false;

  while (true) {
    digitalWrite(LED_BUILTIN, (led_state ^= true));
    LowPower.deepSleep(BLINKERR_MS);
  }
}

void printHex8(uint8_t b) {
  Serial.print(b >> 4, HEX);
  Serial.print(b & 0xF, HEX);
}

void deepSleep(const milliseconds &ms = {}, bool modem_off = true) {
  /* https://github.com/arduino-libraries/ArduinoLowPower/issues/7 */

  Serial.flush();
  Serial.end();
  digitalWrite(LED_BUILTIN, LOW);

  if (modem_off)
    modem.sleep();

  if (ms == milliseconds{}) {
    attachInterrupt(digitalPinToInterrupt(WKUP_PIN), wkupHandler, RISING);
    LowPower.deepSleep();
  } 
  else
    LowPower.deepSleep(static_cast<uint32_t>(ms.count()));
  
  if (modem_off)
    modem.sleep(false);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.begin(115200);
}

void setup() {
  
  /* initialize serial communications */
  Serial.begin(115200);
  while(!Serial);

  /* specify the LoRaWAN frequency plan */
  if (!modem.begin(EU868)) {
    Serial.println("LoRaModem::begin() failed");
    blinkErr();
  };
  Serial.println("This is device '" + modem.deviceEUI() + "'");

  /* low power end device */
  if (!modem.configureClass(CLASS_A)) {
    Serial.println("LoRaModem::configureClass() failed");
    blinkErr();
  }
  Serial.println("Device configured in class 'A'");

  /* configure interrupt source */
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WKUP_PIN, INPUT);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(WKUP_PIN), wkupHandler, RISING);

}

void loop() {
  int b, err;

  /* wait for a new payload */
  Serial.println("Entering deep sleep ...");
  deepSleep();
  Serial.println("Awake ...");

  /* uart message about to come */
  Serial1.begin(115200);
  while (!Serial1.available());

  /* first byte tells how many bytes to read */
  uint8_t nbytes = Serial1.read();
  if (nbytes > MAXFPAYLOAD) {
    Serial.println("Exceeded maximum tx frame payload (" xstr(MAXFPAYLOAD) " B)");
    Serial.println("The payload will be truncated");
    nbytes = MAXFPAYLOAD;
  }

  /* blocking read */
  payl_size = 0;
  while (payl_size < nbytes) {
    while((b = Serial1.read()) < 0);
    payl[payl_size++] = b;
  }

  /* consume remaining input */
  if(payl_size == MAXFPAYLOAD && Serial1.available()) {
    while (Serial1.read() > 0);
  }

  /* payload to send processed */
  Serial1.end();

  /* handle disconnections */
  while (!modem) {
    Serial.print("No connection ...");
    if (!modem.joinOTAA(APPEUI, APPKEY, 10)) {
      Serial.println(" OTAA failed");
      deepSleep(CONN_TIME);
    }
    else {
      Serial.println(" OTAA completed");
      Serial.println("Device activated as '" + modem.getDevAddr() + "'");
    }
  }

  Serial.print("Sending :");
  for (uint8_t *p = payl; p != payl + payl_size; p++) {
    Serial.print(" ");
    printHex8(*p);
  }
  Serial.print("\nSize (B): ");
  Serial.println(payl_size);

  /* uplink */
  modem.beginPacket();      
  modem.write(payl, payl_size);
  if ((err = modem.endPacket()) <= 0) { // unconfirmed
    Serial.print("LoRaModem::endPacket(): uplink transmission failed (err = ");
    Serial.print(err);
    Serial.println(")");
  } else
    Serial.println("Sent");

  // wait past downlink windows
  deepSleep(RXWINDOW_TIME, false);

  // check downlink
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

  } else
    Serial.println("No downlink event");

}

