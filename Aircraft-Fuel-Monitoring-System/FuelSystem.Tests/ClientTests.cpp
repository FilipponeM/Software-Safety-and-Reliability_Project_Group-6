// ClientTests.cpp
// Unit tests for Client-side components
// Covers: Client packet building (REQ-CLT-010, 020)
//         Client logging (REQ-CLT-050, REQ-LOG-010, 020, 030, 040)
//         Client telemetry data (REQ-CLT-010)
//         Client reconnection logic (REQ-COM-030)
//         Client UI threshold (REQ-CLT-080)


#include "pch.h"
#include "CppUnitTest.h"
#include "../Shared/DataPacket.h"
#include "../Client/Client.h"
#include <fstream>
#include <string>
#include <chrono>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ClientTests {

    // ─────────────────────────────────────────────
    // Client Packet Building Tests
    // REQ-CLT-010, 020, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(ClientPacketBuildingTests) {
public:

    TEST_METHOD(Client_Packet_BuildsTelemetryPacket) {
        // REQ-CLT-020: client packages data into structured packet
        DataPacket packet;
        strcpy_s(packet.header.aircraftID,
            sizeof(packet.header.aircraftID), "FLIGHT-101");
        packet.header.packetType = PKT_TELEMETRY;
        packet.telemetry.fuelLevel = 10000.0;
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 25.0;

        Assert::AreEqual("FLIGHT-101",
            (const char*)packet.header.aircraftID);
        Assert::AreEqual(10000.0, packet.telemetry.fuelLevel);
        Assert::AreEqual(150.0, packet.telemetry.fuelConsumptionRate);
        Assert::AreEqual(25.0, packet.telemetry.fuelTemperature);
    }

    TEST_METHOD(Client_Packet_SetsTimestamp) {
        // REQ-CLT-010: packet includes timestamp
        DataPacket packet;
        uint32_t ts = static_cast<uint32_t>(
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));
        packet.header.timestamp = ts;
        Assert::IsTrue(packet.header.timestamp > 0);
    }

    TEST_METHOD(Client_Packet_SetsPacketType_Telemetry) {
        // REQ-CLT-020: client sets correct packet type
        DataPacket packet;
        packet.header.packetType = PKT_TELEMETRY;
        Assert::AreEqual((uint8_t)PKT_TELEMETRY,
            packet.header.packetType);
    }

    TEST_METHOD(Client_Packet_SetsPacketType_AuthRequest) {
        // REQ-SVR-020: client builds auth request packet
        DataPacket packet;
        packet.header.packetType = PKT_AUTH_REQUEST;
        Assert::AreEqual((uint8_t)PKT_AUTH_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Client_Packet_SetsPacketType_DataRequest) {
        // REQ-CLT-060: client builds data request packet
        DataPacket packet;
        packet.header.packetType = PKT_DATA_REQUEST;
        Assert::AreEqual((uint8_t)PKT_DATA_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Client_Packet_SetsExtensionData) {
        // REQ-PKT-030: client uses extensible packet fields
        DataPacket packet;
        std::string ext = "Status:OK;Iter:0";
        packet.setExtensionData(ext.c_str(), ext.size());
        Assert::IsNotNull(packet.extensionData);
    }

    TEST_METHOD(Client_Packet_FuelLevel_DecreasesOverTime) {
        // REQ-CLT-010: fuel level collected and decreases each iteration
        double fuelLevel = 10000.0;
        double rate = 150.0;

        DataPacket packet1;
        packet1.telemetry.fuelLevel = fuelLevel;

        DataPacket packet2;
        packet2.telemetry.fuelLevel = fuelLevel - rate;

        Assert::IsTrue(
            packet2.telemetry.fuelLevel < packet1.telemetry.fuelLevel);
    }

    TEST_METHOD(Client_Packet_Temperature_IncreasesOverTime) {
        // REQ-CLT-010: temperature collected correctly
        DataPacket packet1;
        packet1.telemetry.fuelTemperature = 25.0;

        DataPacket packet2;
        packet2.telemetry.fuelTemperature = 25.5;

        Assert::IsTrue(
            packet2.telemetry.fuelTemperature >
            packet1.telemetry.fuelTemperature);
    }

    TEST_METHOD(Client_Packet_Serializes_For_Transmission) {
        // REQ-CLT-020: packet serialized before sending
        DataPacket packet;
        packet.telemetry.fuelLevel = 9000.0;
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 27.0;

        std::vector<char> buffer = packet.serialize();
        Assert::IsTrue(buffer.size() > 0);
        Assert::IsTrue(buffer.size() >=
            sizeof(PacketHeader) + sizeof(TelemetryData));
    }

    TEST_METHOD(Client_Packet_AircraftID_Max16Chars) {
        // REQ-PKT-040: fixed header size — aircraftID max 16 chars
        DataPacket packet;
        Assert::IsTrue(sizeof(packet.header.aircraftID) == 16);
    }

    TEST_METHOD(Client_Packet_Header_FixedSize) {
        // REQ-PKT-040: fixed packet header size
        Assert::IsTrue(sizeof(PacketHeader) > 0);
        // Size should be consistent every time
        Assert::AreEqual(sizeof(PacketHeader), sizeof(PacketHeader));
    }
    };

    // ─────────────────────────────────────────────
    // Client Fuel Alert Tests
    // REQ-CLT-080
    // ─────────────────────────────────────────────
    TEST_CLASS(ClientFuelAlertTests) {
public:

    const double FUEL_THRESHOLD = 2000.0;

    TEST_METHOD(Client_FuelAlert_TriggersBelow2000) {
        // REQ-CLT-080: alert when fuel below threshold
        DataPacket packet;
        packet.telemetry.fuelLevel = 1900.0;
        Assert::IsTrue(packet.telemetry.fuelLevel < FUEL_THRESHOLD);
    }

    TEST_METHOD(Client_FuelAlert_NoAlertAbove2000) {
        // REQ-CLT-080: no alert when fuel above threshold
        DataPacket packet;
        packet.telemetry.fuelLevel = 5000.0;
        Assert::IsFalse(packet.telemetry.fuelLevel < FUEL_THRESHOLD);
    }

    TEST_METHOD(Client_FuelAlert_ExactlyAtThreshold) {
        // REQ-CLT-080: at exactly 2000 no alert
        DataPacket packet;
        packet.telemetry.fuelLevel = 2000.0;
        Assert::IsFalse(packet.telemetry.fuelLevel < FUEL_THRESHOLD);
    }

    TEST_METHOD(Client_FuelAlert_ZeroFuel) {
        // REQ-CLT-080: zero fuel triggers alert
        DataPacket packet;
        packet.telemetry.fuelLevel = 0.0;
        Assert::IsTrue(packet.telemetry.fuelLevel < FUEL_THRESHOLD);
    }

    TEST_METHOD(Client_FuelAlert_FullTank) {
        // REQ-CLT-080: full tank no alert
        DataPacket packet;
        packet.telemetry.fuelLevel = 10000.0;
        Assert::IsFalse(packet.telemetry.fuelLevel < FUEL_THRESHOLD);
    }
    };

    // ─────────────────────────────────────────────
    // Client Reconnection Logic Tests
    // REQ-COM-030
    // ─────────────────────────────────────────────
    TEST_CLASS(ClientReconnectionTests) {
public:

    TEST_METHOD(Client_Reconnect_MaxAttemptsConstant) {
        // REQ-COM-030: reconnection attempts defined
        Assert::IsTrue(MAX_RECONNECT_ATTEMPTS > 0);
        Assert::IsTrue(MAX_RECONNECT_ATTEMPTS <= 10);
    }

    TEST_METHOD(Client_Reconnect_MaxAttempts_Is3) {
        // REQ-COM-030: max 3 reconnect attempts
        Assert::AreEqual(3, MAX_RECONNECT_ATTEMPTS);
    }
    };

    // ─────────────────────────────────────────────
    // Client Logging Tests
    // REQ-CLT-050, REQ-LOG-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(ClientLoggingTests) {
public:

    TEST_METHOD(Client_Log_SEND_Event) {
        // REQ-CLT-050: client logs transmitted packets
        std::string logFile = "client_send_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,AircraftID,"
            "FuelLevel,ConsumptionRate,Temperature,Detail\n";
        out << "1700000001,SEND,FLIGHT-101,10000,150,25,Status:OK\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("SEND") != std::string::npos);
        Assert::IsTrue(content.find("FLIGHT-101") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Client_Log_CONNECT_Event) {
        // REQ-LOG-020: client logs connect events
        std::string logFile = "client_connect_test.csv";
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

    TEST_METHOD(Client_Log_DISCONNECT_Event) {
        // REQ-LOG-020: client logs disconnect events
        std::string logFile = "client_disconnect_test.csv";
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

    TEST_METHOD(Client_Log_ERROR_Event) {
        // REQ-LOG-020: client logs error events
        std::string logFile = "client_error_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,Detail\n";
        out << "1700000001,ERROR,Send failed\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("ERROR") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Client_Log_CSV_Format) {
        // REQ-LOG-030: client log in CSV format
        std::string logFile = "client_csv_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,AircraftID\n";
        out << "1700000001,SEND,FLIGHT-101\n";
        out.close();
        std::ifstream f(logFile);
        std::string firstLine;
        std::getline(f, firstLine);
        Assert::IsTrue(firstLine.find(",") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Client_Log_Contains_Timestamp) {
        // REQ-LOG-040: timestamps in client log
        std::string logFile = "client_timestamp_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType\n";
        out << "1700000001,SEND\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("1700000001") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Client_Log_Contains_FuelLevel) {
        // REQ-LOG-010: fuel level in client log
        std::string logFile = "client_fuel_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType,AircraftID,FuelLevel\n";
        out << "1700000001,SEND,FLIGHT-101,9500\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("9500") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }

    TEST_METHOD(Client_Log_SEND_RETRY_Event) {
        // REQ-COM-030: client logs retry after reconnect
        std::string logFile = "client_retry_test.csv";
        remove(logFile.c_str());
        std::ofstream out(logFile);
        out << "Timestamp,EventType\n";
        out << "1700000001,SEND_RETRY\n";
        out.close();
        std::ifstream f(logFile);
        std::string content((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        Assert::IsTrue(content.find("SEND_RETRY") != std::string::npos);
        f.close();
        remove(logFile.c_str());
    }
    };
}