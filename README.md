# rds25-comms

Communication Library used for RDS2025. See [rds25's main repo](https://github.com/NU-RDS/rds25-project) for application.

Built to:
* Provide a simple interface for managing communication between High Level and Low Level Microcontrollers on Han(d)
* Handle the dispatching of commands and command acknowledgements between the two levels
* Handle the execution of commands on the Low Level Microcontroller
* Handle sensor data collection and transmission to the High Level Microcontroller
* Handle heartbeat and keep-alive messages to ensure the connection is alive
* Be modular and communication protocol agnostic, allowing for easy integration of different communication protocols, like CAN, EtherCat, etc.
* Be extensible, allowing for easy addition of new commands and sensors


# Installation
To use in a PlatformIO project, add the following to your `platformio.ini` file:

```ini
lib_deps =
    https://github.com/NU-RDS/rds25-comms.git
```

# Modules/Concepts

There are a few key modules and concepts in this library:
* `CommsDriver`: A hardware abstraction layer for the communication interfaces. Used to send and receive messages.
* `RawCommsMessage`: A class representing a raw message that can be sent or received. Each message has an id, a length, and a 64-bit payload.
* `CommsController`: The main controller for the communication library. It manages sensor data (both sending and receiving), command dispatching, heartbeat messages, and the overall communication flow.
* `MessageInfo`: A class representing the metadata of a message that is not explictly part of the message payload. This includes the sender, target, and content type. This is derived solely from the message id as a part of the specification.
* `HeartbeatManager`: A subclass of `CommsController` that handles heartbeat messages and keep-alive functionality.
* `CommandManager`: A subclass of `CommsController` that handles command dispatching and execution.
* `ErrorManager`: A subclass of `CommsController` that handles error messages and error codes.
* `SensorDatastream`: Part of the sensor data collection and transmission system. It manages the collection of sensor data and the transmission of that data to the High Level Microcontroller. Sensor data is collected/sent in a continuous stream, and the `SensorDatastream` class manages the flow of that data.
* `Sensor`: A class representing a sensor that can be added to the `SensorDatastream`. Each sensor has a unique id and a function to collect data. The `SensorDatastream` class manages the collection of data from all sensors and the transmission of that data to the High Level Microcontroller.


# Usage

## High Level Microcontroller Example
```cpp
#include <Arduino.h>

#include "comms.hpp"

using namespace comms;

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
```

This example demonstrates how to use the communication library in a high-level microcontroller application. The code initializes the communication controller, sends a motor control command, and retrieves sensor data from a low-level microcontroller. The use of namespaces and modular design allows for easy integration and extension of the library for different use cases and communication protocols.


## Low Level Microcontroller Example
```cpp
#include <Arduino.h>
#include "comms.hpp"

// bus num, baudrate
TeensyCANDriver<2, CANBaudRate::CBR_500KBPS> g_canDriver;

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
```

This example demonstrates how to use the communication library in a low-level microcontroller application. The code initializes the communication controller, adds a sensor, and continuously reads sensor data.


