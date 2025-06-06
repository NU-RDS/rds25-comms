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
#include "impl/error.hpp"

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
    void enableHeartbeatRequestDispatching(uint32_t intervalMs, const std::vector<MCUID> toMonitor);

    // low-level controls
    void addSensor(uint32_t updateRateMs, uint8_t sensorID, std::shared_ptr<Sensor> sensor);

    // general controls
    void reportError(ErrorCode error, ErrorSeverity severity, ErrorBehavior behavior);
    void clearError(ErrorCode error);

    Option<CommsTickResult> tick();
    MCUID me() const;

    void setUnregisteredMessageHandler(std::function<void(RawCommsMessage)> handler);

   private:
    void updateDatastreams();
    void updateHeartbeats();

    CommsDriver& _driver;

    std::function<void(RawCommsMessage)> _unregisteredMessageHandler;

    std::unordered_map<uint8_t, SensorDatastream> _sensorDatastreams;
    std::vector<SensorStatus> _sensorStatuses;

    HeartbeatManager _heartbeatManager;
    ErrorManager _errorManager;
    CommandManager _commandManager;

    MCUID _me;
};

}  // namespace comms

#endif  // __COMMS_H__
