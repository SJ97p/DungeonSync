#pragma once

#include "Monster.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace DungeonSync::Gameplay
{
    class SpatialGrid final
    {
    public:
        explicit SpatialGrid(float cellSize);

        void Rebuild(
            std::span<const Monster> monsters);

        [[nodiscard]]
        std::vector<std::size_t> Query(
            const DirectX::XMFLOAT2& center,
            float radius) const;

        [[nodiscard]]
        float CellSize() const noexcept;

    private:
        struct CellCoordinate
        {
            std::int32_t x{};
            std::int32_t y{};
        };

        [[nodiscard]]
        CellCoordinate ToCell(
            const DirectX::XMFLOAT2& position) const;

        [[nodiscard]]
        static std::int64_t MakeCellKey(
            std::int32_t x,
            std::int32_t y) noexcept;

        float cellSize_{};

        std::unordered_map<
            std::int64_t,
            std::vector<std::size_t>>
            cells_;
    };
}