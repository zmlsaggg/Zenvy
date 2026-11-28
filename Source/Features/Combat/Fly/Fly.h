#pragma once

#include <cmath>
#include <cstdint>

#include <SDL/SdlFunctions.h>

#include <CS2/Classes/Vector.h>
#include <Features/Common/InWorldPanels.h>
#include <GameClient/Entities/PlayerPawn.h>
#include <Config/ConfigVariable.h>

#include "FlyConfigVariables.h"
#include "FlyState.h"

template <typename HookContext>
class Fly {
public:
    explicit Fly(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void run() const noexcept
    {
        if (!enabled())
            return;

        auto&& playerPawn = hookContext.activeLocalPlayerPawn();
        if (!playerPawn || !playerPawn.isAlive().valueOr(false))
            return;

        const auto absOrigin = playerPawn.absOrigin();
        if (!absOrigin.hasValue())
            return;

        const auto currentVelocity = playerPawn.velocity().valueOr(cs2::Vector{});
        const auto targetVelocity = calculateTargetVelocity(playerPawn);

        auto& stored = state().velocity;
        stored = lerpVelocity(currentVelocity, targetVelocity, accelerationFactor());
        playerPawn.setVelocity(stored);
    }

private:
    [[nodiscard]] static constexpr float degToRad(float deg) noexcept
    {
        return deg * 3.14159265358979f / 180.0f;
    }

    [[nodiscard]] bool enabled() const noexcept
    {
        return GET_CONFIG_VAR(fly_vars::Enabled);
    }

    [[nodiscard]] float speedMultiplier() const noexcept
    {
        const auto value = GET_CONFIG_VAR(fly_vars::SpeedMultiplier);
        return value < 0.1f ? 0.1f : (value > 5.0f ? 5.0f : value);
    }

    [[nodiscard]] float speed() const noexcept
    {
        return 300.0f * speedMultiplier();
    }

    [[nodiscard]] float verticalSpeed() const noexcept
    {
        return 200.0f * speedMultiplier();
    }

    [[nodiscard]] static constexpr float accelerationFactor() noexcept
    {
        return 0.1f;
    }

    [[nodiscard]] cs2::Vector calculateTargetVelocity(auto&& playerPawn) const noexcept
    {
        cs2::Vector target{};

        const auto angles = playerPawn.eyeAngles().valueOr(cs2::Vector{});
        const auto pitch = degToRad(angles.x);
        const auto yaw = degToRad(angles.y);

        const cs2::Vector forward{std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), -std::sin(pitch)};
        const cs2::Vector right{std::sin(yaw), -std::cos(yaw), 0.0f};

        const auto& input = hookContext.inputState();
        if (input.keyDown(sdl3::SDL_SCANCODE_W)) target += forward * speed();
        if (input.keyDown(sdl3::SDL_SCANCODE_S)) target += forward * (-speed());
        if (input.keyDown(sdl3::SDL_SCANCODE_D)) target += right * speed();
        if (input.keyDown(sdl3::SDL_SCANCODE_A)) target += right * (-speed());
        if (input.keyDown(sdl3::SDL_SCANCODE_SPACE)) target.z += verticalSpeed();
        if (input.keyDown(sdl3::SDL_SCANCODE_LSHIFT)) target.z -= verticalSpeed();

        return target;
    }

    [[nodiscard]] static cs2::Vector lerpVelocity(const cs2::Vector& from, const cs2::Vector& to, float factor) noexcept
    {
        return cs2::Vector{
            from.x + (to.x - from.x) * factor,
            from.y + (to.y - from.y) * factor,
            from.z + (to.z - from.z) * factor
        };
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.featuresStates().combatFeaturesStates.flyState;
    }

    HookContext& hookContext;
};
