#include "MovePacket.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

void MovePacket::Parse(std::string InString)
{
    JSONDocument.Parse(InString.c_str());

    if (JSONDocument.HasMember("UserID"))
    {
        UserID = JSONDocument["UserID"].GetString();
    }

    if (JSONDocument.HasMember("X"))
    {
        X = JSONDocument["X"].GetFloat();
    }

    if (JSONDocument.HasMember("Y"))
    {
        Y = JSONDocument["Y"].GetFloat();
    }

    if (JSONDocument.HasMember("Key"))
    {
        Key = JSONDocument["Key"].GetString();
    }
}

std::string MovePacket::ToString()
{
    JSONDocument.SetObject();

    rapidjson::Document::AllocatorType& Allocator = JSONDocument.GetAllocator();

    JSONDocument.AddMember(
        "UserID",
        rapidjson::Value(UserID.c_str(), Allocator),
        Allocator
    );

    JSONDocument.AddMember("X", X, Allocator);
    JSONDocument.AddMember("Y", Y, Allocator);

    JSONDocument.AddMember(
        "Key",
        rapidjson::Value(Key.c_str(), Allocator),
        Allocator
    );

    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    JSONDocument.Accept(Writer);

    return Buffer.GetString();
}

int MovePacket::Length()
{
    return ToString().size();
}

EPacketType MovePacket::GetType()
{
    return EPacketType::MOVE;
}