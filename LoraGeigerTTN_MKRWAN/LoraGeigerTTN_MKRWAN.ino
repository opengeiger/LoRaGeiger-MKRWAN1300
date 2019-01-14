#include <MKRWAN.h>
#include <CayenneLPP.h>
#define MAXCNT 100
#define CalFactor 1

LoRaModem modem;

//#include "arduino_secrets.h"
// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = "";
String appKey = "";

CayenneLPP lpp(51);

volatile int counter = 0;
unsigned long oldTime = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);        //sign of live
  for (int i=1; i<=10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);
    // wait for a second
    Serial.println(10-i);
  }

  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());
  Serial.println("Trying to join TTN ...");
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
  }
  else
    Serial.println("Successfully joined");
  /*
  DataRate    Modulation    SF    BW    bit/s
  0    LoRa    12    125    250
  1    LoRa    11    125    440
  2    LoRa    10    125    980
  3    LoRa    9    125    1'760
  4    LoRa    8    125    3'125
  5    LoRa    7    125    5'470
  6    LoRa    7    250    11'000
  */
  modem.dataRate(5);   // switch to SF7
  //***********************************

  // Set poll interval to 60 secs.
  modem.minPollInterval(60);
  // NOTE: independently by this setting the modem will
  // not allow to send more than one message every 2 minutes,
  // this is enforced by firmware and can not be changed.

  attachInterrupt(digitalPinToInterrupt(0), count, FALLING);
  Serial.println("Start waiting for geiger pulses ...");
}

void loop()
{
  unsigned long time;
  unsigned long dt;
  float rate;
  int err;

  if (counter == MAXCNT) {
    detachInterrupt(digitalPinToInterrupt(0));
    time = millis();
    dt = time-oldTime;
    rate = (float)MAXCNT*60.0*1000.0/(float)dt/CalFactor;
    Serial.println(round(rate));
    lpp.reset();
    lpp.addLuminosity(1, round(rate));
    //************************************
    modem.dataRate(5);   // switch to SF7
    //************************************
    modem.beginPacket();
    modem.write(lpp.getBuffer(), lpp.getSize());
    err = modem.endPacket(true);
    if (err > 0) {
      Serial.println("Message sent correctly!");
    } else {
      Serial.println("Error sending message :(");
      Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
      Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
    }
    delay(1000);
    if (!modem.available()) {
      Serial.println("No downlink message received at this time.");
    }
    else {
      String rcv;
      rcv.reserve(64);
      while (modem.available()) {
        rcv += (char)modem.read();
      }
      Serial.print("Received: " + rcv + " - ");
      for (unsigned int i = 0; i < rcv.length(); i++) {
        Serial.print(rcv[i] >> 4, HEX);
        Serial.print(rcv[i] & 0xF, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println("waiting 60 seconds");
    delay(60000);

    oldTime = millis();
    counter = 0;
    attachInterrupt(digitalPinToInterrupt(0), count, FALLING);
  }
}

void count()
{
  counter++;
}
