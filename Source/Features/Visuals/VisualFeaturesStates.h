#pragma once

#include "ModelGlow/ModelGlowState.h"
#include "PlayerInfoInWorld/PlayerInfoInWorldState.h"
#include "Wallhack/WallhackState.h"

struct VisualFeaturesStates {
    PlayerInfoInWorldState playerInfoInWorldState;
    ModelGlowState modelGlowState;
    WallhackState wallhackState;
};
