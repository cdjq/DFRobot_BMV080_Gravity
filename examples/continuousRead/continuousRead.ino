/**
 * @file  continuousRead.ino
 * @brief  Read PM1.0, PM2.5 and PM10 mass concentration from BMV080 Gravity firmware in continuous measurement mode.
 * @n  This demo starts continuous measurement and polls PM data every 100 ms.
 * @n  The BMV080 Gravity module firmware manages the sensor handle and measurement state internally.
 * @n  The module supports two external communication modes: I2C slave and UART Modbus RTU.
 * @n  Select the communication mode with the macro below, matching the module's GPIO4 level at boot.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include <Wire.h>
#include "DFRobot_BMV080_Gravity.h"

/* >> 1. Please choose your communication method below:
 * I2C mode: set module GPIO4 HIGH before ESP32 firmware boots.
 * UART mode: set module GPIO4 LOW before ESP32 firmware boots.
 */
// #define BMV080_COMM_UART
#define BMV080_COMM_I2C

/**
 * I2C_ADDR is selected by A0/A1 pins.
 * UART_ADDR is the module's current Modbus RTU address. To change the saved UART address,
 * use a serial/Modbus tool, then update UART_ADDR here before using UART mode.
 * --------------------------------------
 * |    A0     |    A1     |  Address   |
 * --------------------------------------
 * |     0     |     0     |   0x54     |
 * |     0     |     1     |   0x55     |
 * |     1     |     0     |   0x56     |
 * |     1     |     1     |   0x57     |
 * --------------------------------------
 */
const uint8_t I2C_ADDR  = 0x57;
const uint8_t UART_ADDR = 0x57;

#if defined(BMV080_COMM_UART)
/* ---------------------------------------------------------------------------------------------------------------------
 *    board   |             MCU                | Leonardo/Mega2560/M0 |    UNO    | ESP8266 | ESP32 |  microbit  |   m0  |
 *     VCC    |            3.3V/5V             |        VCC           |    VCC    |   VCC   |  VCC  |     X      |  vcc  |
 *     GND    |              GND               |        GND           |    GND    |   GND   |  GND  |     X      |  gnd  |
 *     RX     |              TX                |     Serial1 TX1      |     5     |   5/D6  |  26/D3|     X      |  tx1  |
 *     TX     |              RX                |     Serial1 RX1      |     4     |   4/D7  |  25/D2|     X      |  rx1  |
 * ----------------------------------------------------------------------------------------------------------------------*/
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
#include <SoftwareSerial.h>
SoftwareSerial              mySerial(/*rx =*/4, /*tx =*/5);
DFRobot_BMV080_Gravity_UART sensor(&mySerial, 9600, UART_ADDR);
#elif defined(ESP32)
DFRobot_BMV080_Gravity_UART sensor(&Serial1, 9600, UART_ADDR, /*rx =*/25, /*tx =*/26);
#else
DFRobot_BMV080_Gravity_UART sensor(&Serial1, 9600, UART_ADDR);
#endif
#elif defined(BMV080_COMM_I2C)
DFRobot_BMV080_Gravity_I2C sensor(&Wire, I2C_ADDR);
#else
#error "Please select BMV080_COMM_I2C or BMV080_COMM_UART."
#endif

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  while (!sensor.begin()) {
    Serial.println("Sensor init failed.");
    delay(1000);
  }
  Serial.println("BMV080 Gravity init succeeded.");

  /**
   * Start continuous measurement.
   * setMeasureMode(eContinuousMode) writes action value 1.
   * The firmware will keep the sensor measuring continuously.
   */
  if (sensor.setMeasureMode(DFRobot_BMV080_Gravity::eContinuousMode) == 0) {
    Serial.println("Continuous measurement started.");
  } else {
    Serial.println("Start measurement failed.");
  }
}

void loop()
{
  DFRobot_BMV080_Gravity::sData_t data;

  /**
   * Read PM data from firmware.
   * getData returns true only when dataReady flag is set.
   */
  if (sensor.getData(&data)) {
    Serial.print("PM1.0: ");
    Serial.print(data.PM1);
    Serial.print(" ug/m3  PM2.5: ");
    Serial.print(data.PM2_5);
    Serial.print(" ug/m3  PM10: ");
    Serial.print(data.PM10);
    Serial.print(" ug/m3 ");
    if (data.isObstructed) {
      Serial.print("  Obstructed");
    }
    if (data.isOutsideMeasurementRange) {
      Serial.print("  Outside range");
    }
    Serial.println();
  }

  delay(100);
}
