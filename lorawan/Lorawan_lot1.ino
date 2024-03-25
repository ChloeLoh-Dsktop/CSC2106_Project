#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <AESLib.h>
#include <AES.h>


//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// Define the pins connected to the IR sensor and ultrasonic sensor
const int irSensorPin = A0;
const int ultrasonicTrigPin = A1; // Trig pin of ultrasonic sensor
const int ultrasonicEchoPin = A2; // Echo pin of ultrasonic sensor

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={0x5C, 0x5C, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16]={0x6C, 0xE9, 0x2B, 0x6E, 0xA4, 0xC5, 0xFE, 0x6E, 0x95, 0xF2, 0x04, 0x10, 0x2D, 0x54, 0xEF, 0x25};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t btn_activated[1] = { 0x01};
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 7,
  .dio = {2, 5, 6},
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}



int fPort = 1;               // fPort usage: 1=dht11, 2=button, 3=led
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
//-----------------------------

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
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
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(nwkKey[i]);
              }
              Serial.println();
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
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));

            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));

              //------ Added ----------------
              fPort = LMIC.frame[LMIC.dataBeg - 1];
              Serial.print(F("fPort "));
              Serial.println(fPort);
            
             Serial.println();
             //-----------------------------
            }                    
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
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
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

long calculate_distance(){
    // Read the distance from the ultrasonic sensor
    long duration, distance;
    digitalWrite(ultrasonicTrigPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(ultrasonicTrigPin, HIGH);
    delayMicroseconds(10); 
    digitalWrite(ultrasonicTrigPin, LOW);
    duration = pulseIn(ultrasonicEchoPin, HIGH);
    // Calculate the distance
    distance = duration * 0.034 / 2;

    return distance;
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {

        long distance = calculate_distance();
        uint32_t lotNo = 1;
        uint32_t infraredSensor = 0;
        if (digitalRead(irSensorPin) == 0){
          infraredSensor = 1;
        }
        else {
          infraredSensor = 0;
        }
        Serial.println(infraredSensor);
        Serial.println(distance);
        uint32_t ultrasonicSensor = 0;
        if (distance < 3 ){
          ultrasonicSensor = 1;
        }
        else{
          ultrasonicSensor = 0;
        }
        
        Serial.println(ultrasonicSensor);

        byte payload[16]; // Assuming AES encryption with 128-bit key size
        byte cipher[16]; // Assuming AES encryption with 128-bit key size
        byte key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};

        // Convert uint32_t variables to byte arrays
        byte lotNoBytes[4];
        byte ultrasonicSensorBytes[4];
        byte infraredSensorBytes[4];

        lotNoBytes[0] = (lotNo >> 24) & 0xFF;
        lotNoBytes[1] = (lotNo >> 16) & 0xFF;
        lotNoBytes[2] = (lotNo >> 8) & 0xFF;
        lotNoBytes[3] = lotNo & 0xFF;

        ultrasonicSensorBytes[0] = (ultrasonicSensor >> 24) & 0xFF;
        ultrasonicSensorBytes[1] = (ultrasonicSensor >> 16) & 0xFF;
        ultrasonicSensorBytes[2] = (ultrasonicSensor >> 8) & 0xFF;
        ultrasonicSensorBytes[3] = ultrasonicSensor & 0xFF;

        infraredSensorBytes[0] = (infraredSensor >> 24) & 0xFF;
        infraredSensorBytes[1] = (infraredSensor >> 16) & 0xFF;
        infraredSensorBytes[2] = (infraredSensor >> 8) & 0xFF;
        infraredSensorBytes[3] = infraredSensor & 0xFF;

        // Concatenate byte arrays to form the plaintext payload
        memcpy(payload, lotNoBytes, 4);
        memcpy(payload + 4, ultrasonicSensorBytes, 4);
        memcpy(payload + 8, infraredSensorBytes, 4);

        // Encrypt the payload using AES
        AES aes;
        aes.set_key(key, sizeof(key)); // Set encryption key
        aes.encrypt(payload, cipher);

        fPort = 1;
         
        //Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(fPort, cipher, sizeof(cipher), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(9600);

    // Set the IR sensor pin as input
    pinMode(irSensorPin, INPUT);
    // Set the ultrasonic sensor pins
    pinMode(ultrasonicTrigPin, OUTPUT);
    pinMode(ultrasonicEchoPin, INPUT);
    Serial.println(F("Starting"));
    //-----------------------------
    
    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Use with Arduino Pro Mini ATmega328P 3.3V 8 MHz
    // Let LMIC compensate for +/- 1% clock error
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); 

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}