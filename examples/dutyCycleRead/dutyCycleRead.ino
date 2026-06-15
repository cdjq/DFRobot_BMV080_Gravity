/**
 * @file  dutyCycleRead.ino
 * @brief  Read PM1.0, PM2.5 and PM10 mass concentration in duty-cycle measurement mode.
 * @n  This demo configures all measurement parameters and starts duty-cycle measurement.
 * @n  Duty-cycle mode: the sensor measures for integration_time seconds, then sleeps for the remainder of the period.
 * @n  The duty_cycling_period must be at least integration_time + 2 seconds.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      [thdyyl](yuanlong.yu@dfrobot.com)
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include "DFRobot_BMV080_Gravity.h"

// #define BMV080_COMM_UART
#define BMV080_COMM_I2C

/**
 * Duty-cycle timing parameters.
 * @param DUTY_CYCLE_PERIOD    Total cycle time in seconds.
 * @param INTEGRATION_TIME     Sensor ON time per cycle in seconds.
 */
#define DUTY_CYCLE_PERIOD 30
#define INTEGRATION_TIME  10.0f

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
   * Configure measurement algorithm and filtering options.
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
   * setMeasureMode(eDutyCycleMode) writes MEASURE_MODE=1 then action value 1.
   * Firmware applies cached parameters before (re)starting measurement.
   * The firmware will cycle the sensor ON/OFF according to the configured period.
   */
  if (sensor.setMeasureMode(DFRobot_BMV080_Gravity::eDutyCycleMode) == 0) {
    Serial.println("Duty-cycle measurement started.");
  } else {
    Serial.println("Start measurement failed.");
  }
}

void loop()
{
  DFRobot_BMV080_Gravity::sData_t data;

  /**
   * getData returns true only when a new duty-cycle sample is ready.
   *
   * data contains:
   *   PM1 / PM2_5 / PM10: PM1.0, PM2.5 and PM10 mass concentration (ug/m3)
   *   runtime: sensor runtime in seconds
   *   runState/status: firmware run state and status code
   *   isObstructed / isOutsideMeasurementRange: warning flags
   *   measuring / paramsVerified: current measurement state flags
   *   sampleSeq: sample sequence number
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
