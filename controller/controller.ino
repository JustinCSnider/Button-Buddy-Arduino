
/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <Timer.h>

Adafruit_7segment matrix = Adafruit_7segment();

// OTA DFU service
BLEDfu bledfu;

// Uart over BLE service
BLEUart bleuart;

Timer t;
uint8_t timerValue;
uint8_t currentTimerValue;
boolean timerValueSet = false;
int timerZeroFlashCount = 0;
int timerZeroFlashTimeOut = 3;

int buttonPin = A0;
int previoiusButtonPinValue = 0;
boolean buttonPressed = false;
boolean resetButtonPressed = false;
int lastButtonPressCount = 0;

// Function prototypes for packetparser.cpp
uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

// Packet buffer
extern uint8_t packetbuffer[];

void setup(void)
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println(F("Adafruit Bluefruit52 Controller App Example"));
  Serial.println(F("-------------------------------------------"));

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName("Button Buddy");

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();
  
  #ifndef __AVR_ATtiny85__
    Serial.begin(9600);
    Serial.println("7 Segment Backpack Test");
  #endif

  matrix.begin(0x70);
  t.every(1000, checkTimer);
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  lastButtonPressCount++;
  if (analogRead(buttonPin) > 900) {
    if (previoiusButtonPinValue < 900) {
      Serial.println("Button Pressed");
      if (lastButtonPressCount < 4) {
        resetButtonPressed = true;
      } else {
        buttonPressed = !buttonPressed;
      }
      lastButtonPressCount = 0;
    }
  }
  previoiusButtonPinValue = analogRead(buttonPin);
  t.update();
  // Wait for new data to arrive
  uint8_t len = readPacket(&bleuart, 500);
  if (len == 0) return;

  // Got a packet!
  // printHex(packetbuffer, len);

  // Timer
  if (packetbuffer[1] == 'T') {
    uint8_t value = packetbuffer[2];
    timerValue = value;
    currentTimerValue = value;
    Serial.println();
    Serial.println("Timer Value: ");
    Serial.print(timerValue);
    if (timerValue % 60 == 0) {
      matrix.println((timerValue / 60) * 100);
      matrix.drawColon(true);
    } else if (timerValue < 60) {
      matrix.println(timerValue, DEC);
      matrix.drawColon(false);
    } else {
      int finalValue = ((int)(timerValue / 60) * 100) + timerValue % 60;
      matrix.println(finalValue, DEC);
      matrix.drawColon(true);
    }
    matrix.writeDisplay();
    timerValueSet = true;
  }

  // Color
  if (packetbuffer[1] == 'C') {
    uint8_t red = packetbuffer[2];
    uint8_t green = packetbuffer[3];
    uint8_t blue = packetbuffer[4];
    Serial.print ("RGB #");
    if (red < 0x10) Serial.print("0");
    Serial.print(red, HEX);
    if (green < 0x10) Serial.print("0");
    Serial.print(green, HEX);
    if (blue < 0x10) Serial.print("0");
    Serial.println(blue, HEX);
  }
}

void checkTimer() {
    if ((!timerValueSet || !buttonPressed) && !resetButtonPressed) return;
    
    if (currentTimerValue > 0) {
      matrix.blinkRate(0);
      currentTimerValue -= 1;
    }
    
    if (resetButtonPressed) {
      resetButtonPressed = false;
      buttonPressed = false;
      currentTimerValue = timerValue;
    }
    
    if (currentTimerValue % 60 == 0) {
      matrix.println((currentTimerValue / 60) * 100);
      matrix.drawColon(true);
    } else if (currentTimerValue < 60) {
      matrix.println(currentTimerValue, DEC);
      matrix.drawColon(false);
    } else {
      int finalValue = ((int)(currentTimerValue / 60) * 100) + currentTimerValue % 60;
      matrix.println(finalValue, DEC);
      matrix.drawColon(true);
    }
    if (currentTimerValue <= 0) {
      matrix.writeDigitNum(0, 0);
      matrix.writeDigitNum(1, 0);
      matrix.drawColon(true);
      matrix.writeDigitNum(3, 0);
      matrix.writeDigitNum(4, 0);
      matrix.blinkRate(1);
      timerZeroFlashCount += 1;
      if (timerZeroFlashCount >= timerZeroFlashTimeOut) {
        timerValueSet = false;
        buttonPressed = false;
        timerZeroFlashCount = 0;
        matrix.blinkRate(0);
      }
    }
    matrix.writeDisplay();
}
