#pragma once

#include "TcpClient.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

namespace DungeonSync::Network
{
    struct PlayerStateSnapshot
    {
        std::uint32_t sequence{};
        float positionX{};
        float positionY{};
        bool accepted{};
    };

    class ServerStateReceiver final
    {
    public:
        explicit ServerStateReceiver(
            TcpClient& tcpClient) noexcept;

        ~ServerStateReceiver();

        ServerStateReceiver(
            const ServerStateReceiver&) = delete;

        ServerStateReceiver& operator=(
            const ServerStateReceiver&) = delete;

        ServerStateReceiver(
            ServerStateReceiver&&) = delete;

        ServerStateReceiver& operator=(
            ServerStateReceiver&&) = delete;

        [[nodiscard]]
        bool Start();

        void Stop() noexcept;

        [[nodiscard]]
        bool TryConsumeLatest(
            PlayerStateSnapshot& destination);

    private:
        void ReceiveLoop() noexcept;

        TcpClient& tcpClient_;

        std::thread receiveThread_;
        std::atomic_bool running_{ false };

        std::mutex stateMutex_;
        PlayerStateSnapshot latestState_{};
        bool hasNewState_{};
    };
}