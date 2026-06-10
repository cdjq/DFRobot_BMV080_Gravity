# -*- coding: utf-8 -*-
'''!
@file  continuous_read.py
@brief  Read PM data in continuous mode from DFRobot BMV080 Gravity firmware.
@copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
@license     The MIT License (MIT)
@author      DFRobot
@version     V1.0.0
@date        2026-06-09
@url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
'''

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_BMV080_Gravity import (
  DFRobot_BMV080_Gravity,
  DFRobot_BMV080_Gravity_I2C,
  DFRobot_BMV080_Gravity_UART,
)

communication_mode = "I2C"  # "I2C" or "UART"

# I2C_ADDR is selected by A0/A1 pins.
# UART_ADDR is the module's current Modbus RTU address. To change the saved UART address,
# use a serial/Modbus tool, then update UART_ADDR here before using UART mode.
# --------------------------------------
# |    A0     |    A1     |  Address   |
# --------------------------------------
# |     0     |     0     |   0x54     |
# |     0     |     1     |   0x55     |
# |     1     |     0     |   0x56     |
# |     1     |     1     |   0x57     |
# --------------------------------------
i2c_bus = 1
i2c_addr = DFRobot_BMV080_Gravity.DEFAULT_I2C_ADDR

uart_addr = DFRobot_BMV080_Gravity.DEFAULT_RTU_ADDR
uart_baud = 9600
uart_bits = 8
uart_parity = "N"
uart_stopbit = 1


def create_sensor():
  '''!
  @brief Create the sensor object according to communication_mode
  '''
  mode = communication_mode.upper()
  if mode == "I2C":
    return DFRobot_BMV080_Gravity_I2C(i2c_bus, i2c_addr)
  if mode == "UART":
    return DFRobot_BMV080_Gravity_UART(uart_baud, uart_addr, uart_bits, uart_parity, uart_stopbit)
  raise ValueError("communication_mode must be 'I2C' or 'UART'.")


sensor = create_sensor()


def setup():
  '''!
  @brief Initialize the module and start continuous measurement
  '''
  while not sensor.begin():
    print("Sensor init failed.")
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  sensor.set_measurement_algorithm(sensor.BALANCED)
  print("Algorithm:", sensor.get_measurement_algorithm())

  sensor.set_obstruction_detection(True)
  print("Obstruction Detection:", sensor.get_obstruction_detection())

  sensor.set_vibration_filtering(True)
  print("Vibration Filtering:", sensor.get_vibration_filtering())

  if sensor.set_measure_mode(sensor.CONTINUOUS_MODE) == 0:
    print("Continuous measurement started.")
  else:
    print("Start measurement failed.")


def loop():
  '''!
  @brief Read and print PM data when a new sample is available
  '''
  # get_data() returns None until the firmware reports a new sample.
  # Returned data fields include:
  #   pm1 / pm2_5 / pm10: PM1.0, PM2.5 and PM10 mass concentration (ug/m3)
  #   runtime: sensor runtime in seconds
  #   run_state / status: firmware run state and status code
  #   is_obstructed / is_outside_measurement_range: warning flags
  #   measuring / params_verified: current measurement state flags
  #   sample_seq: sample sequence number
  data = sensor.get_data()
  if data is not None:
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    msg = "[%s] PM1.0: %.1f ug/m3  PM2.5: %.1f ug/m3  PM10: %.1f ug/m3" % (timestamp, data.pm1, data.pm2_5, data.pm10)
    if data.is_obstructed:
      msg += "  Obstructed"
    if data.is_outside_measurement_range:
      msg += "  OutsideRange"
    print(msg)
  time.sleep(0.1)


if __name__ == "__main__":
  setup()
  while True:
    loop()
