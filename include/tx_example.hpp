#ifndef __TX_EXAMPLE_H__
#define __TX_EXAMPLE_H__

#include <Arduino.h>
#include "comms.hpp"

using namespace comms;

namespace tx {

// bus num, baudrate
TeensyCANDriver<2, CANBaudRate::CBR_500KBPS> g_canDriver;

CommsController g_controller{
    g_canDriver,
    MCUID::MCU_HIGH_LEVEL  // we are the high level
};

void setup() {
    Serial.begin(9600);
    Serial.println("TX Example Start!");
    g_controller.initialize();
}

void loop() {
    Serial.println("Loop!");
    g_controller.tick();

    MotorControlCommand commandDesc(
        MCUID::MCU_LOW_LEVEL_0,               // who is recieving it?
        0,                                    // what motor?
        MotorControlCommandType::MC_CMD_POS,  // the control type
        10                                    // the value to control
    );

    CommandMessagePayload command = CommandBuilder::motorControl(g_controller.me(), commandDesc);

    g_controller.sendCommand(command);

    delay(100);  // bad
}

}  // namespace tx

#endif  // __TX_EXAMPLE_H__