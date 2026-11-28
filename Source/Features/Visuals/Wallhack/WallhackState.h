#pragma once

#include <array>
#include <cstddef>

#include <CS2/Panorama/PanelHandle.h>

struct WallhackState {
    static constexpr std::size_t kMaxPlayers = 64;
    static constexpr std::size_t kJoints = 10;

    struct PlayerSkeleton {
        bool active{false};
        std::array<cs2::PanelHandle, kJoints> jointHandles{};
    };

    std::array<PlayerSkeleton, kMaxPlayers> skeletons{};
};