/**
 * @file  consecutiveInterrupt.ino
 * @brief  Read PM data from BMV080 Gravity firmware in continuous mode with external interrupt.
 * @n  The BMV080's INT pin is brought out on the module. Connect it to the host MCU's interrupt pin.
 * @n  When new PM data is ready, the sensor toggles the INT pin. The ISR sets a flag,
 * @n  and the main loop reads data from the ESP32 firmware via I2C/UART.
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

/**
 * The external address is selected by A0/A1.
 */
const uint8_t ADDR = 0x57;

/**
 * Interrupt pin configuration.
 * Connect the BMV080 module's INT pin to this host MCU pin.
 * ESP32: any GPIO (e.g., 14)  |  UNO: pin 2 or 3  |  Mega: 2,3,18,19,20,21
 */
#if defined(ESP32)
  #define IRQ_PIN 14
#else
  #define IRQ_PIN 2
#endif

volatile bool dataFlag = false;

void onInterrupt(void)
{
  dataFlag = true;
}

#if defined(BMV080_COMM_UART)
  #if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
    #include <SoftwareSerial.h>
    SoftwareSerial mySerial(/*rx =*/4, /*tx =*/5);
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
   * Start continuous measurement.
   */
  if (sensor.setBmv080Mode(CONTINUOUS_MODE) == 0) {
    Serial.println("Continuous measurement started.");
  } else {
    Serial.print("Start failed, last error: ");
    Serial.println(sensor.getLastError());
  }

  /**
   * Configure hardware interrupt on the BMV080 INT pin.
   * The INT pin is triggered when new measurement data is ready.
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
