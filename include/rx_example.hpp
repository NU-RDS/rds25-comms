#ifndef __RX_EXAMPLE_H__
#define __RX_EXAMPLE_H__

#include "comms.hpp"

#include <memory>

using namespace comms;

namespace rx {

// bus num, baudrate
TeensyCANDriver<3, CANBaudRate::CBR_500KBPS> g_canDriver;

CommsController g_controller{
    g_canDriver,
    MCUID::MCU_LOW_LEVEL_0,  // we are low level 0
};

static bool sensorInitialize() {
    return true;
}

static float sensorRead() {
    return 10.0f;
}
static void sensorCleanup() { /* no-op */ }

void setup() {
    // add a sensor
    g_controller.addSensor(
        100,  // interval for when we update the sensor in ms
        0,    // what sensor this is
        // how to use the sensor!
        std::make_shared<LambdaSensor>(sensorInitialize, sensorRead, sensorCleanup));

    g_controller.initialize();
}

void loop() {
    g_controller.tick();
}

}  // namespace rx

#endif  // __RX_EXAMPLE_H__