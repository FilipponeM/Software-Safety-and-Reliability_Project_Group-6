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
    DISCONNECTED = 0,
    CONNECTED = 1,
    AUTHENTICATED = 2,
    DATA_RECEIVING = 3,
    DATA_TRANSFER = 4
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
    char timestamp[32];
};

//ServerstateSwt=itch packet
struct ServerStateSwitch
{
    int requestedState;
    char timestamp[32];

};