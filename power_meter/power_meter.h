#ifndef PROJECT_POWER_METER_H
#define PROJECT_POWER_METER_H

#include "periph/uart.h"
#include "etl/event.h"
#include "etl/timer.h"
#include "etl/array.h"
#include "etl/numerics.h"
#include "etl/getter_setter.h"
#include "etl/function.h"

namespace Project {

    struct PowerMeterValues {
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
    class PowerMeter {
        template <typename T>
        using GetterSetter = etl::GetterSetter<T, etl::Function<T(), PowerMeter*>, etl::Function<bool(T), PowerMeter*>>;

        etl::Event notifier;
        etl::Timer timer;
        uint8_t address;
        PowerMeterValues& values;
    
    public:
        periph::UART &uart;
        
        using BufferSend = etl::Array<uint8_t, 8>;
        inline static constexpr etl::Time timeout = etl::time::seconds(1);
        inline static constexpr uint8_t defaultAddress = 0xF8;

        constexpr PowerMeter(PowerMeterValues& values, periph::UART &uart)
        : address(defaultAddress)
        , values(values)
        , uart(uart) {}

        /// init uart and notifier
        void init();

        /// deinit uart and notifier
        void deinit();

        /// get voltage
        const float& voltage = values.voltage;

        /// get current
        const float& current = values.current;

        /// get power
        const float& power = values.power;

        /// get energy
        const float& energy = values.energy;

        /// get frequency
        const float& frequency = values.frequency;

        /// get power factor
        const float& powerFactor = values.powerFactor;

        /// get alarm
        const bool& alarm = values.alarm;

        /// get and set power threshold
        GetterSetter<float> powerThreshold = {
            etl::bind<&PowerMeter::getAlarmThreshold>(this),
            etl::bind<&PowerMeter::setAlarmThreshold>(this)
        };

        /// get and set device address
        GetterSetter<uint8_t> deviceAddress = {
            etl::bind<&PowerMeter::getDeviceAddress>(this),
            etl::bind<&PowerMeter::setDeviceAddress>(this)
        };

        bool resetEnergy();

        bool calibrate();

    private:

        float getAlarmThreshold();

        uint8_t getDeviceAddress();

        bool setAlarmThreshold(float power);

        bool setDeviceAddress(uint8_t newAddress);

        static BufferSend makeBufferSend(uint8_t address,
                                         uint8_t cmd,
                                         uint16_t registerAddress,
                                         uint16_t nRegister);

        static uint16_t crc(const uint8_t *data, size_t len);

        static uint32_t decode(const uint8_t* buf, uint32_t reg);

        static void decode(const uint8_t* buf, PowerMeterValues& values);

        void rxCallback(const uint8_t* buf, size_t len);

        enum : uint8_t {
            CMD_READ_HOLDING_REGISTER   = 0x03,
            CMD_READ_INPUT_REGISTER     = 0x04,
            CMD_WRITE_SINGLE_REGISTER   = 0x06,
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
        
        enum : uint32_t {
            FLAG_GETTER = 0b010,
            FLAG_SETTER = 0b100,
        };

        enum : uint32_t {
            START_BYTES = 3,
            STOP_BYTES = 2
        };
    };

}

#endif //PROJECT_POWER_METER_H
