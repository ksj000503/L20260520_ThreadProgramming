#include "MovePacket.h"

void MovePacket::Parse(std::string Instring)
{
	JSONDocument.Parse(Instring.c_str());
	UserID = JSONDocument["UserID"].GetString();
	X = JSONDocument["X"].GetFloat();
	Y = JSONDocument["Y"].GetFloat();
}

std::string MovePacket::ToString()
{
    JSONDocument.SetObject();
    JSONDocument.AddMember("type", "move", JSONDocument.GetAllocator());
    JSONDocument.AddMember("UserID", UserID, JSONDocument.GetAllocator());
    JSONDocument.AddMember("X", X, JSONDocument.GetAllocator());
    JSONDocument.AddMember("Y", Y, JSONDocument.GetAllocator());

    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    JSONDocument.Accept(Writer);

    return Buffer.GetString();
}

int MovePacket::Length()
{
	return 0;
}
