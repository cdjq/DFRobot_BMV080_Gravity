/**
 * @file  dutyCycleInterrupt.ino
 * @brief  Read PM data from BMV080 Gravity firmware in duty-cycle mode with external interrupt.
 * @n  The BMV080's INT pin is brought out on the module. Connect it to the host MCU's interrupt pin.
 * @n  In duty-cycle mode, the sensor wakes periodically, measures for integration_time seconds,
 * @n  then toggles INT when data is ready, and sleeps for the remainder of the period.
 * @n  The main loop only needs to call getBmv080Data(). When new data is available,
 * @n  getBmv080Data() returns true and fills the data structure.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-05-11
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include "DFRobot_BMV080_Gravity.h"

/* >> 1. Please choose your communication method below: */
// #define BMV080_COMM_UART
#define BMV080_COMM_I2C

const uint8_t ADDR = 0x57;

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
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
#include <SoftwareSerial.h>
SoftwareSerial              mySerial(/*rx =*/4, /*tx =*/5);
DFRobot_BMV080_Gravity_UART sensor(&mySerial, 9600, ADDR);
#elif defined(ESP32)
DFRobot_BMV080_Gravity_UART sensor(&Serial1, 9600, ADDR, /*rx =*/25, /*tx =*/26);
#else
DFRobot_BMV080_Gravity_UART sensor(&Serial1, 9600, ADDR);
#endif
#elif defined(BMV080_COMM_I2C)
DFRobot_BMV080_Gravity_I2C sensor(&Wire, ADDR);
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
    Serial.print("Sensor init failed, last error: ");
    Serial.println(sensor.getLastError());
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
  Serial.print("Duty Period: ");
  Serial.print(sensor.getDutyCyclingPeriod());
  Serial.print(" s  Integration Time: ");
  Serial.print(sensor.getIntegrationTime());
  Serial.println(" s");

  /**
   * Configure measurement algorithm and filtering.
   */
  sensor.setMeasurementAlgorithm(BALANCED);
  Serial.print("Algorithm: ");
  Serial.println(sensor.getMeasurementAlgorithm());

  sensor.setObstructionDetection(true);
  Serial.print("Obstruction Detection: ");
  Serial.println(sensor.getObstructionDetection());

  sensor.setDoVibrationFiltering(true);
  Serial.print("Vibration Filtering: ");
  Serial.println(sensor.getDoVibrationFiltering());

  /**
   * Start duty-cycle measurement.
   * Firmware applies cached parameters before (re)starting measurement.
   */
  if (sensor.setBmv080Mode(DUTY_CYCLE_MODE) == 0) {
    Serial.println("Duty-cycle measurement started.");
  } else {
    Serial.print("Start failed, last error: ");
    Serial.println(sensor.getLastError());
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
  DFRobot_BMV080_Gravity::sBmv080Data_t data;

  if (dataFlag) {
    if (sensor.getBmv080Data(&data)) {
      dataFlag = false;
      Serial.print("PM1.0: ");
      Serial.print(data.PM1);
      Serial.print(" ug/m3  PM2.5: ");
      Serial.print(data.PM2_5);
      Serial.print(" ug/m3  PM10: ");
      Serial.print(data.PM10);
      Serial.print(" ug/m3  runtime: ");
      Serial.print(data.runtime);
      Serial.print(" s  runState: ");
      Serial.print(data.runState);

      if (data.isObstructed) {
        Serial.print("  Obstructed");
      }
      if (data.isOutsideMeasurementRange) {
        Serial.print("  Outside range");
      }
      if (data.paramsVerified) {
        Serial.print("  ParamsVerified");
      }
      Serial.println();
    }
  }
}
