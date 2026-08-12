#pragma once

namespace DungeonSync::Network
{
    class WinsockRuntime final
    {
    public:
        WinsockRuntime() noexcept;
        ~WinsockRuntime();

        WinsockRuntime(const WinsockRuntime&) = delete;
        WinsockRuntime& operator=(const WinsockRuntime&) = delete;
        WinsockRuntime(WinsockRuntime&&) = delete;
        WinsockRuntime& operator=(WinsockRuntime&&) = delete;

        [[nodiscard]]
        bool IsInitialized() const noexcept;

        [[nodiscard]]
        int ErrorCode() const noexcept;

    private:
        bool initialized_{};
        int errorCode_{};
    };
}