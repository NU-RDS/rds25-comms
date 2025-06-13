# rds25-comms

Communication Library used for RDS2025. See [rds25's main repo](https://github.com/NU-RDS/rds25-project) for application. To fully understand the context of this library, please refer to the [rds25-project](https://github.com/NU-RDS/rds25-project) where the electrical architecture is documented.

Built to:
* Provide a simple interface for managing communication between High Level and Low Level Microcontrollers on Han(d)
* Handle the dispatching of commands and command acknowledgements between the two levels
* Handle the execution of commands on the Low Level Microcontroller
* Handle sensor data collection and transmission to the High Level Microcontroller
* Handle heartbeat and keep-alive messages to ensure the connection is alive
* Be modular and communication protocol agnostic, allowing for easy integration of different communication protocols, like CAN, EtherCat, etc.
* Be extensible, allowing for easy addition of new commands and sensors

A lot of the design decisions were made to ensure that the library is modular, extensible, and easy to use. Since it was being developed concurrently with the hardware, and the firmware for high-level, low-level, and palm controllers, it was important to ensure that the library could be easily integrated into the existing codebase, and that it could be easily extended to support new features and functionality as they were needed.

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

## Command Dispatching

### Overview

One of the main concerns with building decentralized systems is ensuring that everything is in sync and that commands are executed correctly. Given that the hand is quite complex, but also fragile, it was important to ensure that commands are dispatched correctly and that the system is robust against errors.

For example, if we wanted to move multiple joints at once, we would need to ensure that the commands are dispatched, and will be executed in a certain order. The command dispatching system allows us to do this by providing a way to send commands to the low-level microcontrollers, and then wait for an acknowledgement that the command was executed.

There are a few stages to command dispatching:
1. **Dispatching**: Sending queue of commands from the high-level to low-level microcontroller.
2. **Acknowledgement**: Waiting for a response from the microcontroller to confirm that the commands were recieved and set up for execution. Timeouts are used to ensure that the system does not hang if the command is not acknowledged.
3. **Execution**: Once all commands in the queue are acknowledged, the microcontroller can execute them in the correct order. Upon execution, the microcontroller will send an acknowledgement back to the high-level microcontroller to confirm that the command was executed successfully, including information about the execution status, like the time it took to execute the command, and any errors that may have occurred.
4. **Error Handling**: If an error occurs during execution, the microcontroller will send an error message back to the high-level microcontroller, which can then handle the error appropriately.
5. **Completion**: Once all commands are executed, the high-level microcontroller can continue with its operations, knowing that the commands were executed successfully.

There are a variety of commands that can be dispatched, but the main one is the `MotorControlCommand`, which allows the high-level microcontroller to control the motors on the low-level microcontroller. This command can be used to set the position, velocity, or torque of a motor, and can be used to control multiple motors at once.

This system helps to ensure that the hand is able to move in a coordinated manner, and that the commands are executed correctly. It also allows for easy debugging and error handling, as the high-level microcontroller can see the status of the commands and any errors that may have occurred. Each command is dispatched with a unique ID, which allows the high-level microcontroller to track the status of the command and ensure that it is executed correctly.


### Command Payloads

Command Payloads fit within a 64-bit payload, and are used to send commands to the low-level microcontroller. The payload is structured as follows:
```cpp
struct CommandMessagePayload {
    union {
        uint64_t raw;
        struct {
            // meta information (4 bytes)
            CommandType type;
            MCUID mcuID;
            uint16_t commandID;
            // last 4 bytes for command-specific things
            uint32_t payload;
        };
    };
}
```

Each command has a type, which is used to determine how the command should be executed. The `MCUID` is used to determine which low-level microcontroller the command is being sent to, and the `commandID` is used to track the status of the command. The `payload` is used to send any additional information that is needed for the command.

### Command Types
The command types are defined in the `CommandType` enum, which includes the following types:
```cpp
/// @brief The type of command being sent
enum CommandType : uint8_t {
    // General Commands, for any MCU
    CMD_BEGIN,  // begin the operation of the device
    CMD_STOP,   // end the operation of the device
    CMD_MOTOR_CONTROL,     // Motor-Driver Specific Commands
    CMD_INVALID,
    CMD_COUNT
};
```

There really only are the `CMD_BEGIN`, `CMD_STOP`, and `CMD_MOTOR_CONTROL` commands, which are used to control the motors on the low-level microcontroller. The `CMD_INVALID` command is used to indicate that the command is invalid, and the `CMD_COUNT` command is used to count the number of commands.

Each type of command has another 32 bits of payload that are used to send additional information. For example, the `CMD_MOTOR_CONTROL` command has a payload that is structured as follows:
```cpp
struct MotorControlCommandOpt {
    union {
        uint32_t payload;
        struct {
            uint8_t motorNumber;
            MotorControlCommandType controlType;
            uint16_t value;
        };
    };
};
```
This structure allows the high-level microcontroller to specify which motor to control, what type of control to apply (position, velocity, or torque), and the value to set for that control.

### Hooking Up Commands

In order to actually do something with these commands, we need to set up a way to handle them on the low-level microcontroller. This involves creating a command handler that can process the commands and execute the appropriate actions.

Within the `CommsController::CommandManager::CommandBuffer`, we can register command handlers for each command type. These are abstract classes that define the interface for handling commands of a specific type. 

This interface is as follows:
```cpp
/// @brief Handles specific commands, determines if events are parallizable, etc.
/// Used to specify "when I recieve this type of command, what should happen?"
class CommandHandler {
   public:
    virtual void start(const CommandMessagePayload& payload) {}
    virtual void update(const CommandMessagePayload& payload) {}
    virtual void end(const CommandMessagePayload& payload) {}
    virtual bool isParallelizable(const std::vector<CommandMessagePayload> slice);
};
```

This interface allows us to define how to handle commands, and whether they can be executed in parallel with other commands. The `start`, `update`, and `end` methods are used to handle the command at different stages of its execution, while the `isParallelizable` method is used to determine if the command can be executed in parallel with other commands.

We set the handler easily. Suppose we have a `CommsController` instance called `g_controller`, and we want to handle the `CMD_MOTOR_CONTROL` command. We can do this as follows:

```cpp
// some imaginary class
MotorCommandHandler g_motorCommandHandler;

g_controller.commandManager().commandBuffer().setHandler(
    CommandType::CMD_MOTOR_CONTROL,  // the command type we want to handle
    &g_motorCommandHandler            // the handler instance
);
```
This will register the `g_motorCommandHandler` instance as the handler for the `CMD_MOTOR_CONTROL` command type. The `g_motorCommandHandler` class should implement the `CommandHandler` interface, and define how to handle the command when it is received.

Listening for commands is done in the `CommsController::tick()` method, which will call the appropriate handler methods based on the command type and the current state of the command.

## Heartbeat and Keep-Alive
The heartbeat and keep-alive system is designed to ensure that the communication between the high-level and low-level microcontrollers is alive and functioning correctly. This is important for ensuring that the system is responsive and that commands are executed in a timely manner.

The high-level microcontroller periodically sends heartbeat requests to the low-level microcontrollers, which respond with a heartbeat response. This response includes a count variable that is incremented every time the low-level microcontroller responds to a heartbeat request.

This count is kept track of by the high-level microcontroller. This count is the "true" count of how many times the low-level microcontroller has responded to a heartbeat request. The high-level microcontroller expects each response from the low-level microcontroller to send back the current count of heartbeat responses.

If the low-level microcontroller doesn't respond to a heartbeat request, or if the count sent back is not what was expected, the high-level microcontroller can take appropriate action, such as shutting down the system or logging an error.

### Why the Count?

Storing the count independently on the high-level and low-level microcontrollers allows for a more robust system. There are a lot of reasons why the count might not match, such as:
- The low-level microcontroller was reset or restarted, causing it to lose its count.
- The high-level microcontroller missed a heartbeat response due to a communication error.
- The low-level microcontroller is not functioning correctly and is not sending back the correct count.
By storing the count independently, the high-level microcontroller can detect these issues and take appropriate action. It can also log the count for debugging purposes, allowing developers to see how many times the low-level microcontroller has responded to heartbeat requests.

### Enabling Heartbeats

To enable heartbeats, you can use the `enableHeartbeatRequestDispatching` method on the `CommsController` instance. This method takes two parameters:
```cpp
void CommsController::enableHeartbeatRequestDispatching(
    uint32_t intervalMs,  // how often to send heartbeats
    std::vector<MCUID> targets  // who to monitor?
);
```
This will enable the heartbeat system, and the high-level microcontroller will start sending heartbeat requests to the specified targets at the specified interval. The low-level microcontrollers will respond with heartbeat responses, which will be processed by the `CommsController`.

## Sensor Data Collection and Transmission

The sensor data collection and tranmission system was meant to make it as easy as possible to add new sensors to the system, and to ensure that the data is collected and transmitted in a timely manner. The system is designed to be modular and extensible, allowing for easy addition of new sensors and data types.

Each low-level and palm microntroller can have any amount of sensors, each with their own unique ID per MCU. 

This system allows us to use the naming convention for sensors:

- `Palm.<sensor_id>` for palm sensors. For example, `Palm.0` for the splay sensor on the dex finger, `Palm.1` for the splay sensor on the index finger, etc.
- `LowLevel.<mcu_id>.<sensor_id>` for low-level sensors. For example, `LowLevel.0.0` for the motor encoder on the first low-level microcontroller, `LowLevel.1.0` for the motor encoder on the second low-level microcontroller, etc.

Sending the data is done through the CommsController, which will handle the collection and transmission of the data to the high-level microcontroller. The data is collected in a continuous stream, and the `SensorDatastream` class manages the flow of that data.

### Adding Sensors
To add a sensor to the `SensorDatastream`, you can use the `addSensor` method on the `CommsController` instance. This method takes three parameters:
```cpp
void CommsController::addSensor(
    uint32_t intervalMs,  // how often to collect data
    uint8_t sensorID,     // the ID of the sensor
    std::shared_ptr<Sensor> sensor  // the sensor instance
);
```

This will add the sensor to the `SensorDatastream`, and the data will be collected at the specified interval. The `Sensor` class is an abstract class that defines the interface for collecting data from a sensor. You can create your own sensor classes by inheriting from this class and implementing the `initialize`, `read`, and `cleanup` methods.

### Sensor Data Collection
The `SensorDatastream` class is responsible for collecting data from all sensors and transmitting that data to the high-level microcontroller. The data is collected in a continuous stream, and the `SensorDatastream` class manages the flow of that data.

You can retrieve the sensor data from the `CommsController` using the `getSensorValue` method, which takes two parameters:
```cpp
Option<float> CommsController::getSensorValue(
    MCUID mcuID,  // who is sending the sensor data?
    uint8_t sensorID  // what sensor do we want?
);
```

This will send the most recently collected sensor data for the specified sensor ID from the specified MCU. If the sensor data is not available, it will return an none option.

## Error Handling

The error handling system is the least developed part of the library, but it is designed to handle errors that occur during command execution and sensor data collection. The system is designed to be modular and extensible, allowing for easy addition of new error types and handling mechanisms.

The general idea is that errors have different types, severities, and behaviors. For example:

* An error could be critical, requiring a shutdown of the system (thus is severe), have an error code of 10 indicating a specific error, and have a behavior is that it should be never be unlatched. Only power cycling the system will clear this error.
* An error could be non-critical, meaning that the system can continue to operate, but it should be logged and reported to the user. This could have an error code of 20, and a behavior of being unlatched after a certain period of time or after a certain number of occurrences.
* An error could be informational, meaning that it is not an error in the traditional sense, but it is something that the user should be aware of. This could have an error code of 30, and a behavior of being unlatched after a certain period of time or after a certain number of occurrences.

The error handling system is designed to be modular and extensible, allowing for easy addition of new error types and handling mechanisms. The `ErrorManager` class is responsible for managing errors, and it provides methods for adding, removing, and checking errors.

