# -*- coding: utf-8 -*-
"""!
  @file  read_module_info.py
  @brief  Read module ID, firmware version, run state and BMV080 info.
"""

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_BMV080_Gravity import DFRobot_BMV080_Gravity, DFRobot_BMV080_Gravity_I2C

I2C_BUS = 1
ADDR = 0x57

sensor = DFRobot_BMV080_Gravity_I2C(I2C_BUS, ADDR)


def run_state_to_str(state):
    mapping = {
        DFRobot_BMV080_Gravity.eRunStateBoot: "BOOT",
        DFRobot_BMV080_Gravity.eRunStateReady: "READY",
        DFRobot_BMV080_Gravity.eRunStateMeasuringContinuous: "MEASURING_CONTINUOUS",
        DFRobot_BMV080_Gravity.eRunStateMeasuringDuty: "MEASURING_DUTY",
        DFRobot_BMV080_Gravity.eRunStateStopped: "STOPPED",
        DFRobot_BMV080_Gravity.eRunStateError: "ERROR",
    }
    return mapping.get(state, "UNKNOWN")


def setup():
    while not sensor.begin():
        print("Sensor init failed, last error:", sensor.getLastError())
        time.sleep(1)

    print("========== BMV080 Gravity Module Info ==========")
    print("PID: 0x%04X" % sensor.getPID())
    print("VID: 0x%04X" % sensor.getVID())
    print("Firmware Version: 0x%04X" % sensor.getVersion())
    print("Register Map Version: 0x%04X" % sensor.getRegMapVersion())

    run_state = sensor.getRunState()
    print("Run State: %d (%s)" % (run_state, run_state_to_str(run_state)))
    print("Last SDK Status:", sensor.getStatus())

    dv = sensor.getBmv080DV()
    if dv is not None:
        print("BMV080 Driver Version: %d.%d.%d" % (dv[0], dv[1], dv[2]))
    else:
        print("Read driver version failed, last error:", sensor.getLastError())

    sensor_id = sensor.getBmv080ID()
    if sensor_id is not None:
        print("Sensor ID:", sensor_id)
    else:
        print("Read sensor ID failed, last error:", sensor.getLastError())
    print("================================================")


def loop():
    run_state = sensor.getRunState()
    print(
        "Run State: %d (%s), Last SDK Status: %d"
        % (run_state, run_state_to_str(run_state), sensor.getStatus())
    )
    time.sleep(2)


if __name__ == "__main__":
    setup()
    while True:
        loop()

