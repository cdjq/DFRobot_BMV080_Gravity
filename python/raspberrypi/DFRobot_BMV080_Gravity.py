# -*- coding: utf-8 -*-
"""!
  @file  DFRobot_BMV080_Gravity.py
  @brief  Python driver for DFRobot BMV080 Gravity firmware module.
  @details  This library talks to the module firmware register map over I2C short frames
  @n       (0xA5 header). The ESP32 firmware on the module owns the Bosch BMV080 handle.
  @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
  @license     The MIT License (MIT)
  @author      DFRobot
  @version     V1.0.0
  @date        2026-05-13
  @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import struct
import time
from typing import List, Optional, Tuple

from DFRobot_RTU import DFRobot_RTU

try:
    import smbus  # type: ignore
except ImportError as exc:
    smbus = None
    _SMBUS_IMPORT_ERROR = exc
else:
    _SMBUS_IMPORT_ERROR = None

try:
    from smbus2 import i2c_msg  # type: ignore
except ImportError:
    i2c_msg = None


@dataclass
class sBmv080Data_t:
    """Cached PM data and state flags."""

    PM1: float = 0.0
    PM2_5: float = 0.0
    PM10: float = 0.0
    runtime: float = 0.0
    runState: int = 0
    status: int = 0
    isObstructed: bool = False
    isOutsideMeasurementRange: bool = False
    dataReady: bool = False
    measuring: bool = False
    paramsVerified: bool = False
    valueClamped: bool = False
    valueInvalid: bool = False
    sampleSeq: int = 0


class DFRobot_BMV080_Gravity(object):
    # Return/Error code
    RET_CODE_OK = 0
    RET_CODE_ERROR = 1
    ERR_OK = 0
    ERR_DATA_BUS = 1
    ERR_DATA_READ = 2
    ERR_IC_VERSION = 3

    # Device constants
    DFRobot_BMV080_GRAVITY_DEFAULT_ADDR = 0x57
    DFRobot_BMV080_GRAVITY_PID = 0x0296
    DFRobot_BMV080_GRAVITY_VID = 0x3343
    DFRobot_BMV080_GRAVITY_VERSION = 0x1000
    DFRobot_BMV080_GRAVITY_REG_MAP_VERSION = 0x0003

    # Measurement mode
    CONTINUOUS_MODE = 0
    DUTY_CYCLE_MODE = 1

    # Measurement algorithm
    FAST_RESPONSE = 1
    BALANCED = 2
    HIGH_PRECISION = 3

    # Action register values
    ACTION_START = 1
    ACTION_STOP = 2
    ACTION_RESET = 3

    # UART baud register values
    e2400 = 0x0001
    e4800 = 0x0002
    e9600 = 0x0003
    e14400 = 0x0004
    e19200 = 0x0005
    e38400 = 0x0006
    e57600 = 0x0007
    e115200 = 0x0008

    # UART parity
    eParityNone = 0x00
    eParityEven = 0x01
    eParityOdd = 0x02

    # UART stop bits
    eStopBit1 = 0x01
    eStopBit1_5 = 0x02
    eStopBit2 = 0x03

    # Run states (input register 0x0004)
    eRunStateBoot = 0
    eRunStateReady = 1
    eRunStateMeasuringContinuous = 2
    eRunStateMeasuringDuty = 3
    eRunStateStopped = 4
    eRunStateError = 5

    # Input registers
    REG_INPUT_PID = 0x0000
    REG_INPUT_VID = 0x0001
    REG_INPUT_VERSION = 0x0002
    REG_INPUT_REG_MAP_VERSION = 0x0003
    REG_INPUT_RUN_STATE = 0x0004
    REG_INPUT_LAST_STATUS = 0x0005
    REG_INPUT_PM1_F32_HI = 0x0006
    REG_INPUT_PM1_F32_LO = 0x0007
    REG_INPUT_PM25_F32_HI = 0x0008
    REG_INPUT_PM25_F32_LO = 0x0009
    REG_INPUT_PM10_F32_HI = 0x000A
    REG_INPUT_PM10_F32_LO = 0x000B
    REG_INPUT_RUNTIME_F32_HI = 0x000C
    REG_INPUT_RUNTIME_F32_LO = 0x000D
    REG_INPUT_FLAGS = 0x000E
    REG_INPUT_SAMPLE_SEQ = 0x000F
    REG_INPUT_DRIVER_MAJOR = 0x0010
    REG_INPUT_DRIVER_MINOR = 0x0011
    REG_INPUT_DRIVER_PATCH = 0x0012
    REG_INPUT_SENSOR_ID0 = 0x0013

    # Holding registers
    REG_HOLDING_BAUDRATE = 0x0000
    REG_HOLDING_VERIFY_STOP = 0x0001
    REG_HOLDING_ACTION = 0x0002
    REG_HOLDING_MEASURE_MODE = 0x0003
    REG_HOLDING_ALGORITHM = 0x0004
    REG_HOLDING_OBSTRUCTION = 0x0005
    REG_HOLDING_VIBRATION = 0x0006
    REG_HOLDING_INTEGRATION_F32_HI = 0x0007
    REG_HOLDING_INTEGRATION_F32_LO = 0x0008
    REG_HOLDING_DUTY_PERIOD_S = 0x0009

    # Input flag bits
    INPUT_FLAG_OBSTRUCTED = 1 << 0
    INPUT_FLAG_OUTSIDE_RANGE = 1 << 1
    INPUT_FLAG_DATA_READY = 1 << 2
    INPUT_FLAG_MEASURING = 1 << 4
    INPUT_FLAG_PARAMS_VERIFIED = 1 << 6
    INPUT_FLAG_VALUE_CLAMPED = 1 << 8
    INPUT_FLAG_VALUE_INVALID = 1 << 9

    def __init__(self) -> None:
        self._lastError = self.RET_CODE_OK
        self._data = sBmv080Data_t()

    def begin(self) -> bool:
        regs = self.readInputReg(self.REG_INPUT_PID, 2)
        comm_ok = False

        for _ in range(8):
            regs = self.readInputReg(self.REG_INPUT_PID, 2)
            if regs is not None:
                comm_ok = True
                pid = regs[0]
                vid = regs[1]
                if vid == self.DFRobot_BMV080_GRAVITY_VID and (
                    pid == self.DFRobot_BMV080_GRAVITY_PID
                ):
                    self._lastError = self.RET_CODE_OK
                    return True
            time.sleep(0.01)

        if not comm_ok:
            return False

        self._lastError = self.ERR_IC_VERSION
        return False

    def getPID(self) -> int:
        value = self._readInputValue(self.REG_INPUT_PID)
        return value if value is not None else 0

    def getVID(self) -> int:
        value = self._readInputValue(self.REG_INPUT_VID)
        return value if value is not None else 0

    def getVersion(self) -> int:
        value = self._readInputValue(self.REG_INPUT_VERSION)
        return value if value is not None else 0

    def getRegMapVersion(self) -> int:
        value = self._readInputValue(self.REG_INPUT_REG_MAP_VERSION)
        return value if value is not None else 0

    def getRunState(self) -> int:
        value = self._readInputValue(self.REG_INPUT_RUN_STATE)
        return value if value is not None else 0

    def getStatus(self) -> int:
        value = self._readInputValue(self.REG_INPUT_LAST_STATUS)
        return value if value is not None else 0

    def getBmv080DV(self) -> Optional[Tuple[int, int, int]]:
        regs = self.readInputReg(self.REG_INPUT_DRIVER_MAJOR, 3)
        if regs is None:
            return None
        return regs[0], regs[1], regs[2]

    def getBmv080ID(self) -> Optional[str]:
        regs = self.readInputReg(self.REG_INPUT_SENSOR_ID0, 6)
        if regs is None:
            return None

        raw = bytearray()
        for word in regs:
            raw.append((word >> 8) & 0xFF)
            raw.append(word & 0xFF)
        return raw.decode("ascii", errors="ignore").rstrip("\x00")

    def readBmv080Data(self) -> Optional[sBmv080Data_t]:
        regs = None
        for _ in range(3):
            regs = self.readInputReg(self.REG_INPUT_RUN_STATE, 12)
            if regs is not None:
                break
            time.sleep(0.003)
        if regs is None:
            return None

        flags = regs[10]
        self._data = sBmv080Data_t(
            runState=regs[0],
            status=regs[1],
            PM1=self._regs_to_float(regs[2], regs[3]),
            PM2_5=self._regs_to_float(regs[4], regs[5]),
            PM10=self._regs_to_float(regs[6], regs[7]),
            runtime=self._regs_to_float(regs[8], regs[9]),
            isObstructed=(flags & self.INPUT_FLAG_OBSTRUCTED) != 0,
            isOutsideMeasurementRange=(flags & self.INPUT_FLAG_OUTSIDE_RANGE) != 0,
            dataReady=(flags & self.INPUT_FLAG_DATA_READY) != 0,
            measuring=(flags & self.INPUT_FLAG_MEASURING) != 0,
            paramsVerified=(flags & self.INPUT_FLAG_PARAMS_VERIFIED) != 0,
            valueClamped=(flags & self.INPUT_FLAG_VALUE_CLAMPED) != 0,
            valueInvalid=(flags & self.INPUT_FLAG_VALUE_INVALID) != 0,
            sampleSeq=regs[11],
        )
        self._lastError = self.RET_CODE_OK
        return sBmv080Data_t(**self._data.__dict__)

    def getBmv080Data(self) -> Optional[sBmv080Data_t]:
        data = self.readBmv080Data()
        if data is None or not data.dataReady:
            return None
        return data

    def setBmv080Mode(self, mode: int) -> int:
        if mode not in (self.CONTINUOUS_MODE, self.DUTY_CYCLE_MODE):
            self._lastError = self.ERR_DATA_READ
            return -1

        ret = self._writeHoldingValue(self.REG_HOLDING_MEASURE_MODE, mode)
        if ret != self.RET_CODE_OK:
            return int(ret)
        ret = self._writeHoldingValue(self.REG_HOLDING_ACTION, self.ACTION_START)
        return int(ret)

    def stopBmv080(self) -> bool:
        return self._writeHoldingValue(self.REG_HOLDING_ACTION, self.ACTION_STOP) == self.RET_CODE_OK

    def resetBmv080(self) -> bool:
        return self._writeHoldingValue(self.REG_HOLDING_ACTION, self.ACTION_RESET) == self.RET_CODE_OK

    def setIntegrationTime(self, integration_time: float) -> int:
        if not math.isfinite(integration_time):
            self._lastError = self.ERR_DATA_READ
            return -1
        regs = self._float_to_regs(integration_time)
        ret = self.writeHoldingReg(self.REG_HOLDING_INTEGRATION_F32_HI, regs)
        self._lastError = ret
        return int(ret)

    def getIntegrationTime(self) -> float:
        values = self.readHoldingReg(self.REG_HOLDING_INTEGRATION_F32_HI, 2)
        if values is None:
            return math.nan
        return self._regs_to_float(values[0], values[1])

    def setDutyCyclingPeriod(self, duty_cycling_period: int) -> int:
        return int(self._writeHoldingValue(self.REG_HOLDING_DUTY_PERIOD_S, duty_cycling_period))

    def getDutyCyclingPeriod(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_DUTY_PERIOD_S)
        return value if value is not None else 0

    def setObstructionDetection(self, enable: bool) -> bool:
        return self._writeHoldingValue(self.REG_HOLDING_OBSTRUCTION, 1 if enable else 0) == self.RET_CODE_OK

    def getObstructionDetection(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_OBSTRUCTION)
        if value is None:
            return -1
        return 1 if value != 0 else 0

    def setDoVibrationFiltering(self, enable: bool) -> bool:
        return self._writeHoldingValue(self.REG_HOLDING_VIBRATION, 1 if enable else 0) == self.RET_CODE_OK

    def getDoVibrationFiltering(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_VIBRATION)
        if value is None:
            return -1
        return 1 if value != 0 else 0

    def setMeasurementAlgorithm(self, measurement_algorithm: int) -> int:
        if measurement_algorithm < self.FAST_RESPONSE or measurement_algorithm > self.HIGH_PRECISION:
            self._lastError = self.ERR_DATA_READ
            return -1
        return int(self._writeHoldingValue(self.REG_HOLDING_ALGORITHM, measurement_algorithm))

    def getMeasurementAlgorithm(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_ALGORITHM)
        if value is None:
            return 0
        if value < self.FAST_RESPONSE or value > self.HIGH_PRECISION:
            self._lastError = self.ERR_DATA_READ
            return 0
        return int(value)

    def setBaud(self, baud: int) -> int:
        if baud < self.e2400 or baud > self.e115200:
            self._lastError = self.ERR_DATA_READ
            return self.RET_CODE_ERROR
        return int(self._writeHoldingValue(self.REG_HOLDING_BAUDRATE, baud))

    def getBaud(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_BAUDRATE)
        return value if value is not None else 0

    def getBaudValue(self) -> int:
        return self.baudRegToValue(self.getBaud())

    def setUartFormat(self, parity: int, stop_bit: int = eStopBit1) -> int:
        if parity > self.eParityOdd or stop_bit < self.eStopBit1 or stop_bit > self.eStopBit2:
            self._lastError = self.ERR_DATA_READ
            return self.RET_CODE_ERROR

        value = ((parity & 0xFF) << 8) | (stop_bit & 0xFF)
        return int(self._writeHoldingValue(self.REG_HOLDING_VERIFY_STOP, value))

    def getUartFormat(self) -> int:
        value = self._readHoldingValue(self.REG_HOLDING_VERIFY_STOP)
        return value if value is not None else 0

    def getLastError(self) -> int:
        return int(self._lastError)

    def baudRegToValue(self, baud_reg: int) -> int:
        mapping = {
            self.e2400: 2400,
            self.e4800: 4800,
            self.e9600: 9600,
            self.e14400: 14400,
            self.e19200: 19200,
            self.e38400: 38400,
            self.e57600: 57600,
            self.e115200: 115200,
        }
        return mapping.get(baud_reg, 9600)

    # Python-style aliases
    def set_bmv080_mode(self, mode: int) -> int:
        return self.setBmv080Mode(mode)

    def stop_bmv080(self) -> bool:
        return self.stopBmv080()

    def reset_bmv080(self) -> bool:
        return self.resetBmv080()

    def get_last_error(self) -> int:
        return self.getLastError()

    # Virtual transport methods
    def writeHoldingReg(self, reg: int, data: List[int]) -> int:
        raise NotImplementedError

    def readHoldingReg(self, reg: int, count: int) -> Optional[List[int]]:
        raise NotImplementedError

    def readInputReg(self, reg: int, count: int) -> Optional[List[int]]:
        raise NotImplementedError

    def _readInputValue(self, reg: int) -> Optional[int]:
        for _ in range(3):
            data = self.readInputReg(reg, 1)
            if data is not None:
                self._lastError = self.RET_CODE_OK
                return data[0]
            time.sleep(0.005)
        return None

    def _readHoldingValue(self, reg: int) -> Optional[int]:
        for _ in range(3):
            data = self.readHoldingReg(reg, 1)
            if data is not None:
                self._lastError = self.RET_CODE_OK
                return data[0]
            time.sleep(0.005)
        return None

    def _writeHoldingValue(self, reg: int, value: int) -> int:
        ret = self.writeHoldingReg(reg, [value & 0xFFFF])
        self._lastError = ret
        return ret

    @staticmethod
    def _float_to_regs(value: float) -> List[int]:
        raw = struct.unpack(">I", struct.pack(">f", float(value)))[0]
        return [(raw >> 16) & 0xFFFF, raw & 0xFFFF]

    @staticmethod
    def _regs_to_float(hi: int, lo: int) -> float:
        raw = ((int(hi) & 0xFFFF) << 16) | (int(lo) & 0xFFFF)
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
    BMV080_I2C_SLAVE_SETTLE_MS = 20
    BMV080_I2C_VALIDATE_RETRY = 3

    def __init__(self, bus: int = 1, addr: int = DFRobot_BMV080_Gravity.DFRobot_BMV080_GRAVITY_DEFAULT_ADDR) -> None:
        super().__init__()
        self._bus_id = bus
        self._addr = addr
        self._timeout_s = 0.1
        self._bus = None

    def begin(self) -> bool:
        if not self._ensureBus():
            return False
        return super().begin()

    def close(self) -> None:
        if self._bus is not None:
            try:
                self._bus.close()
            except Exception:
                pass
            self._bus = None

    def setTimeoutTimeMs(self, timeout_ms: int) -> None:
        if timeout_ms < 1:
            timeout_ms = 1
        self._timeout_s = timeout_ms / 1000.0

    def writeHoldingReg(self, reg: int, data: List[int]) -> int:
        if data is None or len(data) == 0:
            self._lastError = self.ERR_DATA_READ
            return self.RET_CODE_ERROR
        if len(data) == 1:
            return self._writeSingleReg(reg, data[0])
        return self._writeMultiRegs(reg, data)

    def readHoldingReg(self, reg: int, count: int) -> Optional[List[int]]:
        return self._readRegs(self.BMV080_I2C_FUNC_READ_HOLDING, reg, count)

    def readInputReg(self, reg: int, count: int) -> Optional[List[int]]:
        return self._readRegs(self.BMV080_I2C_FUNC_READ_INPUT, reg, count)

    def _ensureBus(self) -> bool:
        if self._bus is not None:
            return True
        if smbus is None:
            raise ImportError(
                "smbus is required. Install with `sudo apt install python3-smbus` "
                "or `pip install smbus2`."
            ) from _SMBUS_IMPORT_ERROR
        try:
            self._bus = smbus.SMBus(self._bus_id)
        except Exception:
            self._lastError = self.ERR_DATA_BUS
            return False
        return True

    def _readRegs(self, func: int, reg: int, count: int) -> Optional[List[int]]:
        if count <= 0 or count > self.BMV080_I2C_MAX_READ_REGS:
            self._lastError = self.ERR_DATA_READ
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
            response = self._transferShortFrame(request, response_len)
            if response is None:
                continue

            if response[0] != self.BMV080_I2C_SHORT_HEADER:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            if response[1] == (func | 0x80):
                self._lastError = response[2]
                return None

            if response[1] != func or response[2] != (count * 2) or len(response) != response_len:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            regs = []
            for idx in range(count):
                hi = response[3 + (idx * 2)]
                lo = response[4 + (idx * 2)]
                regs.append(((hi << 8) | lo) & 0xFFFF)
            self._lastError = self.RET_CODE_OK
            return regs

        if self._lastError == self.RET_CODE_OK:
            self._lastError = self.ERR_DATA_READ
        return None

    def _writeSingleReg(self, reg: int, value: int) -> int:
        request = [
            self.BMV080_I2C_SHORT_HEADER,
            self.BMV080_I2C_FUNC_WRITE_HOLDING,
            (reg >> 8) & 0xFF,
            reg & 0xFF,
            (value >> 8) & 0xFF,
            value & 0xFF,
        ]

        for _ in range(self.BMV080_I2C_VALIDATE_RETRY):
            response = self._transferShortFrame(request, 6)
            if response is None:
                continue

            if response[0] != self.BMV080_I2C_SHORT_HEADER:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            if response[1] == (self.BMV080_I2C_FUNC_WRITE_HOLDING | 0x80):
                self._lastError = response[2]
                return self._lastError

            if response != request:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            self._lastError = self.RET_CODE_OK
            return self.RET_CODE_OK

        if self._lastError == self.RET_CODE_OK:
            self._lastError = self.ERR_DATA_READ
        return self._lastError

    def _writeMultiRegs(self, reg: int, data: List[int]) -> int:
        count = len(data)
        if count == 0 or count > self.BMV080_I2C_MAX_WRITE_REGS:
            self._lastError = self.ERR_DATA_READ
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

        for _ in range(self.BMV080_I2C_VALIDATE_RETRY):
            response = self._transferShortFrame(request, 6)
            if response is None:
                continue

            if response[0] != self.BMV080_I2C_SHORT_HEADER:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            if response[1] == (self.BMV080_I2C_FUNC_WRITE_MULTI_HOLDING | 0x80):
                self._lastError = response[2]
                return self._lastError

            resp_reg = ((response[2] << 8) | response[3]) & 0xFFFF
            resp_count = ((response[4] << 8) | response[5]) & 0xFFFF
            if response[1] != self.BMV080_I2C_FUNC_WRITE_MULTI_HOLDING or resp_reg != reg or resp_count != count:
                self._lastError = self.ERR_DATA_READ
                time.sleep(0.003)
                continue

            self._lastError = self.RET_CODE_OK
            return self.RET_CODE_OK

        if self._lastError == self.RET_CODE_OK:
            self._lastError = self.ERR_DATA_READ
        return self._lastError

    def _transferShortFrame(self, request: List[int], response_len: int) -> Optional[List[int]]:
        if (
            request is None
            or response_len <= 0
            or len(request) <= 0
            or len(request) > self.BMV080_I2C_MAX_FRAME_LEN
            or response_len > self.BMV080_I2C_MAX_FRAME_LEN
        ):
            self._lastError = self.ERR_DATA_READ
            return None

        if not self._ensureBus():
            return None

        req = [int(b) & 0xFF for b in request]
        read_with_i2c_rdwr = hasattr(self._bus, "i2c_rdwr") and i2c_msg is not None

        for _ in range(self.BMV080_I2C_SHORTFRAME_RETRY):
            try:
                if read_with_i2c_rdwr:
                    write_msg = i2c_msg.write(self._addr, req)
                    self._bus.i2c_rdwr(write_msg)
                else:
                    # SMBus block write: first byte is command, the rest are payload.
                    self._bus.write_i2c_block_data(self._addr, req[0], req[1:])
            except Exception:
                self._lastError = self.ERR_DATA_BUS
                time.sleep(0.002)
                continue

            time.sleep(self.BMV080_I2C_SLAVE_SETTLE_MS / 1000.0)

            start = time.monotonic()
            while (time.monotonic() - start) <= self._timeout_s:
                try:
                    if read_with_i2c_rdwr:
                        read_msg = i2c_msg.read(self._addr, response_len)
                        self._bus.i2c_rdwr(read_msg)
                        response = list(read_msg)
                    else:
                        # Fallback path when only smbus is available.
                        # Some platforms require a dummy command byte for block read.
                        response = self._bus.read_i2c_block_data(self._addr, 0x00, response_len)
                except Exception:
                    self._lastError = self.ERR_DATA_READ
                    time.sleep(0.002)
                    continue

                if len(response) >= 3:
                    if (
                        response[0] == self.BMV080_I2C_SHORT_HEADER
                        and response[1] == ((req[1] & 0xFF) | 0x80)
                    ):
                        self._lastError = response[2]
                        return response[:3]
                    if len(response) == response_len:
                        self._lastError = self.RET_CODE_OK
                        return [int(v) & 0xFF for v in response]
                time.sleep(0.002)

        if self._lastError == self.RET_CODE_OK:
            self._lastError = self.ERR_DATA_READ
        return None


class DFRobot_BMV080_Gravity_UART(DFRobot_BMV080_Gravity, DFRobot_RTU):
    """UART Modbus RTU transport based on DFRobot_RTU.py."""

    def __init__(
        self,
        baud: int = 9600,
        addr: int = DFRobot_BMV080_Gravity.DFRobot_BMV080_GRAVITY_DEFAULT_ADDR,
        bits: int = 8,
        parity: str = "N",
        stopbit: int = 1,
    ) -> None:
        self._addr = addr
        DFRobot_BMV080_Gravity.__init__(self)
        DFRobot_RTU.__init__(self, baud, bits, parity, stopbit)

    def setTimeoutTimeS(self, timeout_s: float) -> None:
        self.set_timout_time_s(timeout_s)

    def writeHoldingReg(self, reg: int, data: List[int]) -> int:
        if data is None or len(data) == 0:
            self._lastError = self.ERR_DATA_READ
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
            self._lastError = self.ERR_DATA_BUS
            return self.ERR_DATA_BUS

        self._lastError = int(ret)
        return int(ret)

    def readHoldingReg(self, reg: int, count: int) -> Optional[List[int]]:
        return self._readRegistersByRtu(is_input=False, reg=reg, count=count)

    def readInputReg(self, reg: int, count: int) -> Optional[List[int]]:
        return self._readRegistersByRtu(is_input=True, reg=reg, count=count)

    def _readRegistersByRtu(self, is_input: bool, reg: int, count: int) -> Optional[List[int]]:
        if count <= 0:
            self._lastError = self.ERR_DATA_READ
            return None

        try:
            if is_input:
                data = self.read_input_registers(self._addr, reg, count)
            else:
                data = self.read_holding_registers(self._addr, reg, count)
        except Exception:
            self._lastError = self.ERR_DATA_BUS
            return None

        if data is None or len(data) < 1:
            self._lastError = self.ERR_DATA_READ
            return None

        ret = int(data[0])
        if ret != self.RET_CODE_OK:
            self._lastError = ret
            return None

        expected_len = 1 + (count * 2)
        if len(data) < expected_len:
            self._lastError = self.ERR_DATA_READ
            return None

        regs = []
        for i in range(count):
            hi = data[1 + (i * 2)] & 0xFF
            lo = data[2 + (i * 2)] & 0xFF
            regs.append(((hi << 8) | lo) & 0xFFFF)
        self._lastError = self.RET_CODE_OK
        return regs


# Module-level exports for easier example code
CONTINUOUS_MODE = DFRobot_BMV080_Gravity.CONTINUOUS_MODE
DUTY_CYCLE_MODE = DFRobot_BMV080_Gravity.DUTY_CYCLE_MODE
FAST_RESPONSE = DFRobot_BMV080_Gravity.FAST_RESPONSE
BALANCED = DFRobot_BMV080_Gravity.BALANCED
HIGH_PRECISION = DFRobot_BMV080_Gravity.HIGH_PRECISION
