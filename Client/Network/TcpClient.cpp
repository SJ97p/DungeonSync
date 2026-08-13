#include "TcpClient.h"

#include <WS2tcpip.h>

#include <string>

namespace DungeonSync::Network
{
    TcpClient::~TcpClient()
    {
        Disconnect();
    }

    bool TcpClient::Connect(
        std::string_view address,
        std::uint16_t port) noexcept
    {
        Disconnect();
        errorCode_ = 0;

        socket_ = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP);

        if (socket_ == INVALID_SOCKET)
        {
            errorCode_ = WSAGetLastError();
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);

        const std::string nullTerminatedAddress{ address };

        if (inet_pton(
            AF_INET,
            nullTerminatedAddress.c_str(),
            &serverAddress.sin_addr) != 1)
        {
            errorCode_ = WSAGetLastError();
            Disconnect();
            return false;
        }

        if (connect(
            socket_,
            reinterpret_cast<const sockaddr*>(
                &serverAddress),
            sizeof(serverAddress)) == SOCKET_ERROR)
        {
            errorCode_ = WSAGetLastError();
            Disconnect();
            return false;
        }

        return true;
    }

    bool TcpClient::Send(
        const void* data,
        std::size_t size) noexcept
    {
        if (!IsConnected())
        {
            errorCode_ = WSAENOTCONN;
            return false;
        }

        if (data == nullptr && size > 0)
        {
            errorCode_ = WSAEINVAL;
            return false;
        }

        const char* bytes =
            static_cast<const char*>(data);

        std::size_t totalSentBytes = 0;

        while (totalSentBytes < size)
        {
            const int sentBytes = send(
                socket_,
                bytes + totalSentBytes,
                static_cast<int>(
                    size - totalSentBytes),
                0);

            if (sentBytes == SOCKET_ERROR)
            {
                errorCode_ = WSAGetLastError();
                return false;
            }

            if (sentBytes == 0)
            {
                errorCode_ = WSAECONNRESET;
                return false;
            }

            totalSentBytes +=
                static_cast<std::size_t>(sentBytes);
        }

        return true;
    }

    bool TcpClient::Receive(
        void* destination,
        std::size_t size) noexcept
    {
        if (!IsConnected())
        {
            errorCode_ = WSAENOTCONN;
            return false;
        }

        if (destination == nullptr && size > 0)
        {
            errorCode_ = WSAEINVAL;
            return false;
        }

        char* bytes =
            static_cast<char*>(destination);

        std::size_t totalReceivedBytes = 0;

        while (totalReceivedBytes < size)
        {
            const int receivedBytes = recv(
                socket_,
                bytes + totalReceivedBytes,
                static_cast<int>(
                    size - totalReceivedBytes),
                0);

            if (receivedBytes == SOCKET_ERROR)
            {
                errorCode_ = WSAGetLastError();
                return false;
            }

            if (receivedBytes == 0)
            {
                errorCode_ = 0;
                return false;
            }

            totalReceivedBytes +=
                static_cast<std::size_t>(
                    receivedBytes);
        }

        return true;
    }

    bool TcpClient::IsConnected() const noexcept
    {
        return socket_ != INVALID_SOCKET;
    }

    int TcpClient::ErrorCode() const noexcept
    {
        return errorCode_.load();
    }

    void TcpClient::Shutdown() noexcept
    {
        if (socket_ == INVALID_SOCKET)
        {
            return;
        }

        shutdown(
            socket_,
            SD_BOTH);
    }

    void TcpClient::Disconnect() noexcept
    {
        if (socket_ == INVALID_SOCKET)
        {
            return;
        }

        shutdown(
            socket_,
            SD_BOTH);

        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}