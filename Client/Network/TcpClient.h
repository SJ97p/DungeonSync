#pragma once

#include <WinSock2.h>

#include <cstdint>
#include <string_view>

namespace DungeonSync::Network
{
    class TcpClient final
    {
    public:
        TcpClient() noexcept = default;
        ~TcpClient();

        TcpClient(const TcpClient&) = delete;
        TcpClient& operator=(const TcpClient&) = delete;
        TcpClient(TcpClient&&) = delete;
        TcpClient& operator=(TcpClient&&) = delete;

        [[nodiscard]]
        bool Connect(
            std::string_view address,
            std::uint16_t port) noexcept;

        [[nodiscard]]
        bool Send(std::string_view message) noexcept;

        [[nodiscard]]
        bool IsConnected() const noexcept;

        [[nodiscard]]
        int ErrorCode() const noexcept;

        void Disconnect() noexcept;

    private:
        SOCKET socket_{ INVALID_SOCKET };
        int errorCode_{};
    };
}