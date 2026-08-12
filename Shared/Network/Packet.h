#pragma once

#include <WinSock2.h>

#include <bit>
#include <cstdint>

namespace DungeonSync::Network
{
    enum class PacketType : std::uint16_t
    {
        PlayerMove = 1
    };

    struct PlayerMovePacket
    {
        std::uint16_t size{};
        std::uint16_t type{};
        std::uint32_t sequence{};
        std::uint32_t positionX{};
        std::uint32_t positionY{};
    };

    static_assert(
        sizeof(PlayerMovePacket) == 16,
        "PlayerMovePacket must be exactly 16 bytes.");

    [[nodiscard]]
    inline std::uint32_t EncodeFloat(float value) noexcept
    {
        const std::uint32_t bits =
            std::bit_cast<std::uint32_t>(value);

        return htonl(bits);
    }

    [[nodiscard]]
    inline float DecodeFloat(
        std::uint32_t networkValue) noexcept
    {
        const std::uint32_t hostBits =
            ntohl(networkValue);

        return std::bit_cast<float>(hostBits);
    }

    [[nodiscard]]
    inline PlayerMovePacket MakePlayerMovePacket(
        std::uint32_t sequence,
        float positionX,
        float positionY) noexcept
    {
        PlayerMovePacket packet{};

        packet.size = htons(
            static_cast<std::uint16_t>(
                sizeof(PlayerMovePacket)));

        packet.type = htons(
            static_cast<std::uint16_t>(
                PacketType::PlayerMove));

        packet.sequence = htonl(sequence);
        packet.positionX = EncodeFloat(positionX);
        packet.positionY = EncodeFloat(positionY);

        return packet;
    }
}