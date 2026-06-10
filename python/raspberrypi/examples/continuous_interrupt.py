# -*- coding: utf-8 -*-
'''!
@file  continuous_interrupt.py
@brief  Read PM data in continuous mode with external interrupt.
@details  Connect the BMV080 Gravity INT pin to the Raspberry Pi GPIO configured by irq_pin.
@n        The example uses BCM GPIO numbering and watches a falling edge.
@n        The callback only sets a flag; the main loop still calls get_data() to confirm new data.
@n        Do not read I2C/UART or print from the callback.
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

_gpio_import_error = None
try:
  import RPi.GPIO as GPIO  # type: ignore
except ImportError as exc:
  GPIO = None  # type: ignore
  _gpio_import_error = exc

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

uart_port = "/dev/ttyAMA0"
uart_addr = DFRobot_BMV080_Gravity.DEFAULT_RTU_ADDR
uart_baud = 9600
uart_bits = 8
uart_parity = "N"
uart_stopbit = 1

# Raspberry Pi BCM GPIO number. GPIO17 is physical pin 11.
# Change this if GPIO17 is already used by another board function.
irq_pin = 17

data_flag = False


def create_sensor():
  '''!
  @brief Create the sensor object according to communication_mode
  '''
  mode = communication_mode.upper()
  if mode == "I2C":
    return DFRobot_BMV080_Gravity_I2C(i2c_bus, i2c_addr)
  if mode == "UART":
    return DFRobot_BMV080_Gravity_UART(uart_baud, uart_addr, uart_bits, uart_parity, uart_stopbit, uart_port)
  raise ValueError("communication_mode must be 'I2C' or 'UART'.")


sensor = create_sensor()


def on_interrupt(channel):
  '''!
  @brief GPIO interrupt callback
  '''
  del channel
  global data_flag
  data_flag = True


def print_data(data):
  '''!
  @brief Print PM data and status flags
  @details data fields include pm1/pm2_5/pm10 mass concentration (ug/m3),
           runtime, run_state/status, warning flags, measuring/params_verified,
           and sample_seq.
  '''
  timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
  msg = "[%s] PM1.0: %.1f ug/m3  PM2.5: %.1f ug/m3  PM10: %.1f ug/m3" % (timestamp, data.pm1, data.pm2_5, data.pm10)
  if data.is_obstructed:
    msg += "  Obstructed"
  if data.is_outside_measurement_range:
    msg += "  OutsideRange"
  print(msg)


def setup():
  '''!
  @brief Initialize the module, start continuous measurement and enable GPIO interrupt
  '''
  if GPIO is None:
    raise ImportError("RPi.GPIO is required for interrupt examples. Install with `sudo apt install python3-rpi.gpio`.") from _gpio_import_error

  while not sensor.begin():
    print("Sensor init failed.")
    time.sleep(1)
  print("BMV080 Gravity init succeeded.")

  if sensor.set_measure_mode(sensor.CONTINUOUS_MODE) == 0:
    print("Continuous measurement started.")
  else:
    print("Start measurement failed.")

  GPIO.setmode(GPIO.BCM)
  GPIO.setup(irq_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
  # FALLING matches the module firmware's INT event pulse used by this demo.
  # The callback only sets data_flag; the main loop performs the sensor read.
  GPIO.add_event_detect(irq_pin, GPIO.FALLING, callback=on_interrupt, bouncetime=5)
  print("Interrupt enabled on BCM GPIO %d." % irq_pin)


def loop():
  '''!
  @brief Read and print PM data when the INT pin reports an event
  '''
  global data_flag
  if data_flag:
    data_flag = False
    # get_data() returns None if this INT event is not a new PM sample yet.
    data = sensor.get_data()
    if data is not None:
      print_data(data)
  time.sleep(0.01)


def cleanup():
  '''!
  @brief Release GPIO and communication resources
  '''
  if GPIO is not None:
    GPIO.cleanup()
  if hasattr(sensor, "close"):
    sensor.close()


if __name__ == "__main__":
  try:
    setup()
    while True:
      loop()
  except KeyboardInterrupt:
    pass
  finally:
    cleanup()
