# -*- coding: utf-8 -*-
'''!
@file  duty_cycle_read.py
@brief  Configure parameters and read PM data in duty-cycle mode.
@details  Duty-cycle measurement starts with FAST_RESPONSE as required by the BMV080 SDK.
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
  DUTY_CYCLE_MODE,
  DFRobot_BMV080_Gravity_I2C,
  FAST_RESPONSE,
)

I2C_BUS = 1
ADDR = 0x57

DUTY_CYCLE_PERIOD = 30
INTEGRATION_TIME = 10.0

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def setup():
  '''!
    @brief Configure duty-cycle parameters and start duty-cycle measurement
  '''
  while not sensor.begin():
    print("Sensor init failed.")
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.setDutyCyclingPeriod(DUTY_CYCLE_PERIOD) != 0:
    print("Set duty-cycle period failed.")
  if sensor.setIntegrationTime(INTEGRATION_TIME) != 0:
    print("Set integration time failed.")

  sensor.setMeasurementAlgorithm(FAST_RESPONSE)
  print("Algorithm:", sensor.getMeasurementAlgorithm())

  sensor.setObstructionDetection(True)
  print("Obstruction Detection:", sensor.getObstructionDetection())

  sensor.setVibrationFiltering(True)
  print("Vibration Filtering:", sensor.getVibrationFiltering())

  if sensor.setMeasureMode(DUTY_CYCLE_MODE) == 0:
    print("Duty-cycle measurement started.")
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
