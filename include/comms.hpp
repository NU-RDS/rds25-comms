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

namespace comms {

struct CommsTickResult {
    RawCommsMessage rawMessage;
    MessageInfo info;
};

class CommsController {
   public:
    CommsController(CommsDriver& driver, MCUID id);
    void initialize();

    // high-level controls
    void sendCommand(CommandMessagePayload payload);
    Option<float> getSensorValue(MCUID sender, uint8_t sensorID);

    // low-level controls
    void addSensor(uint32_t updateRateMs, uint8_t sensorID, std::shared_ptr<Sensor> sensor);

    Option<CommsTickResult> tick();
    MCUID me() const;

   private:
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

    std::vector<SensorStatus> _sensorStatuses;

    MCUID _me;
};

}  // namespace comms

#endif  // __COMMS_H__
