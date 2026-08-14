#include "DemoScene.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cstdio>
#include <Windows.h>
#include <vector>

namespace DungeonSync::Presentation
{
    DemoScene::DemoScene() 
        : spatialGrid_(0.5F),
        dungeonController_()
    {
        PrepareCurrentRoom();
        (void)dungeonController_.ConsumeRoomChanged();
    }

    void DemoScene::PrepareCurrentRoom()
    {
        const Gameplay::DungeonRoom& room =
            dungeonController_.CurrentRoom();

        constexpr std::size_t ColumnCount = 10;
        constexpr float Spacing = 0.32F;

        const std::size_t activeMonsterCount =
            (std::min)(
                room.monsterCount,
                monsters_.size());

        const std::size_t rowCount =
            (activeMonsterCount +
                ColumnCount - 1) /
            ColumnCount;

        const float startX =
            -static_cast<float>(
                (std::min)(
                    activeMonsterCount,
                    ColumnCount) - 1) *
            Spacing *
            0.5F;

        const float startY =
            -static_cast<float>(
                rowCount - 1) *
            Spacing *
            0.5F;

        for (std::size_t index = 0;
            index < monsters_.size();
            ++index)
        {
            Gameplay::Monster& monster =
                monsters_[index];

            if (index >= activeMonsterCount)
            {
                monster.health = 0.0F;
                monster.alive = false;
                continue;
            }

            const std::size_t column =
                index % ColumnCount;

            const std::size_t row =
                index / ColumnCount;

            monster.position =
                DirectX::XMFLOAT2{
                    startX +
                        static_cast<float>(column) *
                        Spacing,
                    startY +
                        static_cast<float>(row) *
                        Spacing
            };

            monster.health = room.monsterHealth;
            monster.alive = true;
        }

        spatialGrid_.Rebuild(monsters_);

        char message[128]{};

        std::snprintf(
            message,
            sizeof(message),
            "Prepared Room %zu"
            " | monsters: %zu"
            " | health: %.0f\n",
            dungeonController_.CurrentRoomIndex() + 1,
            activeMonsterCount,
            room.monsterHealth);

        OutputDebugStringA(message);
    }

    void DemoScene::Update(
        float deltaSeconds,
        float moveX,
        float moveY,
        bool jumpPressed,
        bool attackPressed,
        bool coneAttackPressed)
    {
        if (dungeonController_.ConsumeRoomChanged())
        {
            PrepareCurrentRoom();
        }

        const float simulationDeltaSeconds =
            (std::min)(deltaSeconds, 0.05F);

        attackEffectRemainingSeconds_ =
            (std::max)(
                0.0F,
                attackEffectRemainingSeconds_ -
                simulationDeltaSeconds);

        constexpr float PlayerMoveSpeed = 1.5F;

        const float movementLengthSquared =
            moveX * moveX +
            moveY * moveY;

        if (movementLengthSquared > 0.0F)
        {
            playerFacing_.x = moveX;
            playerFacing_.y = moveY;
        }

        if (moveX < 0.0F)
        {
            playerVisualFacingX_ = -1.0F;
        }
        else if (moveX > 0.0F)
        {
            playerVisualFacingX_ = 1.0F;
        }

        playerPosition_.x +=
            moveX *
            PlayerMoveSpeed *
            simulationDeltaSeconds;

        playerPosition_.y +=
            moveY *
            PlayerMoveSpeed *
            simulationDeltaSeconds;

        constexpr float MovementLimitX = 1.5F;
        constexpr float MovementLimitDepth = 1.5F;

        playerPosition_.x = std::clamp(
            playerPosition_.x,
            -MovementLimitX,
            MovementLimitX);

        playerPosition_.y = std::clamp(
            playerPosition_.y,
            -MovementLimitDepth,
            MovementLimitDepth);

        constexpr float JumpInitialVelocity = 2.8F;
        constexpr float Gravity = -8.0F;

        if (jumpPressed &&
            playerIsGrounded_)
        {
            playerVerticalVelocity_ =
                JumpInitialVelocity;

            playerIsGrounded_ = false;
        }

        if (!playerIsGrounded_)
        {
            playerVerticalVelocity_ +=
                Gravity *
                simulationDeltaSeconds;

            playerJumpHeight_ +=
                playerVerticalVelocity_ *
                simulationDeltaSeconds;

            if (playerJumpHeight_ <= 0.0F)
            {
                playerJumpHeight_ = 0.0F;
                playerVerticalVelocity_ = 0.0F;
                playerIsGrounded_ = true;
            }
        }

        const Gameplay::RoomState stateBeforeUpdate =
            dungeonController_.CurrentRoom().state;

        dungeonController_.Update(
            playerPosition_,
            monsters_);

        const Gameplay::RoomState stateAfterUpdate =
            dungeonController_.CurrentRoom().state;

        if (stateBeforeUpdate ==
            Gameplay::RoomState::Ready &&
            stateAfterUpdate ==
            Gameplay::RoomState::Combat)
        {
            char roomMessage[128]{};

            std::snprintf(
                roomMessage,
                sizeof(roomMessage),
                "Room %zu entered Combat state.\n",
                dungeonController_.CurrentRoomIndex() + 1);

            OutputDebugStringA(roomMessage);
        }

        //const std::size_t previousRoomIndex =
        //    dungeonController_.CurrentRoomIndex();

        //const Gameplay::RoomState previousRoomState =
        //    dungeonController_
        //    .CurrentRoom()
        //    .state;

        //dungeonController_.Update(
        //    playerPosition_,
        //    monsters_);

        //const Gameplay::RoomState currentRoomState =
        //    dungeonController_
        //    .CurrentRoom()
        //    .state;

        //if (previousRoomState != currentRoomState)
        //{
        //    char roomMessage[128]{};

        //    if (currentRoomState ==
        //        Gameplay::RoomState::Combat)
        //    {
        //        std::snprintf(
        //            roomMessage,
        //            sizeof(roomMessage),
        //            "Room %zu entered Combat state.\n",
        //            previousRoomIndex + 1);

        //        OutputDebugStringA(roomMessage);
        //    }
        //    else if (currentRoomState ==
        //        Gameplay::RoomState::Cleared)
        //    {
        //        std::snprintf(
        //            roomMessage,
        //            sizeof(roomMessage),
        //            "Room %zu cleared.\n",
        //            previousRoomIndex + 1);

        //        OutputDebugStringA(roomMessage);
        //    }
        //}

        //Attack after Spacebar
        if (attackPressed &&
            playerIsGrounded_)
        {
            attackEffectRemainingSeconds_ = 0.15F;
            attackEffectPosition_ = playerPosition_;
            attackEffectFacingX_ = playerVisualFacingX_;

            constexpr float AttackRange = 0.45F;
            constexpr float AttackDamage = 50.0F;

            const std::vector<std::size_t> candidates =
                spatialGrid_.Query(
                    playerPosition_,
                    AttackRange);

            lastAttackResult_ =
                combatSystem_.ApplyAreaAttackToCandidates(
                    playerPosition_,
                    AttackRange,
                    AttackDamage,
                    monsters_,
                    candidates);

            char debugMessage[256]{};

            std::snprintf(
                debugMessage,
                sizeof(debugMessage),
                "Grid attack - candidates: %zu, examined: %zu, hits: %zu, elapsed: %lld ns\n",
                candidates.size(),
                lastAttackResult_.examinedCount,
                lastAttackResult_.hitCount,
                static_cast<long long>(
                    lastAttackResult_.elapsedNanoseconds));

            OutputDebugStringA(debugMessage);

            dungeonController_.Update(
                playerPosition_,
                monsters_);

        }

        //Attack after E
        if (coneAttackPressed &&
            playerIsGrounded_)
        {
            constexpr float ConeAttackRange = 0.9F;
            constexpr float ConeAttackDamage = 50.0F;

            constexpr float Pi =
                3.14159265F;

            constexpr float HalfAngleRadians =
                45.0F * Pi / 180.0F;

            const std::vector<std::size_t> candidates =
                spatialGrid_.Query(
                    playerPosition_,
                    ConeAttackRange);

            lastAttackResult_ =
                combatSystem_.ApplyConeAttackToCandidates(
                    playerPosition_,
                    playerFacing_,
                    ConeAttackRange,
                    HalfAngleRadians,
                    ConeAttackDamage,
                    monsters_,
                    candidates);

            char debugMessage[256]{};

            std::snprintf(
                debugMessage,
                sizeof(debugMessage),
                "Cone attack"
                " | candidates: %zu"
                " | examined: %zu"
                " | hits: %zu"
                " | elapsed: %lld ns\n",
                candidates.size(),
                lastAttackResult_.examinedCount,
                lastAttackResult_.hitCount,
                static_cast<long long>(
                    lastAttackResult_
                    .elapsedNanoseconds));

            OutputDebugStringA(debugMessage);

            dungeonController_.Update(
                playerPosition_,
                monsters_);
        }

        std::size_t clearedRoomIndex = 0;

        if (dungeonController_.ConsumeClearedRoom(
            clearedRoomIndex))
        {
            char roomMessage[128]{};

            std::snprintf(
                roomMessage,
                sizeof(roomMessage),
                "Room %zu cleared.\n",
                clearedRoomIndex + 1);

            OutputDebugStringA(roomMessage);
        }

        if (dungeonController_.ConsumeDungeonCleared())
        {
            OutputDebugStringA(
                "Dungeon cleared. Press R to restart.\n");
        }

        camera_.position =
            DirectX::XMFLOAT3{
                playerPosition_.x,
                3.0F,
                -5.0F
        };

        camera_.target =
            DirectX::XMFLOAT3{
                playerPosition_.x,
                0.7F,
                0.0F
        };

        visibleRenderItemCount_ = 0;

        const DirectX::XMMATRIX playerWorld =
            DirectX::XMMatrixScaling(
                0.70F,
                0.70F,
                1.0F) *
            DirectX::XMMatrixTranslation(
                playerPosition_.x,
                playerJumpHeight_,
                playerPosition_.y);

        DirectX::XMStoreFloat4x4(
            &renderItems_[0].world,
            playerWorld);

        renderItems_[0].tintColor =
            DirectX::XMFLOAT4{
                1.0F,
                1.0F,
                1.0F,
                1.0F
        };

        if (playerVisualFacingX_ > 0.0F)
        {
            renderItems_[0].uvRectangle =
                DirectX::XMFLOAT4{
                    0.0F,
                    0.0F,
                    0.5F,
                    0.5F
            };
        }
        else
        {
            renderItems_[0].uvRectangle =
                DirectX::XMFLOAT4{
                    0.5F,
                    0.0F,
                    -0.5F,
                    0.5F
            };
        }

        ++visibleRenderItemCount_;

        for (const Gameplay::Monster& monster :
            monsters_)
        {
            if (!monster.alive)
            {
                continue;
            }

            Rendering::RenderItem& renderItem =
                renderItems_[visibleRenderItemCount_];

            renderItem.uvRectangle =
                DirectX::XMFLOAT4{
                    0.5F,
                    0.0F,
                    0.5F,
                    0.5F
            };

            const DirectX::XMFLOAT2& monsterPosition =
                monster.position;

            const DirectX::XMMATRIX monsterWorld =
                DirectX::XMMatrixScaling(
                    0.42F,
                    0.42F,
                    1.0F) *
                DirectX::XMMatrixTranslation(
                    monsterPosition.x,
                    0.0F,
                    monsterPosition.y);

            DirectX::XMStoreFloat4x4(
                &renderItem.world,
                monsterWorld);

            if (monster.health < 100.0F)
            {
                renderItem.tintColor =
                    DirectX::XMFLOAT4{
                        1.0F,
                        0.55F,
                        0.55F,
                        1.0F
                };
            }
            else
            {
                renderItem.tintColor =
                    DirectX::XMFLOAT4{
                        1.0F,
                        1.0F,
                        1.0F,
                        1.0F
                };
            }

            ++visibleRenderItemCount_;
        }

        if (attackEffectRemainingSeconds_ > 0.0F)
        {
            Rendering::RenderItem& effectItem =
                renderItems_[visibleRenderItemCount_];

            const float horizontalDirection =
                attackEffectFacingX_;

            const DirectX::XMMATRIX effectWorld =
                DirectX::XMMatrixScaling(
                    0.85F,
                    0.85F,
                    1.0F) *
                DirectX::XMMatrixTranslation(
                    attackEffectPosition_.x +
                    horizontalDirection * 0.28F,
                    0.08F,
                    attackEffectPosition_.y -
                    0.02F);

            DirectX::XMStoreFloat4x4(
                &effectItem.world,
                effectWorld);

            effectItem.tintColor =
                DirectX::XMFLOAT4{
                    1.0F,
                    0.85F,
                    0.45F,
                    0.85F
            };

            if (horizontalDirection > 0.0F)
            {
                effectItem.uvRectangle =
                    DirectX::XMFLOAT4{
                        0.75F,
                        0.75F,
                        -0.25F,
                        0.25F
                };
            }
            else
            {
                effectItem.uvRectangle =
                    DirectX::XMFLOAT4{
                        0.5F,
                        0.75F,
                        0.25F,
                        0.25F
                };
            }

            ++visibleRenderItemCount_;
        }
    }

    void DemoScene::RestartDungeon()
    {
        dungeonController_.Restart();

        playerPosition_ =
            DirectX::XMFLOAT2{
                0.0F,
                0.0F
        };

        playerVisualFacingX_ = 1.0F;
        attackEffectFacingX_ = 1.0F;
        playerJumpHeight_ = 0.0F;
        playerVerticalVelocity_ = 0.0F;
        playerIsGrounded_ = true;
        attackEffectRemainingSeconds_ = 0.0F;

        PrepareCurrentRoom();

        (void)dungeonController_
            .ConsumeRoomChanged();

        OutputDebugStringA(
            "Dungeon restarted.\n");
    }

    const DirectX::XMFLOAT2&
        DemoScene::PlayerPosition() const noexcept
    {
        return playerPosition_;
    }

    void DemoScene::ReconcilePlayerPosition(
        float serverPositionX,
        float serverPositionY,
        bool accepted,
        float deltaSeconds) noexcept
    {
        const float deltaX =
            serverPositionX - playerPosition_.x;

        const float deltaY =
            serverPositionY - playerPosition_.y;

        const float errorDistanceSquared =
            deltaX * deltaX +
            deltaY * deltaY;

        constexpr float SnapDistance = 0.3F;
        constexpr float SnapDistanceSquared =
            SnapDistance * SnapDistance;

        if (!accepted ||
            errorDistanceSquared >
            SnapDistanceSquared)
        {
            playerPosition_.x = serverPositionX;
            playerPosition_.y = serverPositionY;

            return;
        }

        constexpr float CorrectionSpeed = 8.0F;

        const float correctionRatio =
            (std::min)(
                CorrectionSpeed * deltaSeconds,
                1.0F);

        playerPosition_.x +=
            deltaX * correctionRatio;

        playerPosition_.y +=
            deltaY * correctionRatio;
    }

    std::size_t
        DemoScene::CurrentRoomNumber() const noexcept
    {
        return
            dungeonController_.CurrentRoomIndex() + 1;
    }

    std::size_t
        DemoScene::RoomCount() const noexcept
    {
        return dungeonController_.RoomCount();
    }

    std::size_t
        DemoScene::AliveMonsterCount() const noexcept
    {
        return
            dungeonController_.AliveMonsterCount(
                monsters_);
    }

    bool DemoScene::IsDungeonCleared() const noexcept
    {
        return dungeonController_.IsDungeonCleared();
    }

    const Rendering::Camera&
        DemoScene::GetCamera() const noexcept
    {
        return camera_;
    }

    std::span<const Rendering::RenderItem>
        DemoScene::RenderItems() const noexcept
    {
        return std::span<const Rendering::RenderItem>{
            renderItems_.data(),
                visibleRenderItemCount_
        };
    }
}