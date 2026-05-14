# -*- coding: utf-8 -*-
"""!
@file  set_baud_uart_format.py
@brief  Save UART baud/parity/stop-bit configuration in module NVS.
"""

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_BMV080_Gravity import DFRobot_BMV080_Gravity, DFRobot_BMV080_Gravity_I2C

I2C_BUS = 1
ADDR = 0x57

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def setup():
  while not sensor.begin():
    print("Sensor init failed, last error:", sensor.getLastError())
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.setBaud(DFRobot_BMV080_Gravity.e115200) == 0:
    print("Baud register saved as 115200.")
  else:
    print("Set baud failed, last error:", sensor.getLastError())

  if sensor.setUartFormat(DFRobot_BMV080_Gravity.eParityNone, DFRobot_BMV080_Gravity.eStopBit1) == 0:
    print("UART format saved (8-N-1).")
  else:
    print("Set UART format failed, last error:", sensor.getLastError())

  print("Baud Register: 0x%04X" % sensor.getBaud())
  print("Baud Value: %d bps" % sensor.getBaudValue())

  fmt_reg = sensor.getUartFormat()
  parity = (fmt_reg >> 8) & 0xFF
  stop = fmt_reg & 0xFF
  print("UART Format Register: 0x%04X" % fmt_reg)
  print("Parity Field:", parity)
  print("Stop Bit Field:", stop)
  print("Restart module in UART mode to apply new UART settings.")


if __name__ == "__main__":
  setup()
