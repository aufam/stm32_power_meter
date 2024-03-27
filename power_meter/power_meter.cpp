#include "power_meter/power_meter.h"
#include "etl/keywords.h"


fun PowerMeter::fetchAll() -> etl::Future<PowerMeterValues> {
    return [this] (etl::Time timeout) mutable -> etl::Result<PowerMeterValues, osStatus_t> {
        return this->ReadInputRegisters(REG_VOLTAGE, REG_TOTAL).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { 
                PowerMeterValues values;
                values.voltage      = (res[REG_VOLTAGE]) * .1f;
                values.current      = (res[REG_CURRENT_L] | res[REG_CURRENT_H] << 16) * .001f; 
                values.power        = (res[REG_POWER_L] | res[REG_POWER_H] << 16) * .1f; 
                values.energy       = (res[REG_ENERGY_L] | res[REG_ENERGY_H] << 16) * .1f; 
                values.frequency    = (res[REG_FREQUENCY]) * .1f;
                values.powerFactor  = (res[REG_PF]) * .01f;
                values.alarm        = (res[REG_ALARM]) == 0xFFFF;
                return values;
             });
    };
}

fun PowerMeter::getVoltage() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_VOLTAGE, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return res[0] * .1f; });
    };
}

fun PowerMeter::getCurrent() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_CURRENT_L, 2).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return (res[0] | res[1] << 16) * .001f; });
    };
}

fun PowerMeter::getPower() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_POWER_L, 2).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return (res[0] | res[1] << 16) * .1f; });
    };
}

fun PowerMeter::getEnergy() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_ENERGY_L, 2).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return (res[0] | res[1] << 16) * 1.f; });
    };
}

fun PowerMeter::getFrequency() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_FREQUENCY, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return res[0] * .1f; });
    };
}

fun PowerMeter::getPowerFactor() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_PF, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return res[0] * .01f; });
    };
}

fun PowerMeter::getAlarm() -> etl::Future<bool> {
    return [this] (etl::Time timeout) mutable -> etl::Result<bool, osStatus_t> {
        return this->ReadInputRegisters(REG_ALARM, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return res[0] == 0xFFFF; });
    };
}

fun PowerMeter::getPowerThreshold() -> etl::Future<float> {
    return [this] (etl::Time timeout) mutable -> etl::Result<float, osStatus_t> {
        return this->ReadInputRegisters(REG_ALARM_THRESHOLD, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) { return float(res[0]); });
    };
}

fun PowerMeter::setPowerThreshold(float power) -> etl::Future<void> {
    return [this, power] (etl::Time timeout) mutable -> etl::Result<void, osStatus_t> {
        if (power < 0.f) return etl::Err(osError);
        if (power > 25000.f) power = 25000.f;
        
        return this->WriteSingleRegister(REG_ALARM_THRESHOLD, (uint16_t) power).wait(timeout);
    };
}

fun PowerMeter::getDeviceAddress() -> etl::Future<uint8_t> {
    return [this] (etl::Time timeout) -> etl::Result<uint8_t, osStatus_t> {
        return this->ReadHoldingRegisters(REG_DEVICE_ADDRESS, 1).wait(timeout)
            .then([] (etl::Vector<uint16_t> res) -> uint8_t { return res[0]; });
    };
}

fun PowerMeter::setDeviceAddress(uint8_t newAddress) -> etl::Future<void> {
    return [this, newAddress] (etl::Time timeout) -> etl::Result<void, osStatus_t> {
        if (newAddress < 0x01 || newAddress > 0xF7) 
            return etl::Err(osError);
        
        return this->WriteSingleRegister(REG_DEVICE_ADDRESS, newAddress).wait(timeout)
            .then([this] (uint16_t res) { this->server_address = res; });
    };
}

fun PowerMeter::resetEnergy() -> etl::Future<void> {
    return [this] (etl::Time timeout) -> etl::Result<void, osStatus_t> {
        uint8_t buffer[2] = { uint8_t(server_address), CMD_RESET_ENERGY };
        return this->request(buffer, sizeof(buffer)).wait(timeout);
    };
}

fun PowerMeter::calibrate() -> etl::Future<void> {
    return [this] (etl::Time timeout) -> etl::Result<void, osStatus_t> {
        uint8_t buffer[4] = { uint8_t(server_address), CMD_CALIBRATION, 0x37, 0x21 };
        return this->request(buffer, sizeof(buffer)).wait(timeout);
    };
}
