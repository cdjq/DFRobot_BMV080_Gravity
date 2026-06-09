/**
 * @file  DFRobot_BMV080_Gravity.cpp
 * @brief  Driver for DFRobot BMV080 Gravity firmware.
 * @n      Implements I2C short-frame and UART Modbus RTU transport layers for reading PM data and configuring
 * @n      BMV080 sensor parameters through the ESP32 firmware register table.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */

#include "DFRobot_BMV080_Gravity.h"

#define BMV080_I2C_SHORT_HEADER             0xA5
#define BMV080_I2C_FUNC_READ_HOLDING        0x03
#define BMV080_I2C_FUNC_READ_INPUT          0x04
#define BMV080_I2C_FUNC_WRITE_HOLDING       0x06
#define BMV080_I2C_FUNC_WRITE_MULTI_HOLDING 0x10
#define BMV080_I2C_MAX_FRAME_LEN            32
#define BMV080_I2C_MAX_READ_REGS            14
#define BMV080_I2C_MAX_WRITE_REGS           13
#define BMV080_I2C_SHORTFRAME_RETRY         8
#define BMV080_I2C_SLAVE_SETTLE_MS          30
#define BMV080_I2C_VALIDATE_RETRY           3

static void putBE16(uint16_t data, uint8_t *buf)
{
  buf[0] = (uint8_t)((data >> 8) & 0xFF);
  buf[1] = (uint8_t)(data & 0xFF);
}

static uint16_t getBE16(const uint8_t *buf)
{
  return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

static void floatToWords(float value, uint16_t &hi, uint16_t &lo)
{
  uint32_t raw = 0;
  memcpy(&raw, &value, sizeof(raw));
  hi = (uint16_t)((raw >> 16) & 0xFFFFU);
  lo = (uint16_t)(raw & 0xFFFFU);
}

static float wordsToFloat(uint16_t hi, uint16_t lo)
{
  uint32_t raw   = (((uint32_t)hi) << 16) | (uint32_t)lo;
  float    value = 0.0f;
  memcpy(&value, &raw, sizeof(value));
  return value;
}

DFRobot_BMV080_Gravity::DFRobot_BMV080_Gravity(void) : _data({ 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, false, false, false, false, false, false, false, 0 }) {}

DFRobot_BMV080_Gravity::~DFRobot_BMV080_Gravity(void) {}

bool DFRobot_BMV080_Gravity::begin(void)
{
  uint16_t regs[4] = { 0 };
  bool     commOk  = false;
  uint8_t  ret     = RET_CODE_ERROR;

  // Some host/slave pairs may return one stale frame right after boot/mode switch.
  // Retry a few times before concluding transport or ID mismatch.
  for (uint8_t i = 0; i < BMV080_I2C_SHORTFRAME_RETRY; i++) {
    ret = readInputReg(REG_INPUT_PID, regs, 4);
    if (ret == RET_CODE_OK) {
      commOk = true;
      if ((regs[0] == EXPECTED_PID) && (regs[1] == EXPECTED_VID) && (regs[3] == EXPECTED_REG_MAP_VERSION)) {
        return true;
      }
    }
    delay(10);
  }
  return false;
}

bool DFRobot_BMV080_Gravity::readData(sData_t *data)
{
  if (data == NULL) {
    return false;
  }
  uint16_t regs[12] = { 0 };
  uint8_t  ret      = RET_CODE_ERROR;
  for (uint8_t attempt = 0; attempt < BMV080_I2C_VALIDATE_RETRY; attempt++) {
    ret = readInputReg(REG_INPUT_RUN_STATE, regs, 12);
    if (ret == RET_CODE_OK) {
      break;
    }
    delay(3);
  }
  if (ret != RET_CODE_OK) {
    return false;
  }

  _data.runState                  = regs[0];
  _data.status                    = regs[1];
  _data.PM1                       = wordsToFloat(regs[2], regs[3]);
  _data.PM2_5                     = wordsToFloat(regs[4], regs[5]);
  _data.PM10                      = wordsToFloat(regs[6], regs[7]);
  _data.runtime                   = wordsToFloat(regs[8], regs[9]);
  _data.isObstructed              = (regs[10] & INPUT_FLAG_OBSTRUCTED) != 0;
  _data.isOutsideMeasurementRange = (regs[10] & INPUT_FLAG_OUTSIDE_RANGE) != 0;
  _data.dataReady                 = (regs[10] & INPUT_FLAG_DATA_READY) != 0;
  _data.measuring                 = (regs[10] & INPUT_FLAG_MEASURING) != 0;
  _data.paramsVerified            = (regs[10] & INPUT_FLAG_PARAMS_VERIFIED) != 0;
  _data.valueClamped              = (regs[10] & INPUT_FLAG_VALUE_CLAMPED) != 0;
  _data.valueInvalid              = (regs[10] & INPUT_FLAG_VALUE_INVALID) != 0;
  _data.sampleSeq                 = regs[11];

  *data = _data;
  return true;
}

bool DFRobot_BMV080_Gravity::getData(sData_t *data)
{
  if (!readData(data)) {
    return false;
  }
  return _data.dataReady;
}

int DFRobot_BMV080_Gravity::setMeasureMode(eMeasureMode_t mode)
{
  int      ret         = 0;
  uint16_t regs[2]     = { 0 };
  uint16_t targetState = (mode == eDutyCycleMode) ? eRunStateMeasuringDuty : eRunStateMeasuringContinuous;
  uint32_t startTime   = 0;
  uint32_t waitTimeout = 1000;
  if ((mode != eContinuousMode) && (mode != eDutyCycleMode)) {
    return -1;
  }

  ret = writeHoldingValue(REG_HOLDING_MEASURE_MODE, (uint16_t)mode);
  if (ret != RET_CODE_OK) {
    return ret;
  }
  ret = writeAction(eStart);
  if (ret != RET_CODE_OK) {
    return ret;
  }

  startTime = millis();
  do {
    ret = readInputReg(REG_INPUT_RUN_STATE, regs, 2);
    if (ret == RET_CODE_OK) {
      if (regs[0] == targetState) {
        return RET_CODE_OK;
      }
      if (regs[0] == eRunStateError) {
        return regs[1] == RET_CODE_OK ? RET_CODE_ERROR : regs[1];
      }
    }
    delay(10);
  } while ((millis() - startTime) < waitTimeout);

  return ERR_DATA_READ;
}

bool DFRobot_BMV080_Gravity::stopMeasurement(void)
{
  return writeAction(eStop) == RET_CODE_OK;
}

bool DFRobot_BMV080_Gravity::reset(void)
{
  return writeAction(eReset) == RET_CODE_OK;
}

uint8_t DFRobot_BMV080_Gravity::writeAction(eAction_t action)
{
  return writeHoldingValue(REG_HOLDING_ACTION, (uint16_t)action);
}

int DFRobot_BMV080_Gravity::setIntegrationTime(float integration_time)
{
  uint16_t data[2]    = { 0 };
  uint16_t dutyPeriod = getDutyCyclingPeriod();
  if (isnan(integration_time) || isinf(integration_time) || (integration_time <= 0.0f)) {
    return -1;
  }
  if ((dutyPeriod == 0) || (((float)dutyPeriod) < (integration_time + 2.0f))) {
    return -1;
  }
  floatToWords(integration_time, data[0], data[1]);
  return writeHoldingValues(REG_HOLDING_INTEGRATION_F32_HI, data, 2);
}

float DFRobot_BMV080_Gravity::getIntegrationTime(void)
{
  uint16_t data[2] = { 0 };
  uint8_t  ret     = readHoldingReg(REG_HOLDING_INTEGRATION_F32_HI, data, 2);
  if (ret != RET_CODE_OK) {
    return NAN;
  }
  return wordsToFloat(data[0], data[1]);
}

int DFRobot_BMV080_Gravity::setDutyCyclingPeriod(uint16_t duty_cycling_period)
{
  float integrationTime = getIntegrationTime();
  if ((duty_cycling_period < 3) || isnan(integrationTime) || isinf(integrationTime) || (((float)duty_cycling_period) < (integrationTime + 2.0f))) {
    return -1;
  }
  return writeHoldingValue(REG_HOLDING_DUTY_PERIOD_S, duty_cycling_period);
}

uint16_t DFRobot_BMV080_Gravity::getDutyCyclingPeriod(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_DUTY_PERIOD_S, data)) {
    return 0;
  }
  return data;
}

bool DFRobot_BMV080_Gravity::setObstructionDetection(bool enable)
{
  return writeHoldingValue(REG_HOLDING_OBSTRUCTION, enable ? 1 : 0) == RET_CODE_OK;
}

int DFRobot_BMV080_Gravity::getObstructionDetection(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_OBSTRUCTION, data)) {
    return -1;
  }
  return data ? 1 : 0;
}

bool DFRobot_BMV080_Gravity::setVibrationFiltering(bool enable)
{
  return writeHoldingValue(REG_HOLDING_VIBRATION, enable ? 1 : 0) == RET_CODE_OK;
}

int DFRobot_BMV080_Gravity::getVibrationFiltering(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_VIBRATION, data)) {
    return -1;
  }
  return data ? 1 : 0;
}

int DFRobot_BMV080_Gravity::setMeasurementAlgorithm(eMeasurementAlgorithm_t measurement_algorithm)
{
  if ((measurement_algorithm < eFastResponse) || (measurement_algorithm > eHighPrecision)) {
    return -1;
  }
  return writeHoldingValue(REG_HOLDING_ALGORITHM, (uint16_t)measurement_algorithm);
}

DFRobot_BMV080_Gravity::eMeasurementAlgorithm_t DFRobot_BMV080_Gravity::getMeasurementAlgorithm(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_ALGORITHM, data)) {
    return (eMeasurementAlgorithm_t)0;
  }
  if ((data < eFastResponse) || (data > eHighPrecision)) {
    return (eMeasurementAlgorithm_t)0;
  }
  return (eMeasurementAlgorithm_t)data;
}

uint8_t DFRobot_BMV080_Gravity::setBaud(eBaud_t baud)
{
  if ((baud < e2400) || (baud > e115200)) {
    return RET_CODE_ERROR;
  }
  return writeHoldingValue(REG_HOLDING_BAUDRATE, (uint16_t)baud);
}

uint32_t DFRobot_BMV080_Gravity::getBaud(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_BAUDRATE, data)) {
    return 0;
  }
  return baudRegToValue(data);
}

uint8_t DFRobot_BMV080_Gravity::setUartFormat(eParity_t parity, eStopBit_t stopBit)
{
  uint16_t data = 0;
  if ((parity > eParityOdd) || (stopBit < eStopBit1) || (stopBit > eStopBit2)) {
    return RET_CODE_ERROR;
  }
  data = (uint16_t)(((uint16_t)parity << 8) | (uint16_t)stopBit);
  return writeHoldingValue(REG_HOLDING_VERIFY_STOP, data);
}

uint16_t DFRobot_BMV080_Gravity::getUartFormat(void)
{
  uint16_t data = 0;
  if (!readHoldingValue(REG_HOLDING_VERIFY_STOP, data)) {
    return 0;
  }
  return data;
}

bool DFRobot_BMV080_Gravity::readHoldingValue(uint16_t reg, uint16_t &value)
{
  uint8_t ret = 0;
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    ret = readHoldingReg(reg, &value, 1);
    if (ret == RET_CODE_OK)
      break;
    delay(5);
  }
  return ret == RET_CODE_OK;
}

uint8_t DFRobot_BMV080_Gravity::writeHoldingValue(uint16_t reg, uint16_t value)
{
  return writeHoldingValues(reg, &value, 1);
}

uint8_t DFRobot_BMV080_Gravity::writeHoldingValues(uint16_t reg, const uint16_t *data, uint16_t count)
{
  return writeHoldingReg(reg, data, count);
}

uint32_t DFRobot_BMV080_Gravity::baudRegToValue(uint16_t baudReg)
{
  switch (baudReg) {
    case e2400:
      return 2400;
    case e4800:
      return 4800;
    case e9600:
      return 9600;
    case e14400:
      return 14400;
    case e19200:
      return 19200;
    case e38400:
      return 38400;
    case e57600:
      return 57600;
    case e115200:
      return 115200;
    default:
      return 9600;
  }
}

DFRobot_BMV080_Gravity_I2C::DFRobot_BMV080_Gravity_I2C(TwoWire *pWire, uint8_t addr) : _pWire(pWire), _i2cAddr(addr), _timeout(1000) {}

DFRobot_BMV080_Gravity_I2C::~DFRobot_BMV080_Gravity_I2C(void) {}

bool DFRobot_BMV080_Gravity_I2C::begin(void)
{
  if (_pWire == NULL) {
    return false;
  }
  _pWire->begin();
  return DFRobot_BMV080_Gravity::begin();
}

void DFRobot_BMV080_Gravity_I2C::setTimeoutTimeMs(uint32_t timeout)
{
  _timeout = timeout;
}

uint8_t DFRobot_BMV080_Gravity_I2C::writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count)
{
  if ((data == NULL) || (count == 0)) {
    return RET_CODE_ERROR;
  }
  if (count == 1) {
    return writeSingleReg(reg, data[0]);
  }
  return writeMultiRegs(reg, data, count);
}

uint8_t DFRobot_BMV080_Gravity_I2C::readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count)
{
  return readRegs(BMV080_I2C_FUNC_READ_HOLDING, reg, data, count);
}

uint8_t DFRobot_BMV080_Gravity_I2C::readInputReg(uint16_t reg, uint16_t *data, uint16_t count)
{
  return readRegs(BMV080_I2C_FUNC_READ_INPUT, reg, data, count);
}

uint8_t DFRobot_BMV080_Gravity_I2C::readRegs(uint8_t func, uint16_t reg, uint16_t *data, uint16_t count)
{
  // Short-frame format: [0]=0xA5 [1]=func [2..3]=start register [4]=register count.
  uint8_t request[5]                         = { BMV080_I2C_SHORT_HEADER, func, 0, 0, 0 };
  uint8_t response[BMV080_I2C_MAX_FRAME_LEN] = { 0 };
  uint8_t ret                                = ERR_DATA_READ;
  uint8_t responseLen                        = 0;

  if ((data == NULL) || (count == 0) || (count > BMV080_I2C_MAX_READ_REGS)) {
    return RET_CODE_ERROR;
  }

  putBE16(reg, &request[2]);
  request[4]  = (uint8_t)count;
  responseLen = (uint8_t)(3 + (count * 2));

  for (uint8_t attempt = 0; attempt < BMV080_I2C_VALIDATE_RETRY; attempt++) {
    if (!transferShortFrame(request, sizeof(request), response, responseLen)) {
      continue;
    }

    if ((response[0] != BMV080_I2C_SHORT_HEADER) || (response[1] == (func | 0x80))) {
      ret = (response[0] == BMV080_I2C_SHORT_HEADER) ? response[2] : ERR_DATA_READ;
      // Valid exception frame, return directly.
      if ((response[0] == BMV080_I2C_SHORT_HEADER) && (response[1] == (func | 0x80))) {
        return ret;
      }
      delay(3);
      continue;
    }
    if ((response[1] != func) || (response[2] != (uint8_t)(count * 2))) {
      ret = ERR_DATA_READ;
      delay(3);
      continue;
    }

    for (uint16_t i = 0; i < count; i++) {
      data[i] = getBE16(&response[3 + (i * 2)]);
    }
    return RET_CODE_OK;
  }

  return ret;
}

uint8_t DFRobot_BMV080_Gravity_I2C::writeSingleReg(uint16_t reg, uint16_t value)
{
  uint8_t request[6]  = { BMV080_I2C_SHORT_HEADER, BMV080_I2C_FUNC_WRITE_HOLDING, 0, 0, 0, 0 };
  uint8_t response[6] = { 0 };
  uint8_t ret         = ERR_DATA_READ;

  putBE16(reg, &request[2]);
  putBE16(value, &request[4]);

  for (uint8_t attempt = 0; attempt < BMV080_I2C_VALIDATE_RETRY; attempt++) {
    if (!transferShortFrame(request, sizeof(request), response, sizeof(response))) {
      continue;
    }
    if ((response[0] != BMV080_I2C_SHORT_HEADER) || (response[1] == (BMV080_I2C_FUNC_WRITE_HOLDING | 0x80))) {
      ret = (response[0] == BMV080_I2C_SHORT_HEADER) ? response[2] : ERR_DATA_READ;
      // Valid exception frame, return directly.
      if ((response[0] == BMV080_I2C_SHORT_HEADER) && (response[1] == (BMV080_I2C_FUNC_WRITE_HOLDING | 0x80))) {
        return ret;
      }
      delay(3);
      continue;
    }
    bool echoOk = true;
    for (uint8_t i = 0; i < sizeof(request); i++) {
      if (request[i] != response[i]) {
        echoOk = false;
        break;
      }
    }
    if (!echoOk) {
      ret = ERR_DATA_READ;
      delay(3);
      continue;
    }
    return RET_CODE_OK;
  }

  return ret;
}

uint8_t DFRobot_BMV080_Gravity_I2C::writeMultiRegs(uint16_t reg, const uint16_t *data, uint16_t count)
{
  // Multi-write short frame:
  // [0]=0xA5 [1]=0x10 [2..3]=start register [4]=count [5]=byte count [6..]=payload.
  uint8_t request[BMV080_I2C_MAX_FRAME_LEN] = { 0 };
  uint8_t response[6]                       = { 0 };
  uint8_t ret                               = ERR_DATA_READ;
  uint8_t requestLen                        = 0;

  if ((data == NULL) || (count == 0) || (count > BMV080_I2C_MAX_WRITE_REGS)) {
    return RET_CODE_ERROR;
  }

  request[0] = BMV080_I2C_SHORT_HEADER;
  request[1] = BMV080_I2C_FUNC_WRITE_MULTI_HOLDING;
  putBE16(reg, &request[2]);
  request[4] = (uint8_t)count;
  request[5] = (uint8_t)(count * 2);
  for (uint16_t i = 0; i < count; i++) {
    putBE16(data[i], &request[6 + (i * 2)]);
  }
  requestLen = (uint8_t)(6 + (count * 2));

  for (uint8_t attempt = 0; attempt < BMV080_I2C_VALIDATE_RETRY; attempt++) {
    if (!transferShortFrame(request, requestLen, response, sizeof(response))) {
      continue;
    }
    if ((response[0] != BMV080_I2C_SHORT_HEADER) || (response[1] == (BMV080_I2C_FUNC_WRITE_MULTI_HOLDING | 0x80))) {
      ret = (response[0] == BMV080_I2C_SHORT_HEADER) ? response[2] : ERR_DATA_READ;
      // Valid exception frame, return directly.
      if ((response[0] == BMV080_I2C_SHORT_HEADER) && (response[1] == (BMV080_I2C_FUNC_WRITE_MULTI_HOLDING | 0x80))) {
        return ret;
      }
      delay(3);
      continue;
    }
    if ((response[1] != BMV080_I2C_FUNC_WRITE_MULTI_HOLDING) || (getBE16(&response[2]) != reg) || (getBE16(&response[4]) != count)) {
      ret = ERR_DATA_READ;
      delay(3);
      continue;
    }
    return RET_CODE_OK;
  }

  return ret;
}

bool DFRobot_BMV080_Gravity_I2C::transferShortFrame(const uint8_t *request, uint8_t requestLen, uint8_t *response, uint8_t responseLen)
{
  if ((_pWire == NULL) || (request == NULL) || (response == NULL) || (requestLen == 0) || (responseLen == 0) || (requestLen > BMV080_I2C_MAX_FRAME_LEN) || (responseLen > BMV080_I2C_MAX_FRAME_LEN)) {
    return false;
  }

  for (uint8_t retry = 0; retry < BMV080_I2C_SHORTFRAME_RETRY; retry++) {
    uint8_t  received = 0;
    uint32_t start    = millis();
    bool     ok       = true;

    while (_pWire->available()) {
      (void)_pWire->read();
    }

    _pWire->beginTransmission(_i2cAddr);
    if (_pWire->write(request, requestLen) != requestLen) {
      delay(2);
      continue;
    }
    if (_pWire->endTransmission() != 0) {
      delay(2);
      continue;
    }

    // Give ESP32 slave task enough time to parse the request and fill TX buffer.
    delay(BMV080_I2C_SLAVE_SETTLE_MS);

    // Poll until full response arrives. This avoids false failures when ESP32 is busy
    // persisting config and short-frame response is delayed.
    while ((millis() - start) <= _timeout) {
      while (_pWire->available()) {
        (void)_pWire->read();
      }
      received = (uint8_t)_pWire->requestFrom((uint8_t)_i2cAddr, responseLen);
      if (received == responseLen) {
        break;
      }
      if (received >= 3) {
        uint8_t i = 0;
        for (i = 0; (i < received) && _pWire->available() && (i < responseLen); i++) {
          response[i] = (uint8_t)_pWire->read();
        }
        if ((response[0] == BMV080_I2C_SHORT_HEADER) && (response[1] == (request[1] | 0x80))) {
          return true;
        }
      }
      delay(2);
    }
    if (received != responseLen) {
      delay(2);
      continue;
    }

    for (uint8_t i = 0; i < responseLen; i++) {
      while (!_pWire->available()) {
        if ((millis() - start) > _timeout) {
          ok = false;
          break;
        }
        delay(1);
      }
      if (!ok || !_pWire->available()) {
        ok = false;
        break;
      }
      response[i] = (uint8_t)_pWire->read();
    }

    if (ok) {
      return true;
    }
  }

  return false;
}

#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
DFRobot_BMV080_Gravity_UART::DFRobot_BMV080_Gravity_UART(SoftwareSerial *sSerial, uint32_t baud, uint8_t addr) : DFRobot_RTU(sSerial), _serial(sSerial), _baud(baud), _addr(addr), _rxpin(0), _txpin(0) {}
#else
DFRobot_BMV080_Gravity_UART::DFRobot_BMV080_Gravity_UART(HardwareSerial *hSerial, uint32_t baud, uint8_t addr, uint8_t rxpin, uint8_t txpin) : DFRobot_RTU(hSerial), _serial(hSerial), _baud(baud), _addr(addr), _rxpin(rxpin), _txpin(txpin) {}
#endif

DFRobot_BMV080_Gravity_UART::~DFRobot_BMV080_Gravity_UART(void) {}

bool DFRobot_BMV080_Gravity_UART::begin(void)
{
  if (_serial == NULL) {
    return false;
  }
#ifdef ESP32
  _serial->begin(_baud, SERIAL_8N1, _rxpin, _txpin);
  delay(100);
#elif defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  _serial->begin(_baud);
  delay(100);
#else
  _serial->begin(_baud);
#endif
  return DFRobot_BMV080_Gravity::begin();
}

uint8_t DFRobot_BMV080_Gravity_UART::writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count)
{
  uint8_t ret = 0;
  if ((data == NULL) || (count == 0)) {
    return RET_CODE_ERROR;
  }
  if (count == 1) {
    ret = DFRobot_RTU::writeHoldingRegister(_addr, reg, data[0]);
  } else {
    ret = DFRobot_RTU::writeHoldingRegister(_addr, reg, (uint16_t *)data, count);
  }
  return ret;
}

uint8_t DFRobot_BMV080_Gravity_UART::readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count)
{
  uint8_t ret = 0;
  if ((data == NULL) || (count == 0)) {
    return RET_CODE_ERROR;
  }
  ret = DFRobot_RTU::readHoldingRegister(_addr, reg, data, count);
  return ret;
}

uint8_t DFRobot_BMV080_Gravity_UART::readInputReg(uint16_t reg, uint16_t *data, uint16_t count)
{
  uint8_t ret = 0;
  if ((data == NULL) || (count == 0)) {
    return RET_CODE_ERROR;
  }
  ret = DFRobot_RTU::readInputRegister(_addr, reg, data, count);
  return ret;
}
