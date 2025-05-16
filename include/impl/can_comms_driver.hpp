#ifndef __CAN_COMMS_DRIVER_H__
#define __CAN_COMMS_DRIVER_H__

#include <FlexCAN_T4.h>

#include <cstring>

#include "comms_driver.hpp"

namespace comms {

static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> _can1;
static FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> _can2;

enum CANBaudRate {
    CBR_100KBPS,
    CBR_125KBPS,
    CBR_250KBPS,
    CBR_500KBPS,
    CBR_1MBPS
};

template <uint8_t busNum, CANBaudRate baudRate>
class TeensyCANDriver : public CommsDriver {
   public:
    void install() {
        Serial.println("Installing TeensyCANDriver!");
        _busNum = busNum;
        uint32_t baudRateNum = 0;
        switch (baudRate) {
            case CBR_100KBPS:
                baudRateNum = 100000;
                break;
            case CBR_125KBPS:
                baudRateNum = 125000;
                break;
            case CBR_250KBPS:
                baudRateNum = 250000;
                break;
            case CBR_500KBPS:
                baudRateNum = 500000;
                break;
            case CBR_1MBPS:
                baudRateNum = 1000000;
                break;
        }

        switch (_busNum) {
            case 1:
                _can1.begin();
                _can1.setBaudRate(baudRateNum);
                break;
            case 2:
                _can2.begin();
                _can2.setBaudRate(baudRateNum);
                break;
        }

        Serial.println("Installing TeensyCANDriver!");
    }

    void uninstall() {
    }

    void sendMessage(const RawCommsMessage &message) {
        CAN_message_t msg;
        msg.id = message.id;

        Serial.printf("Sending message with id 0x%04x\n", message.id);
        // memcpy(msg.buf, &message.payload, 8);

        switch (_busNum) {
            case 1:
                _can1.write(msg);
                break;
            case 2:
                _can2.write(msg);
                break;
        }
    }

    bool receiveMessage(RawCommsMessage *message) {
        CAN_message_t res;
        bool found;
        switch (_busNum) {
            case 1:
                found = _can1.read(res);
                break;
            case 2:
                found = _can2.read(res);
                break;
        }

        if (!found) return false;

        message->id = res.id;
        memcpy(&message->payload, res.buf, 8);

        return true;
    }

   private:
    uint8_t _busNum;
    CANBaudRate _baudRate;
};

}  // namespace comms

#endif  // __CAN_COMMS_DRIVER_H__