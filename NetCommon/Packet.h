#pragma once
#include "pch.h"

enum class EPacketType : unsigned short
{
    NONE = 0,
    CHAT = 1,
    MOVE = 2,
};

struct PacketHeader
{
    uint16_t size;
    uint16_t type;
};

class IPacket
{
public:
    virtual void Parse(std::string InString) = 0;
    virtual std::string ToString() = 0;
    virtual int Length() = 0;
    virtual EPacketType GetType() = 0;

    rapidjson::Document JSONDocument;
};
