#pragma once

#include <WinSock2.h>

#include <cstdint>

namespace DungeonSync::Server
{
    class TcpListener final
    {
    public:
        TcpListener() noexcept = default;
        ~TcpListener();

        TcpListener(const TcpListener&) = delete;
        TcpListener& operator=(const TcpListener&) = delete;
        TcpListener(TcpListener&&) = delete;
        TcpListener& operator=(TcpListener&&) = delete;

        [[nodiscard]]
        bool Start(std::uint16_t port) noexcept;

        [[nodiscard]]
        SOCKET Accept() noexcept;

        [[nodiscard]]
        int ErrorCode() const noexcept;

    private:
        void Close() noexcept;

        SOCKET listenSocket_{ INVALID_SOCKET };
        int errorCode_{};
    };
}