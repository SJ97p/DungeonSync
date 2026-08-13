#include "ServerStateReceiver.h"

#include "../../Shared/Network/Packet.h"

#include <Windows.h>

namespace DungeonSync::Network
{
    ServerStateReceiver::ServerStateReceiver(
        TcpClient& tcpClient) noexcept
        : tcpClient_(tcpClient)
    {
    }

    ServerStateReceiver::~ServerStateReceiver()
    {
        Stop();
    }

    bool ServerStateReceiver::Start()
    {
        if (running_.load() ||
            !tcpClient_.IsConnected())
        {
            return false;
        }

        running_.store(true);

        try
        {
            receiveThread_ = std::thread(
                &ServerStateReceiver::ReceiveLoop,
                this);
        }
        catch (...)
        {
            running_.store(false);
            return false;
        }

        return true;
    }

    void ServerStateReceiver::Stop() noexcept
    {
        running_.store(false);

        if (receiveThread_.joinable())
        {
            tcpClient_.Shutdown();
            receiveThread_.join();
        }
    }

    bool ServerStateReceiver::TryConsumeLatest(
        PlayerStateSnapshot& destination)
    {
        const std::scoped_lock lock{
            stateMutex_ };

        if (!hasNewState_)
        {
            return false;
        }

        destination = latestState_;
        hasNewState_ = false;

        return true;
    }

    void ServerStateReceiver::ReceiveLoop() noexcept
    {
        while (running_.load())
        {
            PlayerStatePacket packet{};

            if (!tcpClient_.Receive(
                &packet,
                sizeof(packet)))
            {
                break;
            }

            const std::uint16_t packetSize =
                ntohs(packet.size);

            const PacketType packetType =
                static_cast<PacketType>(
                    ntohs(packet.type));

            if (packetSize != sizeof(packet) ||
                packetType !=
                PacketType::PlayerState)
            {
                OutputDebugStringA(
                    "Received invalid PlayerStatePacket.\n");

                break;
            }

            PlayerStateSnapshot snapshot{};

            snapshot.sequence =
                ntohl(packet.sequence);

            snapshot.positionX =
                DecodeFloat(packet.positionX);

            snapshot.positionY =
                DecodeFloat(packet.positionY);

            snapshot.accepted =
                ntohl(packet.accepted) != 0;

            {
                const std::scoped_lock lock{
                    stateMutex_ };

                latestState_ = snapshot;
                hasNewState_ = true;
            }
        }

        running_.store(false);
    }
}