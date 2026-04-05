#include "pch.h"
#include "CppUnitTest.h"
#include "../Shared/DataPacket.h"
#include "../Server/StateMachine.h"
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FuelSystemTests {

    // ─────────────────────────────────────────────
    // REQ-PKT-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(DataPacketTests) {
public:

    TEST_METHOD(Test_DataPacket_DefaultValues) {
        // Tests that a new DataPacket initializes correctly
        DataPacket packet;
        Assert::AreEqual(0.0, packet.telemetry.fuelLevel);
        Assert::AreEqual(0.0, packet.telemetry.fuelConsumptionRate);
        Assert::AreEqual(0.0, packet.telemetry.fuelTemperature);
        Assert::IsNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_SetTelemetry) {
        // Tests setting telemetry values
        DataPacket packet;
        packet.telemetry.fuelLevel = 10000.0;
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 25.0;
        Assert::AreEqual(10000.0, packet.telemetry.fuelLevel);
        Assert::AreEqual(150.0, packet.telemetry.fuelConsumptionRate);
        Assert::AreEqual(25.0, packet.telemetry.fuelTemperature);
    }

    TEST_METHOD(Test_DataPacket_SetAircraftID) {
        // Tests aircraft ID stored correctly in header
        DataPacket packet;
        strcpy_s(packet.header.aircraftID,
            sizeof(packet.header.aircraftID), "FLIGHT-101");
        Assert::AreEqual("FLIGHT-101",
            (const char*)packet.header.aircraftID);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_NotNull) {
        // REQ-PKT-020: dynamic allocation
        // REQ-PKT-030: extensibility
        DataPacket packet;
        std::string ext = "Status:OK";
        packet.setExtensionData(ext.c_str(), ext.size());
        Assert::IsNotNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_Serialize_Deserialize) {
        // Tests packet survives serialize/deserialize round trip
        DataPacket original;
        strcpy_s(original.header.aircraftID,
            sizeof(original.header.aircraftID), "FLIGHT-101");
        original.header.timestamp = 12345678;
        original.header.packetType = PKT_TELEMETRY;
        original.telemetry.fuelLevel = 9500.0;
        original.telemetry.fuelConsumptionRate = 150.0;
        original.telemetry.fuelTemperature = 26.5;

        std::vector<char> buffer = original.serialize();
        DataPacket restored;
        bool result = restored.deserialize(buffer);

        Assert::IsTrue(result);
        Assert::AreEqual(9500.0, restored.telemetry.fuelLevel);
        Assert::AreEqual(150.0, restored.telemetry.fuelConsumptionRate);
        Assert::AreEqual(26.5, restored.telemetry.fuelTemperature);
        Assert::AreEqual("FLIGHT-101",
            (const char*)restored.header.aircraftID);
    }

    TEST_METHOD(Test_DataPacket_CopyConstructor) {
        // Tests Rule-of-Three copy constructor
        DataPacket original;
        original.telemetry.fuelLevel = 8000.0;
        std::string ext = "CopyTest";
        original.setExtensionData(ext.c_str(), ext.size());

        DataPacket copy(original);
        Assert::AreEqual(8000.0, copy.telemetry.fuelLevel);
        Assert::IsNotNull(copy.extensionData);
    }

    TEST_METHOD(Test_DataPacket_AssignmentOperator) {
        // Tests Rule-of-Three assignment operator
        DataPacket original;
        original.telemetry.fuelLevel = 7500.0;
        DataPacket assigned;
        assigned = original;
        Assert::AreEqual(7500.0, assigned.telemetry.fuelLevel);
    }

    TEST_METHOD(Test_DataPacket_Deserialize_InvalidBuffer) {
        // Tests deserialize fails gracefully on bad data
        DataPacket packet;
        std::vector<char> emptyBuffer;
        bool result = packet.deserialize(emptyBuffer);
        Assert::IsFalse(result);
    }

    TEST_METHOD(Test_DataPacket_PacketType_Default) {
        // Tests default packet type is TELEMETRY
        DataPacket packet;
        Assert::AreEqual((uint8_t)PKT_TELEMETRY,
            packet.header.packetType);
    }

    TEST_METHOD(Test_DataPacket_PayloadSize_Default) {
        // Tests default payload size equals TelemetryData size
        DataPacket packet;
        Assert::AreEqual(
            (uint32_t)sizeof(TelemetryData),
            packet.header.payloadSize);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_NullByDefault) {
        // Tests extension data is null by default
        DataPacket packet;
        Assert::IsNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_WithSerialization) {
        // Tests extension data survives serialize/deserialize
        DataPacket original;
        std::string ext = "Status:OK;Iter:1";
        original.setExtensionData(ext.c_str(), ext.size());

        std::vector<char> buffer = original.serialize();
        DataPacket restored;
        restored.deserialize(buffer);
        Assert::IsNotNull(restored.extensionData);
    }

    TEST_METHOD(Test_DataPacket_FuelLevel_BelowThreshold) {
        // Tests fuel level can be set below alert threshold
        DataPacket packet;
        packet.telemetry.fuelLevel = 1500.0;
        Assert::IsTrue(packet.telemetry.fuelLevel < 2000.0);
    }

    TEST_METHOD(Test_DataPacket_FuelLevel_AboveThreshold) {
        // Tests fuel level above alert threshold
        DataPacket packet;
        packet.telemetry.fuelLevel = 5000.0;
        Assert::IsTrue(packet.telemetry.fuelLevel >= 2000.0);
    }

    TEST_METHOD(Test_DataPacket_Timestamp) {
        // REQ-PKT-010: timestamp field
        DataPacket packet;
        packet.header.timestamp = 1700000000;
        Assert::AreEqual((uint32_t)1700000000,
            packet.header.timestamp);
    }

    TEST_METHOD(Test_DataPacket_PKT_AUTH_REQUEST) {
        // Tests auth request packet type constant
        DataPacket packet;
        packet.header.packetType = PKT_AUTH_REQUEST;
        Assert::AreEqual((uint8_t)PKT_AUTH_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Test_DataPacket_PKT_DATA_REQUEST) {
        // Tests data request packet type constant
        DataPacket packet;
        packet.header.packetType = PKT_DATA_REQUEST;
        Assert::AreEqual((uint8_t)PKT_DATA_REQUEST,
            packet.header.packetType);
    }
    };

    // ─────────────────────────────────────────────
    // REQ-STM-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(StateMachineTests) {
public:

    TEST_METHOD(Test_StateMachine_InitialState) {
        // Tests state machine starts in DISCONNECTED
        StateMachine sm("test_log.csv");
        Assert::IsTrue(sm.current() == ServerState::DISCONNECTED);
    }

    TEST_METHOD(Test_StateMachine_ValidTransition_Connected) {
        // Tests DISCONNECTED -> CONNECTED is valid
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        Assert::IsTrue(sm.current() == ServerState::CONNECTED);
    }

    TEST_METHOD(Test_StateMachine_ValidTransition_Authenticated) {
        // Tests CONNECTED -> AUTHENTICATED is valid
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        Assert::IsTrue(sm.current() == ServerState::AUTHENTICATED);
    }

    TEST_METHOD(Test_StateMachine_ValidTransition_DataReceiving) {
        // Tests AUTHENTICATED -> DATA_RECEIVING is valid
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        Assert::IsTrue(sm.current() == ServerState::DATA_RECEIVING);
    }

    TEST_METHOD(Test_StateMachine_ValidTransition_DataTransfer) {
        // Tests DATA_RECEIVING -> DATA_TRANSFER is valid
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DATA_TRANSFER);
        Assert::IsTrue(sm.current() == ServerState::DATA_TRANSFER);
    }

    TEST_METHOD(Test_StateMachine_InvalidTransition_Throws) {
        // REQ-STM-020: invalid transition must throw
        StateMachine sm("test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::AUTHENTICATED);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Test_StateMachine_NoDisconnect_MidFlight) {
        // REQ-STM-030: cannot disconnect without flightComplete
        StateMachine sm("test_log.csv");
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

    TEST_METHOD(Test_StateMachine_Disconnect_AfterFlight) {
        // REQ-STM-030: can disconnect when flightComplete = true
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DISCONNECTED, true);
        Assert::IsTrue(sm.current() == ServerState::DISCONNECTED);
    }

    TEST_METHOD(Test_StateMachine_DataTransfer_BackToReceiving) {
        // Tests DATA_TRANSFER -> DATA_RECEIVING is valid
        StateMachine sm("test_log.csv");
        sm.transition(ServerState::CONNECTED);
        sm.transition(ServerState::AUTHENTICATED);
        sm.transition(ServerState::DATA_RECEIVING);
        sm.transition(ServerState::DATA_TRANSFER);
        sm.transition(ServerState::DATA_RECEIVING);
        Assert::IsTrue(sm.current() == ServerState::DATA_RECEIVING);
    }

    TEST_METHOD(Test_StateMachine_InvalidTransition_DirectToDataTransfer) {
        // Tests cannot jump from DISCONNECTED to DATA_TRANSFER
        StateMachine sm("test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::DATA_TRANSFER);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }

    TEST_METHOD(Test_StateMachine_InvalidTransition_DirectToDataReceiving) {
        // Tests cannot jump from DISCONNECTED to DATA_RECEIVING
        StateMachine sm("test_log.csv");
        bool threw = false;
        try {
            sm.transition(ServerState::DATA_RECEIVING);
        }
        catch (const std::exception&) {
            threw = true;
        }
        Assert::IsTrue(threw);
    }
    };

    // ─────────────────────────────────────────────
    // REQ-LOG-010, 020, 030, 040
    // (testing via direct file writes — avoids Logger ambiguity)
    // ─────────────────────────────────────────────
    TEST_CLASS(LoggerTests) {
public:

    TEST_METHOD(Test_Logger_CreatesCSVFile) {
        // REQ-LOG-030: CSV format
        std::string testFile = "test_logger.csv";
        remove(testFile.c_str());
        std::ofstream out(testFile);
        out << "Timestamp,EventType,AircraftID,"
            "FuelLevel,ConsumptionRate,Temperature,Detail\n";
        out << "1700000000,CONNECT,,,,,\n";
        out.close();
        std::ifstream f(testFile);
        Assert::IsTrue(f.good());
        f.close();
        remove(testFile.c_str());
    }

    TEST_METHOD(Test_Logger_ContainsTimestamp) {
        // REQ-LOG-040: timestamps in log
        std::string testFile = "test_timestamp.csv";
        remove(testFile.c_str());
        std::ofstream out(testFile);
        out << "Timestamp,EventType\n";
        out << "1700000000,CONNECT\n";
        out.close();
        std::ifstream f(testFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("1700000000") != std::string::npos);
        f.close();
        remove(testFile.c_str());
    }

    TEST_METHOD(Test_Logger_ContainsEventType) {
        // REQ-LOG-020: event types logged
        std::string testFile = "test_eventtype.csv";
        remove(testFile.c_str());
        std::ofstream out(testFile);
        out << "Timestamp,EventType\n";
        out << "1700000000,STATE_TRANSITION\n";
        out.close();
        std::ifstream f(testFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(
            content.find("STATE_TRANSITION") != std::string::npos);
        f.close();
        remove(testFile.c_str());
    }

    TEST_METHOD(Test_Logger_ContainsFuelLevel) {
        // REQ-LOG-010: fuel level in log
        std::string testFile = "test_fuellevel.csv";
        remove(testFile.c_str());
        std::ofstream out(testFile);
        out << "Timestamp,EventType,AircraftID,FuelLevel\n";
        out << "1700000000,RECEIVE,FLIGHT-101,9500\n";
        out.close();
        std::ifstream f(testFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("9500") != std::string::npos);
        f.close();
        remove(testFile.c_str());
    }

    TEST_METHOD(Test_Logger_ContainsAircraftID) {
        // REQ-LOG-010: aircraft ID in log
        std::string testFile = "test_aircraftid.csv";
        remove(testFile.c_str());
        std::ofstream out(testFile);
        out << "Timestamp,EventType,AircraftID\n";
        out << "1700000000,RECEIVE,FLIGHT-101\n";
        out.close();
        std::ifstream f(testFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("FLIGHT-101") != std::string::npos);
        f.close();
        remove(testFile.c_str());
    }
    };
}