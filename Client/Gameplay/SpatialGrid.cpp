#include "SpatialGrid.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace DungeonSync::Gameplay
{
    SpatialGrid::SpatialGrid(float cellSize)
        : cellSize_(cellSize)
    {
        if (cellSize_ <= 0.0F)
        {
            throw std::invalid_argument(
                "SpatialGrid cell size must be positive.");
        }
    }

    void SpatialGrid::Rebuild(
        std::span<const Monster> monsters)
    {
        cells_.clear();

        for (std::size_t index = 0;
            index < monsters.size();
            ++index)
        {
            const Monster& monster = monsters[index];

            if (!monster.alive)
            {
                continue;
            }

            const CellCoordinate cell =
                ToCell(monster.position);

            cells_[MakeCellKey(cell.x, cell.y)]
                .push_back(index);
        }
    }

    std::vector<std::size_t> SpatialGrid::Query(
        const DirectX::XMFLOAT2& center,
        float radius) const
    {
        std::vector<std::size_t> candidates;

        if (radius < 0.0F)
        {
            return candidates;
        }

        const DirectX::XMFLOAT2 minimum{
            center.x - radius,
            center.y - radius
        };

        const DirectX::XMFLOAT2 maximum{
            center.x + radius,
            center.y + radius
        };

        const CellCoordinate minimumCell =
            ToCell(minimum);

        const CellCoordinate maximumCell =
            ToCell(maximum);

        for (std::int32_t y = minimumCell.y;
            y <= maximumCell.y;
            ++y)
        {
            for (std::int32_t x = minimumCell.x;
                x <= maximumCell.x;
                ++x)
            {
                const auto foundCell =
                    cells_.find(MakeCellKey(x, y));

                if (foundCell == cells_.end())
                {
                    continue;
                }

                candidates.insert(
                    candidates.end(),
                    foundCell->second.begin(),
                    foundCell->second.end());
            }
        }

        return candidates;
    }

    float SpatialGrid::CellSize() const noexcept
    {
        return cellSize_;
    }

    SpatialGrid::CellCoordinate
        SpatialGrid::ToCell(
            const DirectX::XMFLOAT2& position) const
    {
        return CellCoordinate{
            static_cast<std::int32_t>(
                std::floor(position.x / cellSize_)),
            static_cast<std::int32_t>(
                std::floor(position.y / cellSize_))
        };
    }

    std::int64_t SpatialGrid::MakeCellKey(
        std::int32_t x,
        std::int32_t y) noexcept
    {
        const auto unsignedX =
            static_cast<std::uint32_t>(x);

        const auto unsignedY =
            static_cast<std::uint32_t>(y);

        return
            (static_cast<std::int64_t>(unsignedX) << 32) |
            static_cast<std::int64_t>(unsignedY);
    }
}