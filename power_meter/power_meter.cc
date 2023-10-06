#include "power_meter/power_meter.h"
#include "etl/keywords.h"

using namespace Project;

static const uint16_t crcTable[] = {
        0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
        0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
        0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
        0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
        0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
        0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
        0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
        0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
        0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
        0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
        0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
        0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
        0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
        0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
        0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
        0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
        0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
        0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
        0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
        0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
        0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
        0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
        0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
        0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
        0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
        0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
        0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
        0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
        0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
        0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
        0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
        0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

fun PowerMeter::init() -> void {
    uart.setBaudRate(9600);
    uart.init(); 
    uart.rxCallbackList.push(etl::bind<&PowerMeter::rxCallback>(this));
    notifier.init();
    timer.init({
        .function={
            lambda (PowerMeter* self) {
                val bufferSend = makeBufferSend(self->address, CMD_READ_INPUT_REGISTER, REG_VOLTAGE, REG_TOTAL);
                memcpy(self->uart.rxBuffer.data(), bufferSend.data(), bufferSend.len());
                self->uart.transmit(self->uart.rxBuffer.data(), bufferSend.len());
            }, this, 
        },
        .interval=timeout
    });
}

fun PowerMeter::deinit() -> void {
    uart.deinit();
    notifier.deinit();
}

fun PowerMeter::getAlarmThreshold() -> float {
    val bufferSend = makeBufferSend(address, CMD_READ_HOLDING_REGISTER, REG_ALARM_THRESHOLD, 1);
    uart.transmit(bufferSend.data(), bufferSend.len());
    return notifier.waitFlags({.flags=FLAG_GETTER, .option=osFlagsWaitAny, .timeout=timeout}) == FLAG_GETTER ? 
        (float) decode(uart.rxBuffer.data(), 0) : NAN;
}

fun PowerMeter::getDeviceAddress() -> uint8_t {
    val bufferSend = makeBufferSend(address, CMD_READ_HOLDING_REGISTER, REG_DEVICE_ADDRESS, 1);
    uart.transmit(bufferSend.data(), bufferSend.len());

    if (notifier.waitFlags({.flags=FLAG_GETTER, .option=osFlagsWaitAny, .timeout=timeout}) != FLAG_GETTER) 
        return address; /// return if timeout

    address = decode(uart.rxBuffer.data(), 0);
    return address;
}

fun PowerMeter::setAlarmThreshold(float power) -> bool {
    if (power < 0.f) return false;
    if (power > 25000.f) power = 25000.f;
    val bufferSend = makeBufferSend(address, CMD_WRITE_SINGLE_REGISTER, REG_ALARM_THRESHOLD, (uint16_t) power);
    uart.transmit(bufferSend.data(), bufferSend.len());
    return notifier.waitFlags({.flags=FLAG_SETTER, .option=osFlagsWaitAny, .timeout=timeout}) == FLAG_SETTER;
}

fun PowerMeter::setDeviceAddress(uint8_t newAddress) -> bool {
    if (newAddress < 0x01 || newAddress > 0xF7) return false;
    val bufferSend = makeBufferSend(address, CMD_WRITE_SINGLE_REGISTER, REG_ALARM_THRESHOLD, newAddress);
    uart.transmit(bufferSend.data(), bufferSend.len());
    return notifier.waitFlags({.flags=FLAG_SETTER, .option=osFlagsWaitAny, .timeout=timeout}) == FLAG_SETTER;
}

fun PowerMeter::resetEnergy() -> bool {
    uint8_t bufferSend[4] = { address, CMD_RESET_ENERGY };
    auto checksum = crc(bufferSend, 2);
    bufferSend[2] = (checksum >> 8) & 0xFF;
    bufferSend[3] = (checksum >> 0) & 0xFF;
    uart.transmit(bufferSend, sizeof (bufferSend));
    return notifier.waitFlags({.flags=FLAG_SETTER, .option=osFlagsWaitAny, .timeout=timeout}) == FLAG_SETTER;
}

fun PowerMeter::calibrate() -> bool {
    uint8_t bufferSend[6] = { 0xF8, CMD_CALIBRATION, 0x37, 0x21 };
    auto checksum = crc(bufferSend, 2);
    bufferSend[4] = (checksum >> 8) & 0xFF;
    bufferSend[5] = (checksum >> 0) & 0xFF;
    uart.transmit(bufferSend, sizeof (bufferSend));
    return notifier.waitFlags({.flags=FLAG_SETTER, .option=osFlagsWaitAny, .timeout=timeout}) == FLAG_SETTER;
}

fun PowerMeter::makeBufferSend(uint8_t address, uint8_t cmd, uint16_t registerAddress, uint16_t nRegister) -> PowerMeter::BufferSend {
    BufferSend buf;
    buf[0] = address;
    buf[1] = cmd;
    buf[2] = (registerAddress >> 8) & 0xFF;
    buf[3] = (registerAddress >> 0) & 0xFF;
    buf[4] = (nRegister >> 8) & 0xFF;
    buf[5] = (nRegister >> 0) & 0xFF;

    auto checksum = crc(buf.data(), 6);
    buf[6] = (checksum >> 8) & 0xFF;
    buf[7] = (checksum >> 0) & 0xFF;
    return buf;
}

fun PowerMeter::crc(const uint8_t *data, size_t len) -> uint16_t {
    uint16_t res = 0xFFFF;
    for (val i in etl::range(len)) {
        uint8_t temp = data[i] ^ res;
        res >>= 8;
        res ^= crcTable[temp];
    }
    return res;
}

fun PowerMeter::decode(const uint8_t* buf, uint32_t reg) -> uint32_t {
    val &data = etl::array_cast<uint8_t, 2>(&buf[START_BYTES + reg * 2]);
    return etl::byte_array_cast_back_le<uint16_t>(data); 
}

fun PowerMeter::decode(const uint8_t* buf, PowerMeterValues& values) -> void {
    var safe_multiply = etl::safe_mul<float, uint32_t, float>;
    values.voltage      = safe_multiply(decode(buf, REG_VOLTAGE), .1f);
    values.current      = safe_multiply(decode(buf, REG_CURRENT_L) | decode(buf, REG_CURRENT_H) << 16, .001f);
    values.power        = safe_multiply(decode(buf, REG_POWER_L)   | decode(buf, REG_POWER_H)   << 16, .1f);
    values.energy       = safe_multiply(decode(buf, REG_ENERGY_L)  | decode(buf, REG_ENERGY_H)  << 16, 1.f);
    values.frequency    = safe_multiply(decode(buf, REG_FREQUENCY), .1f);
    values.powerFactor  = safe_multiply(decode(buf, REG_PF), .01f);
    values.alarm        = decode(buf, REG_ALARM) == 0xFFFF;
}

fun PowerMeter::rxCallback(const uint8_t* buf, size_t len) -> void {
    val values = START_BYTES + (REG_TOTAL * 2) + STOP_BYTES;
    val getter = START_BYTES + 2 + STOP_BYTES;
    val setter = BufferSend::size();
    val reset  = 4;
    val calib  = 6;

    if (len == values)
        decode(buf, values);
    elif (len == getter) 
        notifier | FLAG_GETTER;
    elif (len == setter) 
        notifier | FLAG_SETTER;
    elif (len == reset)  
        notifier | FLAG_SETTER;
    elif (len == calib)  
        notifier | FLAG_SETTER;
}

