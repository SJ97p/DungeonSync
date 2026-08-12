#include "WinsockRuntime.h"

#include <WinSock2.h>

namespace DungeonSync::Network
{
    WinsockRuntime::WinsockRuntime() noexcept
    {
        WSADATA winsockData{};

        errorCode_ = WSAStartup(
            MAKEWORD(2, 2),
            &winsockData);

        initialized_ = errorCode_ == 0;
    }

    WinsockRuntime::~WinsockRuntime()
    {
        if (initialized_)
        {
            WSACleanup();
        }
    }

    bool WinsockRuntime::IsInitialized() const noexcept
    {
        return initialized_;
    }

    int WinsockRuntime::ErrorCode() const noexcept
    {
        return errorCode_;
    }
}