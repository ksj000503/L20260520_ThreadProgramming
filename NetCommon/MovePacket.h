#pragma once
#include "Packet.h"
#include <string>

class MovePacket : public IPacket
{
public:
	std::string UserID;
	float X;
	float Y;

	void Parse(std::string Instring) override;
	std::string ToString() override;
	int Length() override;

	EPacketType GetType() override
	{
		return EPacketType::MOVE;
	}
};