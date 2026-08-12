#include "../Shared/Network/WinsockRuntime.h"
#include "Network/TcpListener.h"

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
    std::cout << "Waiting for HELLO message...\n";

    constexpr int HelloMessageSize = 5;
    char receiveBuffer[HelloMessageSize + 1]{};

    int totalReceivedBytes = 0;

    while (totalReceivedBytes < HelloMessageSize)
    {
        const int receivedBytes = recv(
            clientSocket,
            receiveBuffer + totalReceivedBytes,
            HelloMessageSize - totalReceivedBytes,
            0);

        if (receivedBytes == SOCKET_ERROR)
        {
            std::cerr
                << "Failed to receive data. Error: "
                << WSAGetLastError()
                << '\n';

            closesocket(clientSocket);
            return 1;
        }

        if (receivedBytes == 0)
        {
            std::cerr
                << "Client disconnected before sending HELLO.\n";

            closesocket(clientSocket);
            return 1;
        }

        totalReceivedBytes += receivedBytes;
    }

    std::cout
        << "Received from client: "
        << receiveBuffer
        << '\n';

    std::cout << "Press Enter to disconnect the client.\n";
    std::cin.get();

    closesocket(clientSocket);

    return 0;
}