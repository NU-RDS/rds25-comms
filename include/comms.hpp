// comms.hpp
#ifndef __COMMS_H__
#define __COMMS_H__

#include "impl/can_comms_driver.hpp"
#include "impl/command.hpp"
#include "impl/comms_driver.hpp"
#include "impl/debug.hpp"
#include "impl/id.hpp"
#include "impl/option.hpp"
#include "impl/sensor.hpp"
#include "impl/heartbeat.hpp"

namespace comms {

struct CommsTickResult {
    RawCommsMessage rawMessage;
    MessageInfo info;
};

struct CommandAcknowledgementInfo {
    RawCommsMessage message;
    uint32_t lastSent;
    uint8_t numRetries;
};

class CommsController {
   public:
    CommsController(CommsDriver& driver, MCUID id);
    void initialize();

    // high-level controls
    void sendCommand(CommandMessagePayload payload);
    Option<float> getSensorValue(MCUID sender, uint8_t sensorID);
    void enableHeartbeatRequestDispatching(uint32_t intervalMs, const std::vector<MCUID> toMonitor);

    // low-level controls
    void addSensor(uint32_t updateRateMs, uint8_t sensorID, std::shared_ptr<Sensor> sensor);

    Option<CommsTickResult> tick();
    MCUID me() const;

   private:
    void updateDatastreams();
    void updateHeartbeats();
    void updateCommandAcknowledgements();

    void handleCommand(MessageInfo info, RawCommsMessage message);
    void handleHeartbeat(MessageInfo info, RawCommsMessage message);
    void handleError(MessageInfo info, RawCommsMessage message);

    void handleCommandBegin(MessageInfo info, CommandMessagePayload payload);
    void handleCommandStop(MessageInfo info, CommandMessagePayload payload);
    void handleCommandMotorControl(MessageInfo info, CommandMessagePayload payload);
    void handleCommandSensorToggle(MessageInfo info, CommandMessagePayload payload);

    CommsDriver& _driver;
    CommandBuffer _cmdBuf;

    std::unordered_map<uint8_t, SensorDatastream> _sensorDatastreams;
    std::unordered_map<uint16_t, CommandAcknowledgementInfo> _unackedCommands;
    std::vector<uint16_t> _toRemoveUnackedCommands;
    std::vector<SensorStatus> _sensorStatuses;

    HeartbeatManager _heartbeatManager;

    bool _startCommandEnqueued = false;
    RawCommsMessage _startCommandMessage;

    MCUID _me;
};

}  // namespace comms

#endif  // __COMMS_H__
