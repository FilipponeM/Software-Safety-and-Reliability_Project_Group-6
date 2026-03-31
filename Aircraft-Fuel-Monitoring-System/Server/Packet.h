// Packet.h
//
//
//

#pragma once

//Identiy packet type for server
enum PacketType
{
    PACKET_TELEMETRY = 1,
    PACKET_STATE_SWITCH = 2
};

enum class ServerState
{
    DISCONNECTED,
    CONNECTED,
    AUTHENTICATED,
    DATA_RECEIVING,
    DATA_TRANSFER
};

//Static packetheader 
struct PacketHeader
{
    int packetType;
    int packetSize;
    int aircraftID;
    int sequenceNumber;
};

//Fuel data packet
struct FuelTelemetryData
{
    float fuelLevel;
    float fuelConsumption;
    float fuelTemperature;
    char* timestamp;
};

//ServerstateSwt=itch packet
struct ServerStateSwitch
{
    int requestedState;
    char* timestamp;

};