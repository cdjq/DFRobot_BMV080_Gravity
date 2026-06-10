# -*- coding: utf-8 -*-
# pylint: disable=broad-exception-caught
'''!
@file  DFRobot_BMV080_Gravity.py
@brief  Python driver for DFRobot BMV080 Gravity firmware module.
@details  This library talks to the module firmware register map over I2C short frames
@n       (0xA5 header). The ESP32 firmware on the module owns the Bosch BMV080 handle.
@copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
@license     The MIT License (MIT)
@author      DFRobot
@version     V1.0.0
@date        2026-06-09
@url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
'''

from __future__ import annotations

from ctypes import Structure, c_bool, c_float, c_uint16
import math
import struct
import time
from typing import Any, List, Optional

from DFRobot_RTU import DFRobot_RTU

_smbus_import_error: Optional[ImportError] = None

try:
  import smbus2 as smbus  # type: ignore
  from smbus2 import i2c_msg  # type: ignore
except ImportError:
  i2c_msg = None
  try:
    import smbus  # type: ignore
  except ImportError as exc:
    smbus = None
    _smbus_import_error = exc


class DFRobot_BMV080_Gravity(object):
  # Return/Error code
  RET_CODE_OK = 0
  RET_CODE_ERROR = 1
  ERR_OK = 0
  ERR_DATA_BUS = 1
  ERR_DATA_READ = 2
  ERR_IC_VERSION = 3

  # Device constants
  DEFAULT_I2C_ADDR = 0x57
  DEFAULT_RTU_ADDR = 0x57
  _EXPECTED_PID = 0x0296
  _EXPECTED_VID = 0x3343
  _EXPECTED_REG_MAP_VERSION = 0x0004

  # Measurement mode
  CONTINUOUS_MODE = 0
  DUTY_CYCLE_MODE = 1

  # Measurement algorithm
  FAST_RESPONSE = 1
  BALANCED = 2
  HIGH_PRECISION = 3

  # Action register values
  _ACTION_START = 1
  _ACTION_STOP = 2
  _ACTION_RESET = 3

  # UART baud register values
  BAUD_2400 = 0x0001
  BAUD_4800 = 0x0002
  BAUD_9600 = 0x0003
  BAUD_14400 = 0x0004
  BAUD_19200 = 0x0005
  BAUD_38400 = 0x0006
  BAUD_57600 = 0x0007
  BAUD_115200 = 0x0008

  # UART parity
  PARITY_NONE = 0x00
  PARITY_EVEN = 0x01
  PARITY_ODD = 0x02

  # UART stop bits
  STOP_BIT_1 = 0x01
  STOP_BIT_1_5 = 0x02
  STOP_BIT_2 = 0x03

  # Run states (input register 0x0004)
  RUN_STATE_BOOT = 0
  RUN_STATE_READY = 1
  RUN_STATE_MEASURING_CONTINUOUS = 2
  RUN_STATE_MEASURING_DUTY = 3
  RUN_STATE_STOPPED = 4
  RUN_STATE_ERROR = 5

  # Input registers
  _REG_INPUT_PID = 0x0000
  _REG_INPUT_RUN_STATE = 0x0004

  # Holding registers
  _REG_HOLDING_BAUDRATE = 0x0001
  _REG_HOLDING_VERIFY_STOP = 0x0002
  _REG_HOLDING_ACTION = 0x0003
  _REG_HOLDING_MEASURE_MODE = 0x0004
  _REG_HOLDING_ALGORITHM = 0x0005
  _REG_HOLDING_OBSTRUCTION = 0x0006
  _REG_HOLDING_VIBRATION = 0x0007
  _REG_HOLDING_INTEGRATION_F32_HI = 0x0008
  _REG_HOLDING_INTEGRATION_F32_LO = 0x0009
  _REG_HOLDING_DUTY_PERIOD_S = 0x000A

  # Input flag bits
  _INPUT_FLAG_OBSTRUCTED = 1 << 0
  _INPUT_FLAG_OUTSIDE_RANGE = 1 << 1
  _INPUT_FLAG_DATA_READY = 1 << 2
  _INPUT_FLAG_MEASURING = 1 << 4
  _INPUT_FLAG_PARAMS_VERIFIED = 1 << 6
  _INPUT_FLAG_VALUE_CLAMPED = 1 << 8
  _INPUT_FLAG_VALUE_INVALID = 1 << 9

  class BMV080Data(Structure):
    '''!
    @brief Cached PM data and state flags.
    @details get_data() returns this object when a new sample is ready.
             pm1, pm2_5 and pm10 are PM1.0/PM2.5/PM10 mass concentrations in ug/m3.
             runtime is sensor runtime in seconds.
             run_state is the current run state, see RUN_STATE_* constants.
             status is the last BMV080 SDK/firmware status code.
             is_obstructed is True when obstruction is detected.
             is_outside_measurement_range is True when PM value is outside the reliable measurement range.
             data_ready is True when new PM data is available.
             measuring is True while the sensor is measuring.
             params_verified is True after measurement parameters are applied to the sensor.
             value_clamped is a reserved compatibility flag.
             value_invalid is True when firmware sanitized a non-finite PM/runtime value.
             sample_seq increments for each new measurement.
    '''

    _fields_ = [
      ("pm1", c_float),
      ("pm2_5", c_float),
      ("pm10", c_float),
      ("runtime", c_float),
      ("run_state", c_uint16),
      ("status", c_uint16),
      ("is_obstructed", c_bool),
      ("is_outside_measurement_range", c_bool),
      ("data_ready", c_bool),
      ("measuring", c_bool),
      ("params_verified", c_bool),
      ("value_clamped", c_bool),
      ("value_invalid", c_bool),
      ("sample_seq", c_uint16),
    ]

  def __init__(self) -> None:
    self._data = self.BMV080Data()

  def begin(self) -> bool:
    '''!
    @brief Initialize the module and check firmware compatibility
    @return Initialization status
    @retval True Initialization succeeded
    @retval False Initialization failed
    '''
    for _ in range(8):
      regs = self.read_input_reg(self._REG_INPUT_PID, 4)
      if regs is not None:
        pid = regs[0]
        vid = regs[1]
        reg_map = regs[3]
        if vid == self._EXPECTED_VID and pid == self._EXPECTED_PID and reg_map == self._EXPECTED_REG_MAP_VERSION:
          return True
      time.sleep(0.01)

    return False

  def _read_data(self) -> Optional[DFRobot_BMV080_Gravity.BMV080Data]:
    regs = None
    for _ in range(3):
      regs = self.read_input_reg(self._REG_INPUT_RUN_STATE, 12)
      if regs is not None:
        break
      time.sleep(0.003)
    if regs is None:
      return None

    flags = regs[10]
    data = self.BMV080Data()
    data.run_state = regs[0]
    data.status = regs[1]
    data.pm1 = self._regs_to_float(regs[2], regs[3])
    data.pm2_5 = self._regs_to_float(regs[4], regs[5])
    data.pm10 = self._regs_to_float(regs[6], regs[7])
    data.runtime = self._regs_to_float(regs[8], regs[9])
    data.is_obstructed = (flags & self._INPUT_FLAG_OBSTRUCTED) != 0
    data.is_outside_measurement_range = (flags & self._INPUT_FLAG_OUTSIDE_RANGE) != 0
    data.data_ready = (flags & self._INPUT_FLAG_DATA_READY) != 0
    data.measuring = (flags & self._INPUT_FLAG_MEASURING) != 0
    data.params_verified = (flags & self._INPUT_FLAG_PARAMS_VERIFIED) != 0
    data.value_clamped = (flags & self._INPUT_FLAG_VALUE_CLAMPED) != 0
    data.value_invalid = (flags & self._INPUT_FLAG_VALUE_INVALID) != 0
    data.sample_seq = regs[11]
    self._data = data
    return self._copy_data(data)

  def get_data(self) -> Optional[DFRobot_BMV080_Gravity.BMV080Data]:
    '''!
    @brief Read particulate matter measurement data
    @details Returned data fields:
             pm1, pm2_5, pm10: PM1.0/PM2.5/PM10 mass concentration in ug/m3.
             runtime: Sensor runtime in seconds.
             run_state: Current run state, see RUN_STATE_* constants.
             status: Last BMV080 SDK/firmware status code.
             is_obstructed: Obstruction detected.
             is_outside_measurement_range: PM value is outside the reliable measurement range.
             data_ready: New PM data is available.
             measuring: Sensor is currently measuring.
             params_verified: Measurement parameters have been applied to the sensor.
             value_clamped: Reserved compatibility flag.
             value_invalid: Non-finite PM/runtime value was sanitized by firmware.
             sample_seq: Sample sequence number, increments for each new measurement.
    @return BMV080Data object when new data is available, otherwise None
    '''
    data = self._read_data()
    if data is None or not data.data_ready:
      return None
    return data

  @staticmethod
  def _copy_data(data: DFRobot_BMV080_Gravity.BMV080Data) -> DFRobot_BMV080_Gravity.BMV080Data:
    copied = DFRobot_BMV080_Gravity.BMV080Data()
    for field in DFRobot_BMV080_Gravity.BMV080Data._fields_:
      field_name = field[0]
      setattr(copied, field_name, getattr(data, field_name))
    return copied

  def set_measure_mode(self, mode: int) -> int:
    '''!
    @brief Set measurement mode and start measurement
    @param mode Measurement mode
    @n          CONTINUOUS_MODE: Continuous measurement mode
    @n          DUTY_CYCLE_MODE: Duty-cycle measurement mode
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error or start-state timeout
    @note When duty-cycle measurement is started, the firmware forces FAST_RESPONSE as required by the BMV080 SDK.
    '''
    if mode not in (self.CONTINUOUS_MODE, self.DUTY_CYCLE_MODE):
      return -1

    ret = self._write_holding_value(self._REG_HOLDING_MEASURE_MODE, mode)
    if ret != self.RET_CODE_OK:
      return int(ret)
    ret = self._write_holding_value(self._REG_HOLDING_ACTION, self._ACTION_START)
    if ret != self.RET_CODE_OK:
      return int(ret)

    target_state = self.RUN_STATE_MEASURING_DUTY if mode == self.DUTY_CYCLE_MODE else self.RUN_STATE_MEASURING_CONTINUOUS
    start = time.monotonic()
    while (time.monotonic() - start) < 1.0:
      regs = self.read_input_reg(self._REG_INPUT_RUN_STATE, 2)
      if regs is not None:
        if regs[0] == target_state:
          return self.RET_CODE_OK
        if regs[0] == self.RUN_STATE_ERROR:
          return int(regs[1]) if regs[1] != self.RET_CODE_OK else self.RET_CODE_ERROR
      time.sleep(0.01)

    return self.ERR_DATA_READ

  def stop_measurement(self) -> bool:
    '''!
    @brief Stop the current measurement
    @return Stop command execution status
    @retval True Stop command succeeded
    @retval False Stop command failed
    '''
    return self._write_holding_value(self._REG_HOLDING_ACTION, self._ACTION_STOP) == self.RET_CODE_OK

  def reset(self) -> bool:
    '''!
    @brief Reset the sensor and restore default configuration
    @return Reset command execution status
    @retval True Reset command succeeded
    @retval False Reset command failed
    '''
    return self._write_holding_value(self._REG_HOLDING_ACTION, self._ACTION_RESET) == self.RET_CODE_OK

  def set_integration_time(self, integration_time: float) -> int:
    '''!
    @brief Set measurement integration time
    @param integration_time Integration time in seconds
    @n                      The value must be greater than 0 and must not be NAN or INF.
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter or duty-cycle period constraint was not met
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When increasing integration time beyond the current period margin, set duty-cycle period first.
    '''
    duty_period = self._get_duty_cycling_period()
    if not math.isfinite(integration_time) or integration_time <= 0.0:
      return -1
    if duty_period == 0 or float(duty_period) < (integration_time + 2.0):
      return -1
    regs = self._float_to_regs(integration_time)
    ret = self.write_holding_reg(self._REG_HOLDING_INTEGRATION_F32_HI, regs)
    return int(ret)

  def _get_integration_time(self) -> float:
    for _ in range(3):
      values = self.read_holding_reg(self._REG_HOLDING_INTEGRATION_F32_HI, 2)
      if values is not None:
        value = self._regs_to_float(values[0], values[1])
        if math.isfinite(value):
          return value
        break
      time.sleep(0.005)
    high_word = self._read_holding_value(self._REG_HOLDING_INTEGRATION_F32_HI)
    low_word = self._read_holding_value(self._REG_HOLDING_INTEGRATION_F32_LO)
    if high_word is not None and low_word is not None:
      return self._regs_to_float(high_word, low_word)
    return math.nan

  def set_duty_cycling_period(self, duty_cycling_period: int) -> int:
    '''!
    @brief Set duty-cycle measurement period
    @param duty_cycling_period Duty-cycle measurement period in seconds
    @n                         The value must satisfy current integration time plus 2 seconds.
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter or integration time read failed
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When shortening duty-cycle period, lower integration time first if needed.
    '''
    integration_time = self._get_integration_time()
    if duty_cycling_period <= 0 or not math.isfinite(integration_time) or float(duty_cycling_period) < (integration_time + 2.0):
      return -1
    return int(self._write_holding_value(self._REG_HOLDING_DUTY_PERIOD_S, duty_cycling_period))

  def get_integration_time(self) -> float:
    '''!
    @brief Read measurement integration time
    @return Integration time in seconds
    @retval math.nan Read failed
    '''
    return self._get_integration_time()

  def _get_duty_cycling_period(self) -> int:
    value = self._read_holding_value(self._REG_HOLDING_DUTY_PERIOD_S)
    return value if value is not None else 0

  def get_duty_cycling_period(self) -> int:
    '''!
    @brief Read duty-cycle measurement period
    @return Duty-cycle measurement period in seconds
    @retval 0 Read failed
    '''
    return self._get_duty_cycling_period()

  def set_obstruction_detection(self, enable: bool) -> bool:
    '''!
    @brief Enable or disable obstruction detection
    @param enable Obstruction detection switch
    @n            True: Enable obstruction detection
    @n            False: Disable obstruction detection
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
    '''
    return self._write_holding_value(self._REG_HOLDING_OBSTRUCTION, 1 if enable else 0) == self.RET_CODE_OK

  def get_obstruction_detection(self) -> int:
    '''!
    @brief Read obstruction detection switch state
    @return Obstruction detection switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
    '''
    value = self._read_holding_value(self._REG_HOLDING_OBSTRUCTION)
    if value is None:
      return -1
    return 1 if value != 0 else 0

  def set_vibration_filtering(self, enable: bool) -> bool:
    '''!
    @brief Enable or disable vibration filtering
    @param enable Vibration filtering switch
    @n            True: Enable vibration filtering
    @n            False: Disable vibration filtering
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
    '''
    return self._write_holding_value(self._REG_HOLDING_VIBRATION, 1 if enable else 0) == self.RET_CODE_OK

  def get_vibration_filtering(self) -> int:
    '''!
    @brief Read vibration filtering switch state
    @return Vibration filtering switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
    '''
    value = self._read_holding_value(self._REG_HOLDING_VIBRATION)
    if value is None:
      return -1
    return 1 if value != 0 else 0

  def set_measurement_algorithm(self, measurement_algorithm: int) -> int:
    '''!
    @brief Set measurement algorithm
    @param measurement_algorithm Measurement algorithm
    @n                           FAST_RESPONSE: Fast response algorithm
    @n                           BALANCED: Balanced algorithm
    @n                           HIGH_PRECISION: High precision algorithm
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When duty-cycle measurement is started, the firmware forces FAST_RESPONSE as required by the BMV080 SDK.
    '''
    if measurement_algorithm < self.FAST_RESPONSE or measurement_algorithm > self.HIGH_PRECISION:
      return -1
    return int(self._write_holding_value(self._REG_HOLDING_ALGORITHM, measurement_algorithm))

  def get_measurement_algorithm(self) -> int:
    '''!
    @brief Read measurement algorithm
    @return Current measurement algorithm
    @retval FAST_RESPONSE Fast response algorithm
    @retval BALANCED Balanced algorithm
    @retval HIGH_PRECISION High precision algorithm
    @retval 0 Read failed or register value is invalid
    '''
    value = self._read_holding_value(self._REG_HOLDING_ALGORITHM)
    if value is None:
      return 0
    if value < self.FAST_RESPONSE or value > self.HIGH_PRECISION:
      return 0
    return int(value)

  def set_baud(self, baud: int) -> int:
    '''!
    @brief Save UART baud-rate setting to firmware NVS
    @param baud Baud-rate enum value
    @n          BAUD_2400, BAUD_4800, BAUD_9600, BAUD_14400, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new baud rate takes effect after module restart.
    '''
    if baud < self.BAUD_2400 or baud > self.BAUD_115200:
      return self.RET_CODE_ERROR
    return int(self._write_holding_value(self._REG_HOLDING_BAUDRATE, baud))

  def get_baud(self) -> int:
    '''!
    @brief Read UART baud rate
    @return Current baud rate in bps
    @retval 0 Read failed
    @note Invalid register values are parsed as the default 9600 bps.
    '''
    value = self._read_holding_value(self._REG_HOLDING_BAUDRATE)
    return self._baud_reg_to_value(value) if value is not None else 0

  def set_uart_format(self, parity: int, stop_bit: int = STOP_BIT_1) -> int:
    '''!
    @brief Save UART parity and stop-bit setting to firmware NVS
    @param parity Parity configuration
    @n            PARITY_NONE: No parity
    @n            PARITY_EVEN: Even parity
    @n            PARITY_ODD: Odd parity
    @param stop_bit Stop-bit configuration
    @n              STOP_BIT_1: 1 stop bit
    @n              STOP_BIT_1_5: 1.5 stop bits
    @n              STOP_BIT_2: 2 stop bits
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new UART frame format takes effect after module restart.
    '''
    if parity > self.PARITY_ODD or stop_bit < self.STOP_BIT_1 or stop_bit > self.STOP_BIT_2:
      return self.RET_CODE_ERROR

    value = ((parity & 0xFF) << 8) | (stop_bit & 0xFF)
    return int(self._write_holding_value(self._REG_HOLDING_VERIFY_STOP, value))

  def get_uart_format(self) -> int:
    '''!
    @brief Read UART parity and stop-bit register value
    @return UART frame-format register value
    @retval 0 Read failed
    @note The high byte is parity and the low byte is stop bits.
    '''
    value = self._read_holding_value(self._REG_HOLDING_VERIFY_STOP)
    return value if value is not None else 0

  def _baud_reg_to_value(self, baud_reg: int) -> int:
    mapping = {
      self.BAUD_2400: 2400,
      self.BAUD_4800: 4800,
      self.BAUD_9600: 9600,
      self.BAUD_14400: 14400,
      self.BAUD_19200: 19200,
      self.BAUD_38400: 38400,
      self.BAUD_57600: 57600,
      self.BAUD_115200: 115200,
    }
    return mapping.get(baud_reg, 9600)

  # Virtual transport methods
  def write_holding_reg(self, reg: int, data: List[int]) -> int:
    raise NotImplementedError

  def read_holding_reg(self, reg: int, count: int) -> Optional[List[int]]:
    raise NotImplementedError

  def read_input_reg(self, reg: int, count: int) -> Optional[List[int]]:
    raise NotImplementedError

  def _read_holding_value(self, reg: int) -> Optional[int]:
    for _ in range(3):
      data = self.read_holding_reg(reg, 1)
      if data is not None:
        return data[0]
      time.sleep(0.005)
    return None

  def _write_holding_value(self, reg: int, value: int) -> int:
    return self.write_holding_reg(reg, [value & 0xFFFF])

  @staticmethod
  def _float_to_regs(value: float) -> List[int]:
    raw = struct.unpack(">I", struct.pack(">f", float(value)))[0]
    return [(raw >> 16) & 0xFFFF, raw & 0xFFFF]

  @staticmethod
  def _regs_to_float(high_word: int, low_word: int) -> float:
    raw = ((int(high_word) & 0xFFFF) << 16) | (int(low_word) & 0xFFFF)
    return struct.unpack(">f", struct.pack(">I", raw))[0]


class DFRobot_BMV080_Gravity_I2C(DFRobot_BMV080_Gravity):
  BMV080_I2C_SHORT_HEADER = 0xA5
  BMV080_I2C_FUNC_READ_HOLDING = 0x03
  BMV080_I2C_FUNC_READ_INPUT = 0x04
  BMV080_I2C_FUNC_WRITE_HOLDING = 0x06
  BMV080_I2C_FUNC_WRITE_MULTI_HOLDING = 0x10
  BMV080_I2C_MAX_FRAME_LEN = 32
  BMV080_I2C_MAX_READ_REGS = 14
  BMV080_I2C_MAX_WRITE_REGS = 13
  BMV080_I2C_SHORTFRAME_RETRY = 8
  BMV080_I2C_SLAVE_SETTLE_MS = 30
  BMV080_I2C_VALIDATE_RETRY = 3

  def __init__(self, bus: int = 1, addr: int = DFRobot_BMV080_Gravity.DEFAULT_I2C_ADDR) -> None:
    super().__init__()
    self._bus_id = bus
    self._addr = addr
    self._timeout_s = 1.0
    self._bus: Optional[Any] = None

  def begin(self) -> bool:
    '''!
    @brief Initialize I2C communication and check firmware compatibility
    @return Initialization status
    @retval True Initialization succeeded
    @retval False Initialization failed
    '''
    if not self._ensure_bus():
      return False
    return super().begin()

  def close(self) -> None:
    '''!
    @brief Close the I2C bus handle
    '''
    if self._bus is not None:
      try:
        self._bus.close()
      except Exception:
        pass
      self._bus = None

  def set_timeout_time_ms(self, timeout_ms: int) -> None:
    '''!
    @brief Set I2C communication timeout
    @param timeout_ms Timeout in milliseconds
    '''
    if timeout_ms < 1:
      timeout_ms = 1
    self._timeout_s = timeout_ms / 1000.0

  def write_holding_reg(self, reg: int, data: List[int]) -> int:
    if data is None or len(data) == 0:
      return self.RET_CODE_ERROR
    if len(data) == 1:
      return self._write_single_reg(reg, data[0])
    return self._write_multi_regs(reg, data)

  def read_holding_reg(self, reg: int, count: int) -> Optional[List[int]]:
    return self._read_regs(self.BMV080_I2C_FUNC_READ_HOLDING, reg, count)

  def read_input_reg(self, reg: int, count: int) -> Optional[List[int]]:
    return self._read_regs(self.BMV080_I2C_FUNC_READ_INPUT, reg, count)

  def _ensure_bus(self) -> bool:
    if self._bus is not None:
      return True
    if smbus is None:
      raise ImportError("smbus2 or smbus is required. Install with `pip install smbus2` or `sudo apt install python3-smbus`.") from _smbus_import_error
    try:
      self._bus = smbus.SMBus(self._bus_id)
    except Exception:
      return False
    return True

  def _read_regs(self, func: int, reg: int, count: int) -> Optional[List[int]]:
    if count <= 0 or count > self.BMV080_I2C_MAX_READ_REGS:
      return None

    request = [
      self.BMV080_I2C_SHORT_HEADER,
      func & 0xFF,
      (reg >> 8) & 0xFF,
      reg & 0xFF,
      count & 0xFF,
    ]
    response_len = 3 + (count * 2)

    for _ in range(self.BMV080_I2C_VALIDATE_RETRY):
      response = self._transfer_short_frame(request, response_len)
      if response is None:
        continue

      if response[0] != self.BMV080_I2C_SHORT_HEADER:
        time.sleep(0.003)
        continue

      if response[1] == (func | 0x80):
        return None

      if response[1] != func or response[2] != (count * 2) or len(response) != response_len:
        time.sleep(0.003)
        continue

      regs = []
      for index in range(count):
        high_word = response[3 + (index * 2)]
        low_word = response[4 + (index * 2)]
        regs.append(((high_word << 8) | low_word) & 0xFFFF)
      return regs

    return None

  def _write_single_reg(self, reg: int, value: int) -> int:
    request = [
      self.BMV080_I2C_SHORT_HEADER,
      self.BMV080_I2C_FUNC_WRITE_HOLDING,
      (reg >> 8) & 0xFF,
      reg & 0xFF,
      (value >> 8) & 0xFF,
      value & 0xFF,
    ]
    ret = self.ERR_DATA_READ

    for _ in range(self.BMV080_I2C_VALIDATE_RETRY):
      response = self._transfer_short_frame(request, 6)
      if response is None:
        continue

      if response[0] != self.BMV080_I2C_SHORT_HEADER:
        ret = self.ERR_DATA_READ
        time.sleep(0.003)
        continue

      if response[1] == (self.BMV080_I2C_FUNC_WRITE_HOLDING | 0x80):
        return int(response[2])

      if response != request:
        ret = self.ERR_DATA_READ
        time.sleep(0.003)
        continue

      return self.RET_CODE_OK

    return ret

  def _write_multi_regs(self, reg: int, data: List[int]) -> int:
    count = len(data)
    if count == 0 or count > self.BMV080_I2C_MAX_WRITE_REGS:
      return self.RET_CODE_ERROR

    request = [
      self.BMV080_I2C_SHORT_HEADER,
      self.BMV080_I2C_FUNC_WRITE_MULTI_HOLDING,
      (reg >> 8) & 0xFF,
      reg & 0xFF,
      count & 0xFF,
      (count * 2) & 0xFF,
    ]
    for value in data:
      request.append((value >> 8) & 0xFF)
      request.append(value & 0xFF)
    ret = self.ERR_DATA_READ

    for _ in range(self.BMV080_I2C_VALIDATE_RETRY):
      response = self._transfer_short_frame(request, 6)
      if response is None:
        continue

      if response[0] != self.BMV080_I2C_SHORT_HEADER:
        ret = self.ERR_DATA_READ
        time.sleep(0.003)
        continue

      if response[1] == (self.BMV080_I2C_FUNC_WRITE_MULTI_HOLDING | 0x80):
        return int(response[2])

      resp_reg = ((response[2] << 8) | response[3]) & 0xFFFF
      resp_count = ((response[4] << 8) | response[5]) & 0xFFFF
      if response[1] != self.BMV080_I2C_FUNC_WRITE_MULTI_HOLDING or resp_reg != reg or resp_count != count:
        ret = self.ERR_DATA_READ
        time.sleep(0.003)
        continue

      return self.RET_CODE_OK

    return ret

  def _transfer_short_frame(self, request: List[int], response_len: int) -> Optional[List[int]]:
    if request is None or response_len <= 0 or len(request) <= 0 or len(request) > self.BMV080_I2C_MAX_FRAME_LEN or response_len > self.BMV080_I2C_MAX_FRAME_LEN:
      return None

    if not self._ensure_bus():
      return None

    bus = self._bus
    if bus is None:
      return None

    request_bytes = [int(byte) & 0xFF for byte in request]
    i2c_msg_mod = i2c_msg
    read_with_i2c_rdwr = hasattr(bus, "i2c_rdwr") and i2c_msg_mod is not None

    for _ in range(self.BMV080_I2C_SHORTFRAME_RETRY):
      try:
        if read_with_i2c_rdwr and i2c_msg_mod is not None:
          write_msg = i2c_msg_mod.write(self._addr, request_bytes)
          bus.i2c_rdwr(write_msg)
        else:
          # SMBus block write: first byte is command, the rest are payload.
          bus.write_i2c_block_data(self._addr, request_bytes[0], request_bytes[1:])
      except Exception:
        time.sleep(0.002)
        continue

      time.sleep(self.BMV080_I2C_SLAVE_SETTLE_MS / 1000.0)

      start = time.monotonic()
      while (time.monotonic() - start) <= self._timeout_s:
        try:
          if read_with_i2c_rdwr and i2c_msg_mod is not None:
            read_msg = i2c_msg_mod.read(self._addr, response_len)
            bus.i2c_rdwr(read_msg)
            response = list(read_msg)
          else:
            # Fallback path when only smbus is available.
            # Some platforms require a dummy command byte for block read.
            response = bus.read_i2c_block_data(self._addr, 0x00, response_len)
        except Exception:
          time.sleep(0.002)
          continue

        if len(response) >= 3:
          if response[0] == self.BMV080_I2C_SHORT_HEADER and response[1] == ((request_bytes[1] & 0xFF) | 0x80):
            return response[:3]
          if len(response) == response_len:
            return [int(value) & 0xFF for value in response]
        time.sleep(0.002)

    return None


class DFRobot_BMV080_Gravity_UART(DFRobot_BMV080_Gravity, DFRobot_RTU):
  '''!
  @brief UART Modbus RTU transport based on DFRobot_RTU.py.
  '''

  def __init__(
    self,
    baud: int = 9600,
    addr: int = DFRobot_BMV080_Gravity.DEFAULT_RTU_ADDR,
    bits: int = 8,
    parity: str = "N",
    stopbit: int = 1,
    port: str = "/dev/ttyAMA0",
  ) -> None:
    self._addr = addr
    DFRobot_BMV080_Gravity.__init__(self)
    DFRobot_RTU.__init__(self, baud, bits, parity, stopbit, port)

  def set_timeout_time_s(self, timeout_s: float) -> None:
    '''!
    @brief Set UART Modbus RTU timeout
    @param timeout_s Timeout in seconds
    '''
    DFRobot_RTU.set_timeout_time_s(self, timeout_s)

  def write_holding_reg(self, reg: int, data: List[int]) -> int:
    if data is None or len(data) == 0:
      return self.RET_CODE_ERROR

    try:
      if len(data) == 1:
        ret = self.write_holding_register(self._addr, reg, data[0] & 0xFFFF)
      else:
        payload = []
        for value in data:
          payload.append((value >> 8) & 0xFF)
          payload.append(value & 0xFF)
        ret = self.write_holding_registers(self._addr, reg, payload)
    except Exception:
      return self.ERR_DATA_BUS

    return int(ret)

  def read_holding_reg(self, reg: int, count: int) -> Optional[List[int]]:
    return self._read_registers_by_rtu(is_input=False, reg=reg, count=count)

  def read_input_reg(self, reg: int, count: int) -> Optional[List[int]]:
    return self._read_registers_by_rtu(is_input=True, reg=reg, count=count)

  def _read_registers_by_rtu(self, is_input: bool, reg: int, count: int) -> Optional[List[int]]:
    if count <= 0:
      return None

    try:
      if is_input:
        data = self.read_input_registers(self._addr, reg, count)
      else:
        data = self.read_holding_registers(self._addr, reg, count)
    except Exception:
      return None

    if data is None or len(data) < 1:
      return None

    ret = int(data[0])
    if ret != self.RET_CODE_OK:
      return None

    expected_len = 1 + (count * 2)
    if len(data) < expected_len:
      return None

    regs = []
    for index in range(count):
      high_word = data[1 + (index * 2)] & 0xFF
      low_word = data[2 + (index * 2)] & 0xFF
      regs.append(((high_word << 8) | low_word) & 0xFFFF)
    return regs


DFRobot_BMV080_Gravity_Data = DFRobot_BMV080_Gravity.BMV080Data
