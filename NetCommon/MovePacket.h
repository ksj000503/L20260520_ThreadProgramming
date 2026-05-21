#pragma once

#include "Packet.h"

class MovePacket : public IPacket
{
public:
    virtual void Parse(std::string InString) override;
    virtual std::string ToString() override;
    virtual int Length() override;
    virtual EPacketType GetType() override;

public:
    std::string UserID;

    float X = 0;
    float Y = 0;

    std::string Key = "";
};