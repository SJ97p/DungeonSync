#include "../Shared/Network/WinsockRuntime.h"
#include "Network/TcpListener.h"
#include "../Shared/Network/Packet.h"

#include <iostream>
#include <cstdint>
#include <chrono>
#include <cmath>

namespace
{
    [[nodiscard]]
    bool SendAll(
        SOCKET socket,
        const void* data,
        std::size_t size) noexcept
    {
        if (data == nullptr && size > 0)
        {
            return false;
        }

        const char* bytes =
            static_cast<const char*>(data);

        std::size_t totalSentBytes = 0;

        while (totalSentBytes < size)
        {
            const int sentBytes = send(
                socket,
                bytes + totalSentBytes,
                static_cast<int>(
                    size - totalSentBytes),
                0);

            if (sentBytes == SOCKET_ERROR ||
                sentBytes == 0)
            {
                return false;
            }

            totalSentBytes +=
                static_cast<std::size_t>(
                    sentBytes);
        }

        return true;
    }
}

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

    float approvedPositionX = 0.0F;
    float approvedPositionY = 0.0F;

    auto lastApprovedTime =
        std::chrono::steady_clock::now();

    constexpr float MaxMoveSpeed = 1.5F;
    constexpr float MovementTolerance = 0.1F;

    constexpr float MinWorldX = -1.5F;
    constexpr float MaxWorldX = 1.5F;
    constexpr float MinWorldY = -0.8F;
    constexpr float MaxWorldY = 0.8F;

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

        const float positionX =
            DungeonSync::Network::DecodeFloat(
                movePacket.positionX);

        const float positionY =
            DungeonSync::Network::DecodeFloat(
                movePacket.positionY);

        bool accepted = true;
        const char* rejectionReason = nullptr;

        if (!std::isfinite(positionX) ||
            !std::isfinite(positionY))
        {
            accepted = false;
            rejectionReason = "non-finite position";
        }

        const bool insideWorldBounds =
            positionX >= MinWorldX &&
            positionX <= MaxWorldX &&
            positionY >= MinWorldY &&
            positionY <= MaxWorldY;

        if (accepted &&
            !insideWorldBounds)
        {
            accepted = false;
            rejectionReason = "out-of-bounds position";
        }

        const auto currentTime =
            std::chrono::steady_clock::now();

        if (accepted)
        {
            const std::chrono::duration<float> elapsed =
                currentTime - lastApprovedTime;

            constexpr float MaxValidationElapsedSeconds =
                0.25F;

            const float validationElapsedSeconds =
                (std::min)(
                    elapsed.count(),
                    MaxValidationElapsedSeconds);

            const float maxAllowedDistance =
                MaxMoveSpeed *
                validationElapsedSeconds +
                MovementTolerance;

            const float deltaX =
                positionX - approvedPositionX;

            const float deltaY =
                positionY - approvedPositionY;

            const float distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY;

            const float maxAllowedDistanceSquared =
                maxAllowedDistance *
                maxAllowedDistance;

            if (distanceSquared >
                maxAllowedDistanceSquared)
            {
                accepted = false;
                rejectionReason = "excessive movement";
            }
        }

        if (accepted)
        {
            approvedPositionX = positionX;
            approvedPositionY = positionY;
            lastApprovedTime = currentTime;

            std::cout
                << "Approved move"
                << " | sequence: " << sequence
                << " | position: ("
                << approvedPositionX
                << ", "
                << approvedPositionY
                << ")\n";
        }
        else
        {
            std::cerr
                << "Rejected move"
                << " | sequence: " << sequence
                << " | reason: "
                << rejectionReason
                << " | proposed: ("
                << positionX
                << ", "
                << positionY
                << ")"
                << " | approved: ("
                << approvedPositionX
                << ", "
                << approvedPositionY
                << ")\n";
        }

        lastSequence = sequence;

        const DungeonSync::Network::PlayerStatePacket
            statePacket =
            DungeonSync::Network::MakePlayerStatePacket(
                sequence,
                approvedPositionX,
                approvedPositionY,
                accepted);

        if (!SendAll(
            clientSocket,
            &statePacket,
            sizeof(statePacket)))
        {
            std::cerr
                << "Failed to send PlayerStatePacket"
                << " | error: "
                << WSAGetLastError()
                << '\n';

            closesocket(clientSocket);
            return 1;
        }

    }
}