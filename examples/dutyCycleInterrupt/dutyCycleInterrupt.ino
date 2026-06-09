/**
 * @file  dutyCycleInterrupt.ino
 * @brief  Read PM data from BMV080 Gravity firmware in duty-cycle mode with external interrupt.
 * @n  The BMV080's INT pin is brought out on the module. Connect it to the host MCU's interrupt pin.
 * @n  In duty-cycle mode, the sensor wakes periodically, measures for integration_time seconds,
 * @n  then toggles INT when data is ready, and sleeps for the remainder of the period.
 * @n  The INT pin only tells the host to service a firmware event. The main loop
 * @n  still checks getData(), which returns true only when new PM data is ready.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include <Wire.h>

#include "DFRobot_BMV080_Gravity.h"

/* >> 1. Please choose your communication method below: */
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

/**
 * Interrupt pin configuration.
 * Connect the BMV080 module's INT pin to this host MCU pin.
 */
#if defined(ESP32)
#define IRQ_PIN 14
#else
#define IRQ_PIN 2
#endif

/**
 * Duty-cycle timing parameters.
 * @param DUTY_CYCLE_PERIOD    Total cycle time in seconds.
 * @param INTEGRATION_TIME     Sensor ON time per cycle in seconds.
 */
#define DUTY_CYCLE_PERIOD 30
#define INTEGRATION_TIME  10.0f

volatile bool dataFlag = false;

void onInterrupt(void)
{
  dataFlag = true;
}

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
   * Configure duty-cycle timing.
   */
  if (sensor.setDutyCyclingPeriod(DUTY_CYCLE_PERIOD) != 0) {
    Serial.println("Set duty-cycle period failed.");
  }
  if (sensor.setIntegrationTime(INTEGRATION_TIME) != 0) {
    Serial.println("Set integration time failed.");
  }

  /**
   * Configure measurement algorithm and filtering.
   * @note Duty-cycle measurement uses eFastResponse according to the BMV080 SDK.
   */
  sensor.setMeasurementAlgorithm(DFRobot_BMV080_Gravity::eFastResponse);
  Serial.print("Algorithm: ");
  Serial.println(sensor.getMeasurementAlgorithm());

  sensor.setObstructionDetection(true);
  Serial.print("Obstruction Detection: ");
  Serial.println(sensor.getObstructionDetection());

  sensor.setVibrationFiltering(true);
  Serial.print("Vibration Filtering: ");
  Serial.println(sensor.getVibrationFiltering());

  /**
   * Start duty-cycle measurement.
   * Firmware applies cached parameters before (re)starting measurement.
   */
  if (sensor.setMeasureMode(DFRobot_BMV080_Gravity::eDutyCycleMode) == 0) {
    Serial.println("Duty-cycle measurement started.");
  } else {
    Serial.println("Start measurement failed.");
  }

  /**
   * Configure hardware interrupt on the BMV080 INT pin.
   * Each measurement cycle completes with an INT pulse.
   */
  pinMode(IRQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), onInterrupt, RISING);
  Serial.print("Interrupt enabled on pin ");
  Serial.println(IRQ_PIN);
}

void loop()
{
  DFRobot_BMV080_Gravity::sData_t data;

  if (dataFlag) {
    if (sensor.getData(&data)) {
      dataFlag = false;
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
  }
}
