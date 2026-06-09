/**
 * @file  continuousInterrupt.ino
 * @brief  Read PM data from BMV080 Gravity firmware in continuous mode with external interrupt.
 * @n  The BMV080's INT pin is brought out on the module. Connect it to the host MCU's interrupt pin.
 * @n  When new PM data is ready, the sensor toggles the INT pin. The ISR sets a flag,
 * @n  and the main loop reads data from the ESP32 firmware via I2C/UART.
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
   */
  if (sensor.setMeasureMode(DFRobot_BMV080_Gravity::eContinuousMode) == 0) {
    Serial.println("Continuous measurement started.");
  } else {
    Serial.println("Start measurement failed.");
  }

  /**
   * Configure hardware interrupt on the BMV080 INT pin.
   * The INT pin can report sensor service events, not only PM data readiness.
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
