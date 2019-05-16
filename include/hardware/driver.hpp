/**
 * driver.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_DRIVER_HPP_
#define HARDWARE_DRIVER_HPP_

#include <core/types.hpp>

namespace cassio {
namespace hardware {

// Offset to add to PIC interrupts to avoid conflict with software interrupts.
constexpr u8 IRQ_OFFSET = 0x20;

/**
 * @brief Defines the interrupt number for the various types of devices.
 * 
 * Enumerates the various types of device drivers, including those that are
 * related to the PIC (Programmable Interface Controller).
 * 
 * @see https://en.wikipedia.org/wiki/Interrupt_request_(PC_architecture)
 * 
 */
enum class DriverType : u8 {
    SystemTimer                                     = 0x00 + IRQ_OFFSET,
    KeyboardController                              = 0x01 + IRQ_OFFSET,
    SerialPort2                                     = 0x03 + IRQ_OFFSET,
    SerialPort1                                     = 0x04 + IRQ_OFFSET,
    ParallelPort2                                   = 0x05 + IRQ_OFFSET,
    FloppyController                                = 0x06 + IRQ_OFFSET,
    ParallelPort1                                   = 0x07 + IRQ_OFFSET,
    RealTimeClock                                   = 0x08 + IRQ_OFFSET,
    AdvancedConfigurationPowerInterface             = 0x09 + IRQ_OFFSET,
    MouseController                                 = 0x0C + IRQ_OFFSET,
    CoProcessor                                     = 0x0D + IRQ_OFFSET,
    PrimaryAdvancedTechnologyAttachment             = 0x0E + IRQ_OFFSET,
    SecondaryAdvancedTechnologyAttachment           = 0x0F + IRQ_OFFSET
};

/**
 * @brief
 *  
 */
class Driver {
protected:
    u8 number;

protected:
    /**
     * @brief
     * 
     */
    Driver(DriverType type);

    /**
     * @brief
     * 
     */
    ~Driver();

public:
    /**
     * @brief
     * 
     */
    virtual void activate();

    /**
     * @brief
     * 
     */
    virtual void deactivate();

    /**
     * @brief Resets the device, so that the state becomes known.
     * 
     * Useful for removing hardware from an unknown state, that might have been left
     * by another operating system or bootloader.
     * 
     */
    virtual i32 reset();

    /**
     * @brief
     * 
     */
    virtual u32 handleInterrupt(u32 esp);

    /** Deleted Methods */
    Driver(const Driver&) = delete;
    Driver(Driver&&) = delete;
    Driver& operator=(const Driver&) = delete;
    Driver& operator=(Driver&&) = delete;

};

// Number of drivers to support in the Driver Manager.
constexpr u32 NUM_DRIVERS = 256;

/**
 * @brief
 * 
 */
class DriverManager final {
private:
    Driver* drivers[NUM_DRIVERS];
    i32 size;

    static DriverManager instance;

private:
    /**
     * @brief
     * 
     */
    DriverManager();

    /**
     * @brief
     * 
     */
    ~DriverManager() = default;

public:
    /**
     * @brief
     * 
     */
    inline static DriverManager& getManager() {
        return instance;
    }

    /**
     * @brief
     * 
     */
    void addDriver(Driver& drv);

    /**
     * @brief
     * 
     */
    void load();

    /**
     * @brief
     * 
     */
    void unload();

    /** Deleted Methods */
    DriverManager(const DriverManager&) = delete;
    DriverManager(DriverManager&&) = delete;
    DriverManager& operator=(const DriverManager&) = delete;
    DriverManager& operator=(DriverManager&&) = delete;

};

}
}

#endif // HARDWARE_DRIVER_HPP_
