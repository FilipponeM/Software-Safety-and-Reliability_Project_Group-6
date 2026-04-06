#pragma once

#include <string>
#include <stdexcept>
#include "../Shared/Logger.h"

// REQ-STM-010: all required states
enum class ServerState {
    DISCONNECTED = 0,
    CONNECTED = 1,
    AUTHENTICATED = 2,
    DATA_RECEIVING = 3,
    DATA_TRANSFER = 4
};

inline std::string stateToString(ServerState s) {
    switch (s) {
    case ServerState::DISCONNECTED:   return "DISCONNECTED";
    case ServerState::CONNECTED:      return "CONNECTED";
    case ServerState::AUTHENTICATED:  return "AUTHENTICATED";
    case ServerState::DATA_RECEIVING: return "DATA_RECEIVING";
    case ServerState::DATA_TRANSFER:  return "DATA_TRANSFER";
    }
    return "UNKNOWN";
}

class StateMachine {
public:
    explicit StateMachine(const std::string& logFile)
        : current_(ServerState::DISCONNECTED), logFile_(logFile) {}

    ServerState current() const { return current_; }

    // REQ-STM-020: validate transition
    // REQ-STM-040: log every transition
    // REQ-STM-030: flightComplete required before DISCONNECTED
    void transition(ServerState next, bool flightComplete = false) {
        if (!isValid(current_, next, flightComplete))
            throw std::runtime_error(
                "Invalid state transition: " +
                stateToString(current_) + " -> " + stateToString(next));
        Logger::logStateTransition(logFile_, stateToString(current_), stateToString(next));
        current_ = next;
    }

private:
    ServerState current_;
    std::string logFile_;

    static bool isValid(ServerState from, ServerState to, bool flightComplete) {
        switch (from) {
        case ServerState::DISCONNECTED:
            return to == ServerState::CONNECTED;
        case ServerState::CONNECTED:
            if (to == ServerState::DISCONNECTED) return flightComplete;
            return to == ServerState::AUTHENTICATED;
        case ServerState::AUTHENTICATED:
            if (to == ServerState::DISCONNECTED) return flightComplete;
            return to == ServerState::DATA_RECEIVING || to == ServerState::DATA_TRANSFER;
        case ServerState::DATA_RECEIVING:
            if (to == ServerState::DISCONNECTED) return flightComplete;
            return to == ServerState::DATA_TRANSFER || to == ServerState::AUTHENTICATED;
        case ServerState::DATA_TRANSFER:
            if (to == ServerState::DISCONNECTED) return flightComplete;
            return to == ServerState::DATA_RECEIVING || to == ServerState::AUTHENTICATED;
        }
        return false;
    }
};