# -*- coding: utf-8 -*-
'''!
@file  set_baud.py
@brief  Save UART baud/parity/stop-bit configuration in module NVS.
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


def parity_to_text(parity):
  '''!
  @brief Convert parity field value to readable text
  '''
  mapping = {
    DFRobot_BMV080_Gravity.PARITY_NONE: "No parity",
    DFRobot_BMV080_Gravity.PARITY_EVEN: "Even parity",
    DFRobot_BMV080_Gravity.PARITY_ODD: "Odd parity",
  }
  return mapping.get(parity, "Unknown")


def stop_bit_to_text(stop_bit):
  '''!
  @brief Convert stop-bit field value to readable text
  '''
  mapping = {
    DFRobot_BMV080_Gravity.STOP_BIT_1: "1 stop bit",
    DFRobot_BMV080_Gravity.STOP_BIT_1_5: "1.5 stop bits",
    DFRobot_BMV080_Gravity.STOP_BIT_2: "2 stop bits",
  }
  return mapping.get(stop_bit, "Unknown")


def setup():
  '''!
  @brief Save UART baud-rate and frame-format settings to module NVS
  '''
  while not sensor.begin():
    print("Sensor init failed.")
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.set_baud(sensor.BAUD_115200) == 0:
    print("Baud register saved as 115200.")
  else:
    print("Set baud failed.")

  if sensor.set_uart_format(sensor.PARITY_NONE, sensor.STOP_BIT_1) == 0:
    print("UART format saved (8-N-1).")
  else:
    print("Set UART format failed.")

  print("Baud: %d bps" % sensor.get_baud())

  # get_uart_format() returns a combined parity/stop-bit register:
  #   high byte: parity field
  #     0 = no parity, 1 = even parity, 2 = odd parity
  #   low byte: stop-bit field
  #     1 = 1 stop bit, 2 = 1.5 stop bits, 3 = 2 stop bits
  fmt_reg = sensor.get_uart_format()
  parity = (fmt_reg >> 8) & 0xFF
  stop = fmt_reg & 0xFF
  print("UART Format Register: 0x%04X" % fmt_reg)
  print("Parity Field: %d (%s)" % (parity, parity_to_text(parity)))
  print("Stop Bit Field: %d (%s)" % (stop, stop_bit_to_text(stop)))
  print("Restart module in UART mode to apply new UART settings.")


if __name__ == "__main__":
  setup()
