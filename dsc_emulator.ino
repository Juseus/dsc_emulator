/*
 * BMW DSC Emulator
 *
 */
#include <FreqMeasure.h>
#include <TimerEvent.h>
#include <SPI.h>
#include <mcp2515.h>

//#define DEBUG

// CAN
MCP2515 mcp2515(10);
struct can_frame canMsg;
unsigned int tmpAvgSpeed = 0;
unsigned int msbAvgSpeed = 0;
unsigned int lsbAvgSpeed = 0;

// Counterd
double speedFactor = 6.92; // 1km/h <-> 6,92Hz
double currentFreq = 0;
int sampleCount = 0;
double freqSum = 0;
double freqCheck = 0;
unsigned long int prevTime = millis();
unsigned long int currentTime = millis();
unsigned long int elapsedTime = 0;
unsigned long int loopTime = 0;

const unsigned int canMsgPeriod = 20; // DSC message interval is 20ms
const unsigned int measureTimeoutEventPeriod = 150; // 1km/h <-> 6,92Hz <-> 144,5ms period
const unsigned int numberOfSamples = 1;

// Create TimerEvent instance
TimerEvent canMsgEvent;
TimerEvent measureTimeoutEvent;

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  // CAN
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
  mcp2515.setNormalMode();

  FreqMeasure.begin();
  randomSeed( (unsigned long)( micros()%millis() ) );

  // Set the interval timing as well as pass the function name
  canMsgEvent.set(canMsgPeriod, canMsgFunc);
  measureTimeoutEvent.set(measureTimeoutEventPeriod, measureTimeoutFunc);
}

void loop() {
  vehicleSpeed();
  canMsgEvent.update();
  measureTimeoutEvent.update();
}

void vehicleSpeed() {
  // measurement will never be available for 0hz -> need to handle via measureTimeoutEvent!
  if (FreqMeasure.available()) {
    // average several reading together
    freqSum += FreqMeasure.read();
    sampleCount += 1;
    // measure consumed time
    currentTime = micros();
    elapsedTime = currentTime - prevTime;
    loopTime = round(elapsedTime / sampleCount);
    if ((elapsedTime + loopTime) > 20000) {
      freqCheck = FreqMeasure.countToFrequency(freqSum / sampleCount);
      // all frequencies below speedFactor -> set to zero
      if (freqCheck < speedFactor) {
        currentFreq = 0;
      } else {
        currentFreq = freqCheck;
      }
      freqSum = 0;
      sampleCount = 0;
      prevTime = currentTime;
    }
    measureTimeoutEvent.reset();
  }
}

void measureTimeoutFunc() {
  currentFreq = 0;
}

void canMsgFunc() {

  // Calculate average speed
  tmpAvgSpeed = round(currentFreq / speedFactor * double(127.0) + 352);
  msbAvgSpeed = tmpAvgSpeed >> 8;
  // Take LSB and clear last 3 bits as those are status bits
  lsbAvgSpeed = tmpAvgSpeed & 0xF8;

  // CAN Arbitration ID for ASC1 is 0x153.
  // It's sent out by the traction control module at a refresh rate of 10ms for ASC and 20ms for DSC.
  canMsg.can_id  = 0x153;
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x00;
  canMsg.data[1] = lsbAvgSpeed;
  canMsg.data[2] = msbAvgSpeed;
  canMsg.data[3] = 0xFF;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0xFF;
  canMsg.data[6] = 0xFF;
  canMsg.data[7] = 0x00;

  mcp2515.sendMessage(&canMsg);

  #ifdef DEBUG
  Serial.print((String)"Frequency: " + currentFreq);
  Serial.print(" ");
  Serial.print((String)"Speed: " + round(currentFreq / speedFactor));
  Serial.print(" ");
  Serial.print("LSB: 0x" );
  Serial.print(lsbAvgSpeed, HEX);
  Serial.print(" ");
  Serial.print("MSB: 0x" );
  Serial.print(msbAvgSpeed, HEX);
  Serial.print("\n");
  #endif
}
