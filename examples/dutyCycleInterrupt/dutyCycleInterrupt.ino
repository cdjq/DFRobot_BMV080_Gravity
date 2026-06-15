/**
 * @file  dutyCycleInterrupt.ino
 * @brief  Read PM data from BMV080 Gravity firmware in duty-cycle mode with external interrupt.
 * @n  The BMV080's INT pin is brought out on the module. Connect it to the host MCU's interrupt pin.
 * @n  In duty-cycle mode, the sensor wakes periodically, measures for integration_time seconds,
 * @n  then reports an INT event when data is ready, and sleeps for the remainder of the period.
 * @n  This demo watches the falling edge of INT. The ISR only sets a flag; I2C/UART
 * @n  communication and Serial printing are handled in loop().
 * @n  The main loop still checks getData(), which returns true only when a new PM sample is ready.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      [thdyyl](yuanlong.yu@dfrobot.com)
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include "DFRobot_BMV080_Gravity.h"

/* >> 1. Please choose your communication method below: */
// #define BMV080_COMM_UART
#define BMV080_COMM_I2C

/**
 * Duty-cycle timing parameters.
 * @param DUTY_CYCLE_PERIOD    Total cycle time in seconds.
 * @param INTEGRATION_TIME     Sensor ON time per cycle in seconds.
 */
#define DUTY_CYCLE_PERIOD 30
#define INTEGRATION_TIME  10.0f

volatile uint8_t dataFlag = 0;

#if defined(ESP8266)
#ifndef IRAM_ATTR
#define IRAM_ATTR ICACHE_RAM_ATTR
#endif
void IRAM_ATTR onInterrupt(void)
#else
void onInterrupt(void)
#endif
{
  if (dataFlag == 0) {
    dataFlag = 1;
  }
}

#if defined(BMV080_COMM_UART)
const uint8_t UART_ADDR = 0x57;
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
#elif defined(ARDUINO_BBC_MICROBIT) || defined(ARDUINO_BBC_MICROBIT_V2)
#error "BBC micro:bit (nRF51, sandeepmistry/nRF5): Serial1 is not defined. Use I2C in this sketch (#define BMV080_COMM_I2C) or a board with Serial1."
#else
DFRobot_BMV080_Gravity_UART sensor(&Serial1, 9600, UART_ADDR);
#endif
#elif defined(BMV080_COMM_I2C)
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
DFRobot_BMV080_Gravity_I2C sensor(&Wire, I2C_ADDR);
#else
#error "Please select BMV080_COMM_I2C or BMV080_COMM_UART."
#endif

void setup()
{
  delay(2000);
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
   * Configure duty-cycle timing parameters.
   *
   * DUTY_CYCLE_PERIOD is the total cycle time in seconds.
   * INTEGRATION_TIME is the sensor ON/measuring time in each cycle.
   *
   * In duty-cycle mode, DUTY_CYCLE_PERIOD must be at least
   * INTEGRATION_TIME + 2 seconds. The example values below are valid:
   * 30 seconds >= 10 seconds + 2 seconds.
   *
   * Configuration order matters when changing existing settings:
   * - If increasing INTEGRATION_TIME beyond the current period margin,
   *   call setDutyCyclingPeriod() first to enlarge the period.
   * - If shortening DUTY_CYCLE_PERIOD, lower INTEGRATION_TIME first when
   *   the new period would be less than integration time + 2 seconds.
   */
  if (sensor.setDutyCyclingPeriod(DUTY_CYCLE_PERIOD) != 0) {
    Serial.println("Set duty-cycle period failed.");
  }
  Serial.print("DutyCycle: ");
  Serial.println(sensor.getDutyCyclingPeriod());

  if (sensor.setIntegrationTime(INTEGRATION_TIME) != 0) {
    Serial.println("Set integration time failed.");
  }
  Serial.print("IntegrationTime: ");
  Serial.println(sensor.getIntegrationTime());

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
   * The callback must stay short: only set dataFlag, then return.
   */
#if defined(ESP32)
  // D6 pin is used as interrupt pin by default, other non-conflicting pins can also be selected as external interrupt pins.
  pinMode(14 /*D6*/, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(14 /*D6*/) /* Query the interrupt number of the D6 pin */, onInterrupt, FALLING);
#elif defined(ESP8266)
#if defined(BMV080_COMM_UART)
  const uint8_t interruptPin = 12;
#elif defined(BMV080_COMM_I2C)
  const uint8_t interruptPin = 13;
#else
#error "Please select BMV080_COMM_I2C or BMV080_COMM_UART."
#endif
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), onInterrupt, FALLING);
#elif defined(ARDUINO_SAM_ZERO) || defined(ARDUINO_SAMD_ZERO)
  // Pin 6 is used as interrupt pin by default, other non-conflicting pins can also be selected as external interrupt pins.
  pinMode(6, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(6) /* Query the interrupt number of the 6 pin */, onInterrupt, FALLING);
#else
  /* The Correspondence Table of AVR Series Arduino Interrupt Pins And Terminal Numbers
     * ---------------------------------------------------------------------------------------
     * |                                        |  DigitalPin  | 2  | 3  |                   |
     * |    Uno, Nano, Mini, other 328-based    |--------------------------------------------|
     * |                                        | Interrupt No | 0  | 1  |                   |
     * |-------------------------------------------------------------------------------------|
     * |                                        |    Pin       | 2  | 3  | 21 | 20 | 19 | 18 |
     * |               Mega2560                 |--------------------------------------------|
     * |                                        | Interrupt No | 0  | 1  | 2  | 3  | 4  | 5  |
     * |-------------------------------------------------------------------------------------|
     * |                                        |    Pin       | 3  | 2  | 0  | 1  | 7  |    |
     * |    Leonardo, other 32u4-based          |--------------------------------------------|
     * |                                        | Interrupt No | 0  | 1  | 2  | 3  | 4  |    |
     * |--------------------------------------------------------------------------------------
     * ---------------------------------------------------------------------------------------------------------------------------------------------
     *                      The Correspondence Table of micro:bit Interrupt Pins And Terminal Numbers
     * ---------------------------------------------------------------------------------------------------------------------------------------------
     * |             micro:bit                       | DigitalPin | P0-P20 can be used as an external interrupt                                    |
     * |                                             |---------------------------------------------------------------------------------------------|
     * |                                             |Interrupt No| Interrupt number is a pin digital value, such as P0 interrupt number 0, P1 is 1 |
     * |-------------------------------------------------------------------------------------------------------------------------------------------|
     */
#if defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO)
  pinMode(3, INPUT_PULLUP);
#elif defined(ARDUINO_BBC_MICROBIT) || defined(ARDUINO_BBC_MICROBIT_V2)
  pinMode(0, INPUT_PULLUP);
#else
  pinMode(2, INPUT_PULLUP);
#endif
  attachInterrupt(/*Interrupt No*/ 0, onInterrupt, FALLING);    // Open the external interrupt 0, connect INT to the digital pin of the main control:
                                                                // UNO(2), Mega2560(2), Leonardo(3), microbit(P0).
#endif
}

void loop()
{
  DFRobot_BMV080_Gravity::sData_t data;

  if (dataFlag == 1) {
    /**
     * getData fills data with PM concentrations, runtime, runState/status,
     * warning flags such as isObstructed/isOutsideMeasurementRange, and sampleSeq.
     */
    if (sensor.getData(&data)) {
      dataFlag = 0;
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
