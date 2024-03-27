#ifndef PROJECT_POWER_METER_H
#define PROJECT_POWER_METER_H

#include "modbus/rtu/client.h"
#include "etl/getter_setter.h"
#include <cmath>

namespace Project { struct PowerMeterValues; class PowerMeter; }

struct Project::PowerMeterValues {
    float voltage = NAN;    ///< in volt
    float current = NAN;    ///< in ampere
    float power = NAN;      ///< in watt
    float energy = NAN;     ///< in Wh
    float frequency = NAN;  ///< in Hz
    float powerFactor = NAN;///< power factor
    bool alarm = false;     ///< indicates power is greater than power threshold
};

/// power meter using PZEM-004t. UART 9600.
/// see https://innovatorsguru.com/wp-content/uploads/2019/06/PZEM-004T-V3.0-Datasheet-User-Manual.pdf
class Project::PowerMeter : public modbus::rtu::Client {
    template <typename T>
    using GetterSetterFuture = etl::GetterSetter<etl::Future<T>, 
        etl::Function<etl::Future<T>(), PowerMeter*>, 
        etl::Function<etl::Future<void>(T), PowerMeter*>
    >;

    template <typename T>
    using GetterFuture = etl::Getter<etl::Future<T>, 
        etl::Function<etl::Future<T>(), PowerMeter*>
    >;

public:
    inline static constexpr int defaultAddress = 0xF8;

    using modbus::rtu::Client::Client;

    /// get voltage
    const GetterFuture<float> voltage = {
        .get=etl::bind<&PowerMeter::getVoltage>(this),
    };

    /// get current
    const GetterFuture<float> current = {
        .get=etl::bind<&PowerMeter::getCurrent>(this),
    };

    /// get power
    const GetterFuture<float> power = {
        .get=etl::bind<&PowerMeter::getPower>(this),
    };

    /// get energy
    const GetterFuture<float> energy = {
        .get=etl::bind<&PowerMeter::getEnergy>(this),
    };

    /// get frequency
    const GetterFuture<float> frequency = {
        .get=etl::bind<&PowerMeter::getFrequency>(this),
    };

    /// get power factor
    const GetterFuture<float> powerFactor = {
        .get=etl::bind<&PowerMeter::getPowerFactor>(this),
    };

    /// get alarm
    const GetterFuture<bool> alarm = {
        .get=etl::bind<&PowerMeter::getAlarm>(this),
    };

    /// get and set power threshold
    const GetterSetterFuture<float> powerThreshold = {
        .get=etl::bind<&PowerMeter::getPowerThreshold>(this),
        .set=etl::bind<&PowerMeter::setPowerThreshold>(this)
    };

    /// get and set device address
    const GetterSetterFuture<uint8_t> deviceAddress = {
        .get=etl::bind<&PowerMeter::getDeviceAddress>(this),
        .set=etl::bind<&PowerMeter::setDeviceAddress>(this)
    };

    etl::Future<PowerMeterValues> fetchAll();

    etl::Future<void> resetEnergy();
    etl::Future<void> calibrate();

private:
    etl::Future<float> getVoltage();
    etl::Future<float> getCurrent();
    etl::Future<float> getPower();
    etl::Future<float> getEnergy();
    etl::Future<float> getFrequency();
    etl::Future<float> getPowerFactor();
    etl::Future<bool> getAlarm();

    etl::Future<float> getPowerThreshold();
    etl::Future<void> setPowerThreshold(float power);

    etl::Future<uint8_t> getDeviceAddress();
    etl::Future<void> setDeviceAddress(uint8_t newAddress);

    enum : uint8_t {
        CMD_CALIBRATION             = 0x41,
        CMD_RESET_ENERGY            = 0x42,
    };
    
    enum : uint16_t {
        REG_VOLTAGE     = 0x0000,
        REG_CURRENT_L   = 0x0001,
        REG_CURRENT_H   = 0X0002,
        REG_POWER_L     = 0x0003,
        REG_POWER_H     = 0x0004,
        REG_ENERGY_L    = 0x0005,
        REG_ENERGY_H    = 0x0006,
        REG_FREQUENCY   = 0x0007,
        REG_PF          = 0x0008,
        REG_ALARM       = 0x0009,
        REG_TOTAL       = 0x000A,

        REG_ALARM_THRESHOLD = 0x0001,
        REG_DEVICE_ADDRESS  = 0x0002,
    };
};

#endif //PROJECT_POWER_METER_H
