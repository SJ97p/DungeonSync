#include "../Shared/Network/WinsockRuntime.h"
#include "Network/TcpListener.h"
#include "../Shared/Network/Packet.h"

#include <iostream>
#include <cstdint>

int main()
{
    DungeonSync::Network::WinsockRuntime winsock;

    if (!winsock.IsInitialized())
    {
        std::cerr
            << "Failed to initialize Winsock. Error: "
            << winsock.ErrorCode()
            << '\n';

        return 1;
    }

    constexpr std::uint16_t ServerPort = 27015;

    DungeonSync::Server::TcpListener listener;

    if (!listener.Start(ServerPort))
    {
        std::cerr
            << "Failed to start TCP listener. Error: "
            << listener.ErrorCode()
            << '\n';

        return 1;
    }

    std::cout
        << "DungeonSync Server is listening on port "
        << ServerPort
        << ".\n";

    std::cout << "Waiting for one client...\n";

    const SOCKET clientSocket = listener.Accept();

    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr
            << "Failed to accept client. Error: "
            << listener.ErrorCode()
            << '\n';

        return 1;
    }

    std::cout << "Client connected successfully.\n";
    std::cout << "Receiving PlayerMovePacket stream...\n";

    std::uint32_t lastSequence = 0;

    while (true)
    {
        DungeonSync::Network::PlayerMovePacket movePacket{};

        char* packetBytes =
            reinterpret_cast<char*>(&movePacket);

        int totalReceivedBytes = 0;

        while (totalReceivedBytes <
            static_cast<int>(sizeof(movePacket)))
        {
            const int receivedBytes = recv(
                clientSocket,
                packetBytes + totalReceivedBytes,
                static_cast<int>(sizeof(movePacket)) -
                totalReceivedBytes,
                0);

            if (receivedBytes == SOCKET_ERROR)
            {
                std::cerr
                    << "Failed to receive packet. Error: "
                    << WSAGetLastError()
                    << '\n';

                closesocket(clientSocket);
                return 1;
            }

            if (receivedBytes == 0)
            {
                std::cout << "Client disconnected.\n";

                closesocket(clientSocket);
                return 0;
            }

            totalReceivedBytes += receivedBytes;
        }

        const std::uint16_t packetSize =
            ntohs(movePacket.size);

        const auto packetType =
            static_cast<DungeonSync::Network::PacketType>(
                ntohs(movePacket.type));

        if (packetSize != sizeof(movePacket) ||
            packetType !=
            DungeonSync::Network::PacketType::PlayerMove)
        {
            std::cerr << "Received an invalid packet.\n";

            closesocket(clientSocket);
            return 1;
        }

        const std::uint32_t sequence =
            ntohl(movePacket.sequence);

        if (sequence <= lastSequence)
        {
            std::cerr
                << "Ignored stale move packet. Sequence: "
                << sequence
                << '\n';

            continue;
        }

        lastSequence = sequence;

        const float positionX =
            DungeonSync::Network::DecodeFloat(
                movePacket.positionX);

        const float positionY =
            DungeonSync::Network::DecodeFloat(
                movePacket.positionY);

        std::cout
            << "Move"
            << " | sequence: " << sequence
            << " | position: ("
            << positionX
            << ", "
            << positionY
            << ")\n";
    }
}