# -*- coding: utf-8 -*-
"""!
@file  duty_cycle_read.py
@brief  Configure parameters and read PM data in duty-cycle mode.
"""

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_BMV080_Gravity import (
  BALANCED,
  DUTY_CYCLE_MODE,
  DFRobot_BMV080_Gravity_I2C,
)

I2C_BUS = 1
ADDR = 0x57

DUTY_CYCLE_PERIOD = 30
INTEGRATION_TIME = 10.0

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def setup():
  while not sensor.begin():
    print("Sensor init failed, last error:", sensor.getLastError())
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.setDutyCyclingPeriod(DUTY_CYCLE_PERIOD) != 0:
    print("Set duty-cycle period failed, error:", sensor.getLastError())
  if sensor.setIntegrationTime(INTEGRATION_TIME) != 0:
    print("Set integration time failed, error:", sensor.getLastError())

  print("Duty Period: %d s  Integration Time: %.1f s" % (sensor.getDutyCyclingPeriod(), sensor.getIntegrationTime()))

  sensor.setMeasurementAlgorithm(BALANCED)
  print("Algorithm:", sensor.getMeasurementAlgorithm())

  sensor.setObstructionDetection(True)
  print("Obstruction Detection:", sensor.getObstructionDetection())

  sensor.setDoVibrationFiltering(True)
  print("Vibration Filtering:", sensor.getDoVibrationFiltering())

  if sensor.setBmv080Mode(DUTY_CYCLE_MODE) == 0:
    print("Duty-cycle measurement started.")
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
