#include "TcpListener.h"

#include <WS2tcpip.h>

namespace DungeonSync::Server
{
    TcpListener::~TcpListener()
    {
        Close();
    }

    bool TcpListener::Start(std::uint16_t port) noexcept
    {
        Close();
        errorCode_ = 0;

        listenSocket_ = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP);

        if (listenSocket_ == INVALID_SOCKET)
        {
            errorCode_ = WSAGetLastError();
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);

        if (bind(
            listenSocket_,
            reinterpret_cast<const sockaddr*>(
                &serverAddress),
            sizeof(serverAddress)) == SOCKET_ERROR)
        {
            errorCode_ = WSAGetLastError();
            Close();
            return false;
        }

        if (listen(
            listenSocket_,
            SOMAXCONN) == SOCKET_ERROR)
        {
            errorCode_ = WSAGetLastError();
            Close();
            return false;
        }

        return true;
    }

    SOCKET TcpListener::Accept() noexcept
    {
        errorCode_ = 0;

        const SOCKET clientSocket = accept(
            listenSocket_,
            nullptr,
            nullptr);

        if (clientSocket == INVALID_SOCKET)
        {
            errorCode_ = WSAGetLastError();
        }

        return clientSocket;
    }

    int TcpListener::ErrorCode() const noexcept
    {
        return errorCode_;
    }

    void TcpListener::Close() noexcept
    {
        if (listenSocket_ == INVALID_SOCKET)
        {
            return;
        }

        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }
}