// All settings can be changed in settings.h

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <CayenneLPP.h>
#include "DHT.h"
#include "settings.h"

// BME280 I2C address is 0x76(108)
#define Addr 0x76

#define DHTPIN 9     // digital pin DHT22

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

CayenneLPP lpp(53);
DHT dht(DHTPIN, DHTTYPE);
double cTemp0;    // from Pressure Sensor
double cTemp1;    // Fron DHT22
double pressure;  // from Pressure Sensor
double humidity;  // Fron DHT22
int batteryRAW;
float battery;

#ifdef SLEEP
  #include "LowPower.h"
  bool next = false;
#endif

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {2, 3, 4},
};

void burn8Readings(int pin) {
  for (int i = 0; i < 8; i++) {
    analogRead(pin);
  }
}

void getSensorData(){
  unsigned int b1[24];
  unsigned int data[8];
  unsigned int dig_H1 = 0;

  // Pressure Sensor
  for(int i = 0; i < 24; i++) {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((136+i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 24 bytes of data
    if(Wire.available() == 1)
    {
      b1[i] = Wire.read();
    }
  }

  // Convert the data
  // temp coefficients
  unsigned int dig_T1 = (b1[0] & 0xff) + ((b1[1] & 0xff) * 256);
  int dig_T2 = b1[2] + (b1[3] * 256);
  int dig_T3 = b1[4] + (b1[5] * 256);

  // pressure coefficients
  unsigned int dig_P1 = (b1[6] & 0xff) + ((b1[7] & 0xff ) * 256);
  int dig_P2 = b1[8] + (b1[9] * 256);
  int dig_P3 = b1[10] + (b1[11] * 256);
  int dig_P4 = b1[12] + (b1[13] * 256);
  int dig_P5 = b1[14] + (b1[15] * 256);
  int dig_P6 = b1[16] + (b1[17] * 256);
  int dig_P7 = b1[18] + (b1[19] * 256);
  int dig_P8 = b1[20] + (b1[21] * 256);
  int dig_P9 = b1[22] + (b1[23] * 256);

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select data register
  Wire.write(161);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Request 1 byte of data
  Wire.requestFrom(Addr, 1);

  // Read 1 byte of data
  if(Wire.available() == 1) {
    dig_H1 = Wire.read();
  }

  for(int i = 0; i < 7; i++) {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((225+i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 7 bytes of data
    if(Wire.available() == 1)
    {
      b1[i] = Wire.read();
    }
  }

  // Convert the data
  // humidity coefficients
  int dig_H2 = b1[0] + (b1[1] * 256);
  unsigned int dig_H3 = b1[2] & 0xFF ;
  int dig_H4 = (b1[3] * 16) + (b1[4] & 0xF);
  int dig_H5 = (b1[4] / 16) + (b1[5] * 16);
  int dig_H6 = b1[6];

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select control humidity register
  Wire.write(0xF2);
  // Humidity over sampling rate = 1
  Wire.write(0x01);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select control measurement register
  Wire.write(0xF4);
  // Normal mode, temp and pressure over sampling rate = 1
  Wire.write(0x27);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select config register
  Wire.write(0xF5);
  // Stand_by time = 1000ms
  Wire.write(0xA0);
  // Stop I2C Transmission
  Wire.endTransmission();

  for(int i = 0; i < 8; i++) {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((247+i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 8 bytes of data
    if(Wire.available() == 1)
    {
      data[i] = Wire.read();
    }
  }

  // Convert pressure and temperature data to 19-bits
  long adc_p = (((long)(data[0] & 0xFF) * 65536) + ((long)(data[1] & 0xFF) * 256) + (long)(data[2] & 0xF0)) / 16;
  long adc_t = (((long)(data[3] & 0xFF) * 65536) + ((long)(data[4] & 0xFF) * 256) + (long)(data[5] & 0xF0)) / 16;
  // Convert the humidity data
  long adc_h = ((long)(data[6] & 0xFF) * 256 + (long)(data[7] & 0xFF));

  // Temperature offset calculations
  double var1 = (((double)adc_t) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
  double var2 = ((((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0) *
  (((double)adc_t)/131072.0 - ((double)dig_T1)/8192.0)) * ((double)dig_T3);
  double t_fine = (long)(var1 + var2);
  cTemp0 = (var1 + var2) / 5120.0;

  // Pressure offset calculations
  var1 = ((double)t_fine / 2.0) - 64000.0;
  var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
  var2 = var2 + var1 * ((double)dig_P5) * 2.0;
  var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
  var1 = (((double) dig_P3) * var1 * var1 / 524288.0 + ((double) dig_P2) * var1) / 524288.0;
  var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
  double p = 1048576.0 - (double)adc_p;
  p = (p - (var2 / 4096.0)) * 6250.0 / var1;
  var1 = ((double) dig_P9) * p * p / 2147483648.0;
  var2 = p * ((double) dig_P8) / 32768.0;
  pressure = (p + (var1 + var2 + ((double)dig_P7)) / 16.0) / 100;

  // DHT22
  cTemp1 = dht.readTemperature();
  humidity = dht.readHumidity();

  batteryRAW = analogRead(A2);
  battery = batteryRAW*32.0/10240.0*1.10;

  #ifdef DEBUG
    Serial.println(F("Messured values:"));
    Serial.println(F(" Pressure Sensor:"));
    Serial.print(F("  Pressure:"));
    Serial.print(pressure);
    Serial.print(F(" Temperature:"));
    Serial.print(cTemp0);
    Serial.println();
    
    Serial.println(F(" Humidity Sensor:"));
    Serial.print(F("  Humidity:"));
    Serial.print(humidity);
    Serial.print(F(" Temperature:"));
    Serial.print(cTemp1);
    Serial.println();

    Serial.print(F(" BatteryRAW: "));
    Serial.println(batteryRAW);
    Serial.print(F(" Battery: "));
    Serial.println(battery);
  #endif
}

void onEvent (ev_t ev) {
    #ifdef DEBUG
      Serial.println(F("Enter onEvent"));
    #endif

    switch(ev) {
        case EV_TXCOMPLETE:
            #ifdef DEBUG
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            #endif
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                #ifdef DEBUG
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
                #endif
            }
            // Schedule next transmission
            #ifndef SLEEP
               os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            #else
               next = true;
            #endif
            break;
      #ifdef DEBUG
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
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
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
      #endif
         default:
            #ifdef DEBUG
            Serial.println(F("Unknown event"));
            #endif
            break;
    }
    #ifdef DEBUG
      Serial.println(F("Leave onEvent"));
    #endif
}

void do_send(osjob_t* j){

    #ifdef DEBUG
      Serial.println(F("Enter do_send"));
    #endif

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {

        getSensorData();
        // Prepare upstream data transmission at the next possible time.
        lpp.reset();
        lpp.addTemperature(1, cTemp0);
      //  lpp.addTemperature(2, cTemp1);
        lpp.addBarometricPressure(2, pressure);
        lpp.addRelativeHumidity(3, humidity);
        lpp.addAnalogInput(4, battery);

        LMIC.txChnl = 0;

        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
        #ifdef DEBUG
        Serial.print(F("Packet queued, Freq: "));
        Serial.println(LMIC.freq);
        #endif
    }
    // Next TX is scheduled after TX_COMPLETE event.
    #ifdef DEBUG
      Serial.println(F("Leave do_send"));
    #endif
}

void setup() {
  init();

  #ifdef DEBUG
  Serial.begin(9600);
  #endif

  // Initialise I2C communication as MASTER
  Wire.begin();

  dht.begin();

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));

  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.


  // Disable link check validation
  
   LMIC_setLinkCheckMode(0);
  // Uncomment to use channel 868100000 only
  // for(int i=0; i<9; i++) { // For EU; for US use i<71
  //     if(i != 0) {
  //         LMIC_disableChannel(i);
  //     }
  // }

  // Set data rate (SF) and transmit power for uplink
  LMIC_setDrTxpow(DR_SF12, 14);

  // Switch ADC reference to internal and clear capacitors
  analogReference(INTERNAL);
  burn8Readings(A2);
  delay(10);

  // Start job
  do_send(&sendjob);
}

void loop() {
    #ifndef SLEEP
      os_runloop_once();
      delay(TX_INTERVAL*1000);
      // Start job
      do_send(&sendjob);
    #else
      if (next == false) {
        os_runloop_once();
      } else {

        int sleepcycles = TX_INTERVAL / 8;  // calculate the number of sleepcycles (8s) given the TX_INTERVAL
        #ifdef DEBUG
          Serial.print(F("Enter sleeping for "));
          Serial.print(sleepcycles);
          Serial.println(F(" cycles of 8 seconds"));
        #endif
        Serial.flush(); // give the serial print chance to complete

        extern volatile unsigned long timer0_overflow_count;
        
        for (int i=0; i<sleepcycles; i++) {
          // Enter power down state for 8 s with ADC and BOD module disabled
          LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
          //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);

          // LMIC uses micros() to keep track of the duty cycle, so
          // hack timer0_overflow for a rude adjustment:
          cli();
          timer0_overflow_count+= 8 * 64 * clockCyclesPerMicrosecond();
          sei();
        }
        #ifdef DEBUG
          Serial.println(F("Sleep complete"));
        #endif
       
        next = false;
        // Start job
        do_send(&sendjob);
      }
    #endif
}
