# -*- coding: utf-8 -*-
"""!
@file  consecutive_read.py
@brief  Read PM data in continuous mode from DFRobot BMV080 Gravity firmware.
"""

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_BMV080_Gravity import CONTINUOUS_MODE, DFRobot_BMV080_Gravity_I2C

I2C_BUS = 1
ADDR = 0x57

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def setup():
  while not sensor.begin():
    print("Sensor init failed, last error:", sensor.getLastError())
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.setBmv080Mode(CONTINUOUS_MODE) == 0:
    print("Continuous measurement started.")
  else:
    print("Start failed, last error:", sensor.getLastError())


def loop():
  data = sensor.getBmv080Data()
  if data is not None:
    msg = "PM1.0: %.1f ug/m3  PM2.5: %.1f ug/m3  PM10: %.1f ug/m3  runtime: %.1f s  runState: %d" % (data.PM1, data.PM2_5, data.PM10, data.runtime, data.runState)
    if data.isObstructed:
      msg += "  Obstructed"
    if data.isOutsideMeasurementRange:
      msg += "  OutsideRange"
    if data.paramsVerified:
      msg += "  ParamsVerified"
    print(msg)
  time.sleep(0.1)


if __name__ == "__main__":
  setup()
  while True:
    loop()
