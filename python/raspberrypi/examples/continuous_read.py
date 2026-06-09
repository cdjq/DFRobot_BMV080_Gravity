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
from DFRobot_BMV080_Gravity import CONTINUOUS_MODE, DFRobot_BMV080_Gravity_I2C

I2C_BUS = 1
ADDR = 0x57

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def setup():
  '''!
    @brief Initialize the module and start continuous measurement
  '''
  while not sensor.begin():
    print("Sensor init failed.")
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.setMeasureMode(CONTINUOUS_MODE) == 0:
    print("Continuous measurement started.")
  else:
    print("Start measurement failed.")


def loop():
  '''!
    @brief Read and print PM data when a new sample is available
  '''
  data = sensor.getData()
  if data is not None:
    msg = "PM1.0: %.1f ug/m3  PM2.5: %.1f ug/m3  PM10: %.1f ug/m3" % (data.PM1, data.PM2_5, data.PM10)
    if data.isObstructed:
      msg += "  Obstructed"
    if data.isOutsideMeasurementRange:
      msg += "  OutsideRange"
    print(msg)
  time.sleep(0.1)


if __name__ == "__main__":
  setup()
  while True:
    loop()
