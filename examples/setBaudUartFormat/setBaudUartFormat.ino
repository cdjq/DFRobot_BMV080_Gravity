/**
 * @file  setBaudUartFormat.ino
 * @brief  Save UART baud rate, parity and stop-bit settings to BMV080 Gravity firmware.
 * @n  This demo configures the UART communication parameters of the module firmware.
 * @n  The settings are stored in ESP32 NVS and take effect after the module restarts in UART Modbus RTU mode.
 * @n  This demo works with both I2C and UART communication; the UART settings configure the module's own serial port.
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
  uint16_t fmtReg = 0;
  uint8_t  parity = 0;
  uint8_t  stop   = 0;

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
   * Save UART baud rate (holding register 0x0001).
   * Available baud rates:
   *   e2400, e4800, e9600, e14400, e19200, e38400, e57600, e115200
   */
  if (sensor.setBaud(DFRobot_BMV080_Gravity::e115200) == 0) {
    Serial.println("Baud register saved as 115200.");
  } else {
    Serial.println("Set baud failed.");
  }

  /**
   * Save UART parity and stop-bit (holding register 0x0002).
   * Parity:  eParityNone / eParityEven / eParityOdd
   * StopBit: eStopBit1 / eStopBit1_5 / eStopBit2
   */
  if (sensor.setUartFormat(DFRobot_BMV080_Gravity::eParityNone, DFRobot_BMV080_Gravity::eStopBit1) == 0) {
    Serial.println("UART format saved (8-N-1).");
  } else {
    Serial.println("Set UART format failed.");
  }

  /**
   * Read back registers for verification.
   * Use getBaud to read the actual baud rate in bps.
   * Use getUartFormat to read the combined parity/stop-bit register.
   */
  Serial.print("Baud: ");
  Serial.print(sensor.getBaud());
  Serial.println(" bps");

  fmtReg = sensor.getUartFormat();
  parity = (uint8_t)((fmtReg >> 8) & 0xFF);
  stop   = (uint8_t)(fmtReg & 0xFF);

  Serial.print("UART Format Register: 0x");
  Serial.println(fmtReg, HEX);
  Serial.print("Parity Field: ");
  Serial.println(parity);
  Serial.print("Stop Bit Field: ");
  Serial.println(stop);

  Serial.println();
  Serial.println("Restart the module in UART mode to apply the new UART settings.");
}

void loop() {}
