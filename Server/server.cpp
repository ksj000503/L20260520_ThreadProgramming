#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetUtill.h"
#include "Packet.h"
#include "ChatPacket.h"
#include "MovePacket.h"

#include <winsock2.h>
#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "NetCommon")

using namespace std;

char Buffer[1024] = { 0, };

int PlayerX = 1;
int PlayerY = 1;

struct Player
{
    string UserID;
    int X = 1;
    int Y = 1;
};

map<string, Player> Players;
vector<string> MapData;

void ProcessMove(Player& CurrentPlayer, string Key)
{
    int NextX = CurrentPlayer.X;
    int NextY = CurrentPlayer.Y;

    if (Key == "w") NextY--;
    if (Key == "s") NextY++;
    if (Key == "a") NextX--;
    if (Key == "d") NextX++;

    if (NextY < 0 || NextY >= (int)MapData.size())
    {
        return;
    }

    if (NextX < 0 || NextX >= (int)MapData[NextY].size())
    {
        return;
    }


    if (MapData[NextY][NextX] != '1')
    {
        CurrentPlayer.X = NextX;
        CurrentPlayer.Y = NextY;
    }
}

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



int main()
{
    cout << "server start" << endl;

    LoadMap();
    

    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN ListenSockAddr;
    memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));

    ListenSockAddr.sin_family = AF_INET;
    ListenSockAddr.sin_addr.s_addr = INADDR_ANY;
    ListenSockAddr.sin_port = htons(35000);

    bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));

    listen(ListenSocket, SOMAXCONN);

    TIMEVAL TimeOut;
    TimeOut.tv_sec = 0;
    TimeOut.tv_usec = 500000;

    fd_set ReadSockets;
    fd_set CopyReadSockets;

    FD_ZERO(&ReadSockets);
    FD_SET(ListenSocket, &ReadSockets);

    while (true)
    {
        CopyReadSockets = ReadSockets;

        int ChangeCount = select(0, &CopyReadSockets, 0, 0, &TimeOut);

        if (ChangeCount <= 0)
        {
            continue;
        }

        for (int i = 0; i < (int)ReadSockets.fd_count; ++i)
        {
            if (FD_ISSET(ReadSockets.fd_array[i], &CopyReadSockets))
            {
                if (ReadSockets.fd_array[i] == ListenSocket)
                {
                    SOCKADDR_IN ClientSockAddr;
                    memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));

                    int ClientSockSockLength = sizeof(ClientSockAddr);

                    SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockSockLength);

                    cout << "connect client " << inet_ntoa(ClientSockAddr.sin_addr)<< endl;

                    FD_SET(ClientSocket, &ReadSockets);

                    for (auto& Pair : Players)
                    {
                        MovePacket SendData;
                        SendData.UserID = Pair.second.UserID;
                        SendData.X = Pair.second.X;
                        SendData.Y = Pair.second.Y;

                        string JSONString = SendData.ToString();
                        PacketHeader SendHeader;
                        SendHeader.size = htons((unsigned short)JSONString.size());
                        SendHeader.type = htons((unsigned short)EPacketType::MOVE);

                        SendAll(ClientSocket, (char*)&SendHeader, sizeof(SendHeader));
                        SendAll(ClientSocket, JSONString.c_str(), JSONString.size());
                    }
                }
                else
                {
                    PacketHeader Header;

                    int RecvBytes = recv(ReadSockets.fd_array[i], (char*)&Header, sizeof(Header),MSG_WAITALL);

                    if (RecvBytes <= 0)
                    {
                        cout << "header recv fail" << endl;
                        DisconnectSocket(ReadSockets.fd_array[i], &ReadSockets);
                        continue;
                    }

                    unsigned short PacketSize = ntohs(Header.size);
                    unsigned short PacketType = ntohs(Header.type);

                    memset(Buffer, 0, sizeof(Buffer));

                    RecvBytes = recv(ReadSockets.fd_array[i], Buffer, PacketSize, MSG_WAITALL);

                    if (RecvBytes <= 0)
                    {
                        cout << "data recv fail" << endl;
                        DisconnectSocket(ReadSockets.fd_array[i], &ReadSockets);
                        continue;
                    }

                    switch ((EPacketType)PacketType)
                    {
                    case EPacketType::CHAT:
                    {
                        ChatPacket Data;
                        Data.Parse(Buffer);

                        cout << "[CHAT] " << Data.UserID << " : " << Data.Message<< endl;

                        for (int j = 0; j < (int)ReadSockets.fd_count; ++j)
                        {
                            if (ReadSockets.fd_array[j] != ListenSocket)
                            {
                                PacketHeader SendHeader;

                                SendHeader.size = htons(PacketSize);
                                SendHeader.type = htons(PacketType);

                                int SentBytes = SendAll(ReadSockets.fd_array[j], (char*)&SendHeader, sizeof(SendHeader));

                                if (SentBytes <= 0)
                                {
                                    continue;
                                }

                                SendAll(ReadSockets.fd_array[j], Buffer, PacketSize);
                            }
                        }

                        break;
                    }

                    case EPacketType::MOVE:
                    {
                        MovePacket Data;
                        Data.Parse(Buffer);

                        Player& CurrentPlayer = Players[Data.UserID];;
                        CurrentPlayer.UserID = Data.UserID;

                        int PrevX = CurrentPlayer.X;
                        int PrevY = CurrentPlayer.Y;

                        ProcessMove(CurrentPlayer, Data.Key);

                        cout << "[MOVE] " << Data.UserID << " (" << PrevX << ", " << PrevY << ")" << " -> "
                            << "(" << CurrentPlayer.X << ", " << CurrentPlayer.Y << ")" << endl;

                        MovePacket SendData;

                        SendData.UserID = Data.UserID;
                        SendData.X = CurrentPlayer.X;
                        SendData.Y = CurrentPlayer.Y;

                        string JSONString = SendData.ToString();

                        PacketHeader SendHeader;

                        SendHeader.size = htons((unsigned short)JSONString.size());

                        SendHeader.type = htons((unsigned short)EPacketType::MOVE);

                        for (int j = 0; j < (int)ReadSockets.fd_count; ++j)
                        {
                            if (ReadSockets.fd_array[j] != ListenSocket)
                            {
                                int SentBytes = SendAll(ReadSockets.fd_array[j], (char*)&SendHeader, sizeof(SendHeader));

                                if (SentBytes <= 0)
                                {
                                    continue;
                                }

                                SendAll(ReadSockets.fd_array[j], JSONString.c_str(), JSONString.size());
                            }
                        }

                        break;
                    }
                    }
                }
            }
        }
    }

    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}