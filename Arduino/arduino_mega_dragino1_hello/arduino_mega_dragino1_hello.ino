#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// This is the "App EUI" in Helium. Make sure it is little-endian (lsb).
static const u1_t PROGMEM APPEUI[8] = {0x93, 0xD4, 0x61, 0x97, 0x06, 0x89, 0x4F, 0xDB};
void os_getArtEui(u1_t *buf) { memcpy_P(buf, APPEUI, 8); }

// This should also be in little endian format
// These are user configurable values and Helium console permits anything
static const u1_t PROGMEM DEVEUI[8] = {0x14, 0xB7, 0x3B, 0xC1, 0xE5, 0xF7, 0x28, 0xAE};
void os_getDevEui(u1_t *buf) { memcpy_P(buf, DEVEUI, 8); }

// This is the "App Key" in Helium. It is big-endian (msb).
static const u1_t PROGMEM APPKEY[16] = {0xA7, 0x41, 0x97, 0x14, 0x76, 0x07, 0x1B, 0x9C, 0x09, 0x66, 0xB5, 0xAC, 0x12, 0xC2, 0x51, 0xCC};
void os_getDevKey(u1_t *buf) { memcpy_P(buf, APPKEY, 16); }

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,//RFM Chip Select 
    .rxtx = LMIC_UNUSED_PIN, 
    .rst = 9,//RFM Reset
    .dio = {2, 6, 7}, //RFM Interrupt, RFM LoRa pin, RFM Lo  
}; 

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOIN_TXCOMPLETE:
    Serial.println(F("EV_JOIN_TXCOMPLETE"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED"));
    {
      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
      Serial.print("netid: ");
      Serial.println(netid, DEC);
      Serial.print("devaddr: ");
      Serial.println(devaddr, HEX);
      Serial.print("artKey: ");
      for (size_t i = 0; i < sizeof(artKey); ++i) {
        if (i != 0)
          Serial.print("-");
        Serial.print(artKey[i], HEX);
      }
      Serial.println("");
      Serial.print("nwkKey: ");
      for (size_t i = 0; i < sizeof(nwkKey); ++i) {
        if (i != 0)
          Serial.print("-");
        Serial.print(nwkKey[i], HEX);
      }
      Serial.println("");
    }
    // Disable link check validation (automatically enabled
    // during join, but because slow data rates change max TX
    // size, we don't use it in this example.
    LMIC_setLinkCheckMode(0);
    break;
  /*
  || This event is defined but not used in the code. No
  || point in wasting codespace on it.
  ||
  || case EV_RFU1:
  ||     Serial.println(F("EV_RFU1"));
  ||     break;
  */
  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    break;
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack"));
    if (LMIC.dataLen) {
      Serial.println(F("Received "));
      Serial.println(LMIC.dataLen);
      Serial.println(F(" bytes of payload"));
    }
    // Schedule next transmission
    Serial.println(F("schedule next transmission"));
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL),
                        do_send);
    Serial.println(F("after os_setTimedCallback"));
    break;
  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;
  /*
  || This event is defined but not used in the code. No
  || point in wasting codespace on it.
  ||
  || case EV_SCAN_FOUND:
  ||    Serial.println(F("EV_SCAN_FOUND"));
  ||    break;
  */
  case EV_TXSTART:
    Serial.println(F("EV_TXSTART"));
    break;
  default:
    Serial.print(F("Unknown event: "));
    Serial.println((unsigned)ev);
    break;
  }
}

void do_send(osjob_t *j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    Serial.println(F("call LMIC_setTxData2"));
    LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  delay(5000);
  while (!Serial)
    ;
  Serial.begin(9600);
  Serial.println(F("Starting arduino_mega_dragino1_hello"));

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // allow much more clock error than the X/1000 default. See:
  // https://github.com/mcci-catena/arduino-lorawan/issues/74#issuecomment-462171974
  // https://github.com/mcci-catena/arduino-lmic/commit/42da75b56#diff-16d75524a9920f5d043fe731a27cf85aL633
  // the X/1000 means an error rate of 0.1%; the above issue discusses using
  // values up to 10%. so, values from 10 (10% error, the most lax) to 1000
  // (0.1% error, the most strict) can be used.
  LMIC_setClockError(1 * MAX_CLOCK_ERROR / 40);

  LMIC_setLinkCheckMode(0);
  LMIC_setDrTxpow(DR_SF8, 20);
  LMIC_selectSubBand(1);

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() { os_runloop_once(); }
