#ifndef __RX_EXAMPLE_H__
#define __RX_EXAMPLE_H__

#include "comms.hpp"

using namespace comms;

namespace rx {

// bus num, baudrate
TeensyCANDriver<2, CANBaudRate::CBR_500KBPS> g_canDriver;

CommsController g_controller{
    g_canDriver,
    MCUID::MCU_HIGH_LEVEL  // we are the high level
};

void setup() {
    g_controller.initialize();
}

void loop() {
    g_controller.tick();
}

}  // namespace rx

#endif  // __RX_EXAMPLE_H__