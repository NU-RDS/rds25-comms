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

    // enable heartbeats
    g_controller.enableHeartbeatRequestDispatching(100,                      // how often?
                                                   {MCUID::MCU_LOW_LEVEL_0}  // who to monitor?
    );

    MotorControlCommandOpt commandDesc(MCUID::MCU_LOW_LEVEL_0,               // who is recieving it?
                                       0,                                    // what motor?
                                       MotorControlCommandType::MC_CMD_POS,  // the control type
                                       10                                    // the value to control
    );

    CommandMessagePayload motorCmd = CommandBuilder::motorControl(g_controller.me(), commandDesc);

    g_controller.sendCommand(motorCmd);

    // print out the data recieved by the sensor
    Option<float> sensorValueOpt =
        g_controller.getSensorValue(MCUID::MCU_LOW_LEVEL_0,  // who is sending the sensor data?
                                    0                        // what sensor do we want?
        );

    if (sensorValueOpt.isNone()) {
        Serial.println("No sensor value yet!");
    } else {
        Serial.printf("%0.2f\n", sensorValueOpt.value());
    }

    delay(100);  // bad
}

}  // namespace tx

#endif  // __TX_EXAMPLE_H__