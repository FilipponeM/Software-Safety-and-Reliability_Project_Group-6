#include "pch.h"
#include "CppUnitTest.h"
#include "../Shared/DataPacket.h"
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SharedTests {

    // ─────────────────────────────────────────────
    // Shared DataPacket Tests
    // REQ-PKT-010, 020, 030, 040
    // ─────────────────────────────────────────────
    TEST_CLASS(DataPacketTests) {
public:

    TEST_METHOD(Test_DataPacket_DefaultValues) {
        DataPacket packet;
        Assert::AreEqual(0.0, packet.telemetry.fuelLevel);
        Assert::AreEqual(0.0, packet.telemetry.fuelConsumptionRate);
        Assert::AreEqual(0.0, packet.telemetry.fuelTemperature);
        Assert::IsNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_SetTelemetry) {
        DataPacket packet;
        packet.telemetry.fuelLevel = 10000.0;
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 25.0;
        Assert::AreEqual(10000.0, packet.telemetry.fuelLevel);
        Assert::AreEqual(150.0, packet.telemetry.fuelConsumptionRate);
        Assert::AreEqual(25.0, packet.telemetry.fuelTemperature);
    }

    TEST_METHOD(Test_DataPacket_SetAircraftID) {
        DataPacket packet;
        strcpy_s(packet.header.aircraftID,
            sizeof(packet.header.aircraftID), "FLIGHT-101");
        Assert::AreEqual("FLIGHT-101",
            (const char*)packet.header.aircraftID);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_NotNull) {
        DataPacket packet;
        std::string ext = "Status:OK";
        packet.setExtensionData(ext.c_str(), ext.size());
        Assert::IsNotNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_Serialize_Deserialize) {
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
        DataPacket original;
        original.telemetry.fuelLevel = 8000.0;
        std::string ext = "CopyTest";
        original.setExtensionData(ext.c_str(), ext.size());
        DataPacket copy(original);
        Assert::AreEqual(8000.0, copy.telemetry.fuelLevel);
        Assert::IsNotNull(copy.extensionData);
    }

    TEST_METHOD(Test_DataPacket_AssignmentOperator) {
        DataPacket original;
        original.telemetry.fuelLevel = 7500.0;
        DataPacket assigned;
        assigned = original;
        Assert::AreEqual(7500.0, assigned.telemetry.fuelLevel);
    }

    TEST_METHOD(Test_DataPacket_Deserialize_InvalidBuffer) {
        DataPacket packet;
        std::vector<char> emptyBuffer;
        bool result = packet.deserialize(emptyBuffer);
        Assert::IsFalse(result);
    }

    TEST_METHOD(Test_DataPacket_PacketType_Default) {
        DataPacket packet;
        Assert::AreEqual((uint8_t)PKT_TELEMETRY,
            packet.header.packetType);
    }

    TEST_METHOD(Test_DataPacket_PayloadSize_Default) {
        DataPacket packet;
        Assert::AreEqual(
            (uint32_t)sizeof(TelemetryData),
            packet.header.payloadSize);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_NullByDefault) {
        DataPacket packet;
        Assert::IsNull(packet.extensionData);
    }

    TEST_METHOD(Test_DataPacket_ExtensionData_WithSerialization) {
        DataPacket original;
        std::string ext = "Status:OK;Iter:1";
        original.setExtensionData(ext.c_str(), ext.size());
        std::vector<char> buffer = original.serialize();
        DataPacket restored;
        restored.deserialize(buffer);
        Assert::IsNotNull(restored.extensionData);
    }

    TEST_METHOD(Test_DataPacket_FuelLevel_BelowThreshold) {
        DataPacket packet;
        packet.telemetry.fuelLevel = 1500.0;
        Assert::IsTrue(packet.telemetry.fuelLevel < 2000.0);
    }

    TEST_METHOD(Test_DataPacket_FuelLevel_AboveThreshold) {
        DataPacket packet;
        packet.telemetry.fuelLevel = 5000.0;
        Assert::IsTrue(packet.telemetry.fuelLevel >= 2000.0);
    }

    TEST_METHOD(Test_DataPacket_Timestamp) {
        DataPacket packet;
        packet.header.timestamp = 1700000000;
        Assert::AreEqual((uint32_t)1700000000,
            packet.header.timestamp);
    }

    TEST_METHOD(Test_DataPacket_PKT_AUTH_REQUEST) {
        DataPacket packet;
        packet.header.packetType = PKT_AUTH_REQUEST;
        Assert::AreEqual((uint8_t)PKT_AUTH_REQUEST,
            packet.header.packetType);
    }

    TEST_METHOD(Test_DataPacket_PKT_DATA_REQUEST) {
        DataPacket packet;
        packet.header.packetType = PKT_DATA_REQUEST;
        Assert::AreEqual((uint8_t)PKT_DATA_REQUEST,
            packet.header.packetType);
    }
    };
}