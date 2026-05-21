#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Packet.h"
#include "ChatPacket.h"
#include "MovePacket.h"
#include "NetUtill.h"

#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <process.h>
#include <conio.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "NetCommon")


using namespace std;

char SendBuffer[1024] = { 0, };
char RecvBuffer[1024] = { 0, };

bool IsRecvThreadRunning = true;
bool IsSendThreadRunning = true;

char UserID[64] = {0,};
struct Player
{
	int X = 1;
	int Y = 1;
};
map<string, Player> Players;
vector<string> ChatLogs;

vector<string> MapData;

void LoadMap()
{
	ifstream File("map.txt");

	if (File.is_open() == false)
	{
		cout << "map load fail" << endl;
		return;
	}

	string Line;

	while (getline(File, Line))
	{
		MapData.push_back(Line);
	}

	cout << "map load success" << endl;
}

void DrawMap()
{
	system("cls");

	for (int y = 0; y < (int)MapData.size(); ++y)
	{
		for (int x = 0; x < (int)MapData[y].size(); ++x)
		{
			bool IsPlayer = false;

			for (auto& Pair : Players)
			{
				Player& CurrentPlayer = Pair.second;

				if (CurrentPlayer.X == x && CurrentPlayer.Y == y)
				{
					cout << Pair.first[0];
					IsPlayer = true;
					break;
				}
			}

			if (IsPlayer == true)
			{
				continue;
			}

			if (MapData[y][x] == '1')
			{
				cout << "¡á";
			}
			else
			{
				cout << " ";
			}
		}

		cout << endl;
	}

	cout << endl;

	for (string& Chat : ChatLogs)
	{
		cout << Chat << endl;
	}
}

void MakePacketHeader(PacketHeader& OutPacketHeader, int DataSize, EPacketType Type)
{
	OutPacketHeader.size = htons(DataSize);
	OutPacketHeader.type = htons(static_cast<unsigned short>(Type));
}

unsigned WINAPI RecvThread(void* Argument)
{
	SOCKET ServerSocket = *(SOCKET*)Argument;

	

	while (IsRecvThreadRunning)
	{
		PacketHeader Header;
		int RecvBytes = recv(ServerSocket, (char*)&Header, sizeof(Header), MSG_WAITALL);
		if (RecvBytes <= 0)
		{
			cout << "recv header fail" << endl;
			break;
		}
		
		unsigned short PacketSize = ntohs(Header.size);
		unsigned short PacketType = ntohs(Header.type);

		memset(RecvBuffer, 0, sizeof(RecvBuffer));
		RecvBytes = recv(ServerSocket, RecvBuffer, PacketSize, MSG_WAITALL);
		if (RecvBytes <= 0)
		{
			cout << "recv data fail" << endl;
			break;
		}


		switch (static_cast<EPacketType>(PacketType))
		{
		case EPacketType::CHAT:
		{
			ChatPacket Data;
			Data.Parse(RecvBuffer); 
			string ChatText = "ID: " + Data.UserID +" Message: " + Data.Message;

			ChatLogs.push_back(ChatText);

			if (ChatLogs.size() > 5)
			{
				ChatLogs.erase(ChatLogs.begin());
			}

			DrawMap();
			break;
		}
		case EPacketType::MOVE:
		{
			MovePacket Data;
			Data.Parse(RecvBuffer);

			Players[Data.UserID].X = (int)Data.X;
			Players[Data.UserID].Y = (int)Data.Y;

			DrawMap();

			break;
		}
		default:
			cout << "unknown packet type" << endl;
			break;
		}
	}

	return 0;
}

unsigned WINAPI SendThread(void* Argument)
{
	SOCKET ServerSocket = *(SOCKET*)Argument;

	MovePacket MoveData;
	MoveData.UserID = UserID;
	MoveData.X = 0;
	MoveData.Y = 0;

	while (IsSendThreadRunning)
	{
		char ch = _getch();  

		std::string JSONString = "";
		PacketHeader Header;


		if (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd')
		{
			MoveData.Key = std::string(1, ch);

			JSONString = MoveData.ToString();
			MakePacketHeader(Header, JSONString.size(), MoveData.GetType());
		}
	
		else if (ch == '\r')
		{
			cout << "chat: ";
			cin.getline(SendBuffer, sizeof(SendBuffer));

			ChatPacket Data;
			Data.UserID = UserID;
			Data.Message = SendBuffer;
			Data.Gold = 1000;

			JSONString = Data.ToString();
			MakePacketHeader(Header, JSONString.size(), Data.GetType());
		}
		else
		{
			continue; 
		}

		int SentBytes = SendAll(ServerSocket, (char*)&Header, sizeof(Header));
		if (SentBytes <= 0) { break; }

		SentBytes = SendAll(ServerSocket, JSONString.c_str(), JSONString.size());
		if (SentBytes <= 0) { break; }
	}

	return 0;
}

int main()
{
	cout << "client" << endl;

	LoadMap();
	DrawMap();
	char Path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, Path);

	cout << Path << endl;
	WSAData wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(35000);

	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	cout << "client connect" << endl;

	cout << "UserID ÀÔ·Â: ";
	cin.getline(UserID, sizeof(UserID));

	HANDLE ThreadHandles[2] = { 0, };

	//nonblocking, asynchrous
	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, RecvThread, &ServerSocket, /*CREATE_SUSPENDED*/0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, SendThread, &ServerSocket, /*CREATE_SUSPENDED*/0, 0);
	//ResumeThread(ThreadHandles[0]);
	//ResumeThread(ThreadHandles[1]);
	//SuspendThread(ThreadHandles[0]);
	//SuspendThread(ThreadHandles[1]);


	//blocking
	WaitForMultipleObjects(2, ThreadHandles, FALSE, INFINITE);

	closesocket(ServerSocket);

	//TerminateThread(ThreadHandles[0], 0);
	//TerminateThread(ThreadHandles[1], 0);
	IsSendThreadRunning = false;
	IsRecvThreadRunning = false;


	CloseHandle(ThreadHandles[0]);
	CloseHandle(ThreadHandles[1]);

	WSACleanup();

	return 0;
}