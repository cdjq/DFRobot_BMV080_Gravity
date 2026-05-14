/**
 * @file  readModuleInfo.ino
 * @brief  Read module identity/version/state information from BMV080 Gravity firmware.
 * @n  This demo focuses on information APIs:
 * @n  getPID, getVID, getVersion, getRegMapVersion,
 * @n  getRunState, getStatus, getBmv080DV, getBmv080ID.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-05-13
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */
#include "DFRobot_BMV080_Gravity.h"

/* >> 1. Please choose your communication method below:
 * I2C mode: set module GPIO4 HIGH before ESP32 firmware boots.
 * UART mode: set module GPIO4 LOW before ESP32 firmware boots.
 */
// #define BMV080_COMM_UART
#define BMV080_COMM_I2C

/**
 * The external address is selected by A0/A1 and shared by I2C slave address and Modbus ID.
 * --------------------------------------
 * |    A0     |    A1     |  Address   |
 * --------------------------------------
 * |     0     |     0     |   0x54     |
 * |     0     |     1     |   0x55     |
 * |     1     |     0     |   0x56     |
 * |     1     |     1     |   0x57     |
 * --------------------------------------
 */
const uint8_t ADDR = 0x57;

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

static const char *runStateToStr(uint16_t state)
{
  switch (state) {
    case DFRobot_BMV080_Gravity::eRunStateBoot:
      return "BOOT";
    case DFRobot_BMV080_Gravity::eRunStateReady:
      return "READY";
    case DFRobot_BMV080_Gravity::eRunStateMeasuringContinuous:
      return "MEASURING_CONTINUOUS";
    case DFRobot_BMV080_Gravity::eRunStateMeasuringDuty:
      return "MEASURING_DUTY";
    case DFRobot_BMV080_Gravity::eRunStateStopped:
      return "STOPPED";
    case DFRobot_BMV080_Gravity::eRunStateError:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void setup()
{
  uint16_t major        = 0;
  uint16_t minor        = 0;
  uint16_t patch        = 0;
  char     sensorId[13] = { 0 };

  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  while (!sensor.begin()) {
    Serial.print("Sensor init failed, last error: ");
    Serial.println(sensor.getLastError());
    delay(1000);
  }

  Serial.println("========== BMV080 Gravity Module Info ==========");

  Serial.print("PID: 0x");
  Serial.println(sensor.getPID(), HEX);
  Serial.println("Expected PID: 0x0296 (SEN0662)");

  Serial.print("VID: 0x");
  Serial.println(sensor.getVID(), HEX);
  Serial.println("Expected VID: 0x3343 (DFRobot)");

  Serial.print("Firmware Version: 0x");
  Serial.println(sensor.getVersion(), HEX);

  Serial.print("Register Map Version: 0x");
  Serial.println(sensor.getRegMapVersion(), HEX);

  Serial.print("Run State: ");
  Serial.print(sensor.getRunState());
  Serial.print(" (");
  Serial.print(runStateToStr(sensor.getRunState()));
  Serial.println(")");

  Serial.print("Last SDK Status: ");
  Serial.println(sensor.getStatus());

  if (sensor.getBmv080DV(major, minor, patch)) {
    Serial.print("BMV080 Driver Version: ");
    Serial.print(major);
    Serial.print(".");
    Serial.print(minor);
    Serial.print(".");
    Serial.println(patch);
  } else {
    Serial.print("Read driver version failed, last error: ");
    Serial.println(sensor.getLastError());
  }

  if (sensor.getBmv080ID(sensorId)) {
    Serial.print("Sensor ID: ");
    Serial.println(sensorId);
  } else {
    Serial.print("Read sensor ID failed, last error: ");
    Serial.println(sensor.getLastError());
  }

  Serial.println("================================================");
}

void loop()
{
  /**
   * Poll runtime state/status every 2 seconds so users can observe transitions
   * (for example after external start/stop/reset commands).
   */
  Serial.print("Run State: ");
  Serial.print(sensor.getRunState());
  Serial.print(" (");
  Serial.print(runStateToStr(sensor.getRunState()));
  Serial.print("), Last SDK Status: ");
  Serial.println(sensor.getStatus());
  delay(2000);
}
