// ServerTests.cpp
// Unit tests for Server-side components
// Covers: StateMachine (REQ-STM-010, 020, 030, 040)
//         DataPacket serialization used by server (REQ-PKT-010, 020, 030, 040)
//         Logger server-side events (REQ-LOG-010, 020, 030, 040)

#include "pch.h"
#include "CppUnitTest.h"
#include "../Shared/DataPacket.h"
#include "../Server/StateMachine.h"
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ServerTests {

    // ─────────────────────────────────────────────
    // Server State Machine Tests
    // REQ-STM-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(ServerStateMachineTests) {
public:

    TEST_METHOD(Server_STM_InitialState_IsDisconnected) {
        // REQ-STM-010: server starts in DISCONNECTED state
        StateMachine sm("server_test_log.csv");
        Assert::IsTrue(sm.current() == ServerState::DISCONNECTED);
    }

    TEST_METHOD(Server_STM_Transition_DISCONNECTED_to_CONNECTED) {
        // REQ-STM-010, REQ-STM-020: valid transition
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        Assert::IsTrue(sm.current() == ServerState::CONNECTED);
    }

    TEST_METHOD(Server_STM_Transition_CONNECTED_to_AUTHENTICATED) {
        // REQ-STM-010, REQ-STM-020: valid transition after connection
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        Assert::IsTrue(sm.current() == ServerState::AUTHENTICATED);
    }

    TEST_METHOD(Server_STM_Transition_AUTHENTICATED_to_DATA_RECEIVING) {
        // REQ-STM-010, REQ-STM-020: valid transition after auth
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        Assert::IsTrue(sm.current() == ServerState::DATA_RECEIVING);
    }

    TEST_METHOD(Server_STM_Transition_DATA_RECEIVING_to_DATA_TRANSFER) {
        // REQ-STM-010, REQ-STM-020: valid transition to data transfer
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DATA_TRANSFER);
        Assert::IsTrue(sm.current() == ServerState::DATA_TRANSFER);
    }

    TEST_METHOD(Server_STM_Transition_DATA_TRANSFER_to_DATA_RECEIVING) {
        // REQ-STM-020: can return to receiving after transfer
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DATA_TRANSFER);
        sm.transition(ServerState::DATA_RECEIVING);
        Assert::IsTrue(sm.current() == ServerState::DATA_RECEIVING);
    }

    TEST_METHOD(Server_STM_InvalidTransition_DISCONNECTED_to_AUTHENTICATED) {
        // REQ-STM-020: must throw on invalid transition
        StateMachine sm("server_test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::AUTHENTICATED);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Server_STM_InvalidTransition_DISCONNECTED_to_DATA_RECEIVING) {
        // REQ-STM-020: cannot skip states
        StateMachine sm("server_test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::DATA_RECEIVING);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Server_STM_InvalidTransition_DISCONNECTED_to_DATA_TRANSFER) {
        // REQ-STM-020: cannot jump directly to DATA_TRANSFER
        StateMachine sm("server_test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::DATA_TRANSFER);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Server_STM_NoDisconnect_Without_FlightComplete) {
        // REQ-STM-030: cannot disconnect mid-flight
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        bool threw = false;
        try {
            sm.transition(ServerState::DISCONNECTED, false);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Server_STM_Disconnect_With_FlightComplete) {
        // REQ-STM-030: can disconnect when flight is complete
        StateMachine sm("server_test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DISCONNECTED, true);
        Assert::IsTrue(sm.current() == ServerState::DISCONNECTED);
    }

    TEST_METHOD(Server_STM_LogFile_Created_On_Transition) {
        // REQ-STM-040: state transitions logged to file
        std::string logFile = "server_stm_test.csv";
        remove(logFile.c_str());
        StateMachine sm(logFile);
        sm.transition(ServerState::CONNECTED);
        std::ifstream f(logFile);
        Assert::IsTrue(f.good());
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_STM_LogFile_Contains_Transition) {
        // REQ-STM-040: log contains transition details
        std::string logFile = "server_stm_content_test.csv";
        remove(logFile.c_str());
        StateMachine sm(logFile);
        sm.transition(ServerState::CONNECTED);
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(
            content.find("DISCONNECTED->CONNECTED") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }
    };

    // ─────────────────────────────────────────────
    // Server Packet Processing Tests
    // REQ-SVR-030: receive and parse structured packets
    // REQ-PKT-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(ServerPacketTests) {
public:

    TEST_METHOD(Server_Packet_Deserialize_ValidBuffer) {
        // REQ-SVR-030: server can parse incoming packets
        DataPacket original;
        strcpy_s(original.header.aircraftID,
            sizeof(original.header.aircraftID), "FLIGHT-101");
        original.header.packetType = PKT_TELEMETRY;
        original.telemetry.fuelLevel = 9500.0;
        original.telemetry.fuelConsumptionRate = 150.0;
        original.telemetry.fuelTemperature = 26.5;

        std::vector<char> buffer = original.serialize();
        DataPacket received;
        bool result = received.deserialize(buffer);

        Assert::IsTrue(result);
        Assert::AreEqual(9500.0, received.telemetry.fuelLevel);
        Assert::AreEqual(150.0, received.telemetry.fuelConsumptionRate);
        Assert::AreEqual(26.5, received.telemetry.fuelTemperature);
    }

    TEST_METHOD(Server_Packet_Deserialize_InvalidBuffer) {
        // REQ-SVR-030: server handles malformed packets gracefully
        DataPacket packet;
        std::vector<char> badBuffer(2, 0);
        bool result = packet.deserialize(badBuffer);
        Assert::IsFalse(result);
    }

    TEST_METHOD(Server_Packet_Identifies_Telemetry_Type) {
        // REQ-SVR-030: server identifies packet type correctly
        DataPacket packet;
        packet.header.packetType = PKT_TELEMETRY;
        Assert::AreEqual((uint8_t)PKT_TELEMETRY,
            packet.header.packetType);
    }

    TEST_METHOD(Server_Packet_Identifies_Auth_Request) {
        // REQ-SVR-020: server identifies auth request packet
        DataPacket packet;
        packet.header.packetType = PKT_AUTH_REQUEST;
        Assert::AreEqual((uint8_t)PKT_AUTH_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Server_Packet_Identifies_Data_Request) {
        // REQ-SVR-060: server identifies large data request
        DataPacket packet;
        packet.header.packetType = PKT_DATA_REQUEST;
        Assert::AreEqual((uint8_t)PKT_DATA_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Server_Packet_FuelAlert_Below_Threshold) {
        // REQ-SVR-070: fuel level below threshold detected
        DataPacket packet;
        packet.telemetry.fuelLevel = 1500.0;
        Assert::IsTrue(packet.telemetry.fuelLevel < 2000.0);
    }

    TEST_METHOD(Server_Packet_FuelAlert_Above_Threshold) {
        // REQ-SVR-070: no alert when fuel above threshold
        DataPacket packet;
        packet.telemetry.fuelLevel = 5000.0;
        Assert::IsFalse(packet.telemetry.fuelLevel < 2000.0);
    }

    TEST_METHOD(Server_Packet_AircraftID_Stored) {
        // REQ-PKT-010: aircraft ID in packet
        DataPacket packet;
        strcpy_s(packet.header.aircraftID,
            sizeof(packet.header.aircraftID), "FLIGHT-101");
        Assert::AreEqual("FLIGHT-101",
            (const char*)packet.header.aircraftID);
    }

    TEST_METHOD(Server_Packet_Timestamp_Stored) {
        // REQ-PKT-010: timestamp in packet
        DataPacket packet;
        packet.header.timestamp = 1700000000;
        Assert::AreEqual((uint32_t)1700000000,
            packet.header.timestamp);
    }

    TEST_METHOD(Server_Packet_ExtensionData_Dynamic) {
        // REQ-PKT-020: dynamically allocated element
        DataPacket packet;
        std::string ext = "Server:Extension";
        packet.setExtensionData(ext.c_str(), ext.size());
        Assert::IsNotNull(packet.extensionData);
    }
    };

    // ─────────────────────────────────────────────
    // Server Logging Tests
    // REQ-SVR-040, REQ-LOG-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(ServerLoggingTests) {
public:

    TEST_METHOD(Server_Log_RECEIVE_Event) {
        // REQ-SVR-040: server logs received packets
        std::string logFile = "server_receive_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,AircraftID,"
            "FuelLevel,ConsumptionRate,Temperature,Detail\n";
        out << "1700000001,RECEIVE,FLIGHT-101,9500,150,26.5,\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("RECEIVE") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_CONNECT_Event) {
        // REQ-LOG-020: server logs connect events
        std::string logFile = "server_connect_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType\n";
        out << "1700000001,CONNECT\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("CONNECT") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_DISCONNECT_Event) {
        // REQ-LOG-020: server logs disconnect events
        std::string logFile = "server_disconnect_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType\n";
        out << "1700000001,DISCONNECT\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("DISCONNECT") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_ERROR_Event) {
        // REQ-LOG-020: server logs error events
        std::string logFile = "server_error_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,Detail\n";
        out << "1700000001,ERROR,Client disconnected\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("ERROR") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_STATE_TRANSITION_Event) {
        // REQ-STM-040: state transitions logged
        std::string logFile = "server_state_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,Detail\n";
        out << "1700000001,STATE_TRANSITION,CONNECTED->AUTHENTICATED\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(
            content.find("STATE_TRANSITION") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_CSV_Format) {
        // REQ-LOG-030: log in CSV format
        std::string logFile = "server_csv_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,AircraftID\n";
        out << "1700000001,RECEIVE,FLIGHT-101\n";
        out.close();
        std::ifstream f(logFile);
        std::string firstLine;
        std::getline(f, firstLine);
        Assert::IsTrue(firstLine.find(",") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_Contains_Timestamp) {
        // REQ-LOG-040: timestamps in every log entry
        std::string logFile = "server_timestamp_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType\n";
        out << "1700000001,RECEIVE\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("1700000001") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Server_Log_FUEL_ALERT_Event) {
        // REQ-SVR-070: fuel alert logged
        std::string logFile = "server_alert_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,Detail\n";
        out << "1700000001,FUEL_ALERT,LOW FUEL: FLIGHT-101 fuel=1500\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("FUEL_ALERT") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }
    };
}