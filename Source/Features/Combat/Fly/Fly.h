#pragma once

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
        if (!playerPawn || !playerPawn.isAlive().value_or(false))
            return;

        const auto absOrigin = playerPawn.absOrigin();
        if (!absOrigin.hasValue())
            return;

        const auto currentVelocity = playerPawn.velocity().value_or(cs2::Vector{});
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

    [[nodiscard]] static float fastSin(float angle) noexcept
    {
        constexpr float kPi = 3.14159265358979f;
        constexpr float kTwoPi = 6.28318530717959f;
        while (angle > kPi)
            angle -= kTwoPi;
        while (angle < -kPi)
            angle += kTwoPi;

        const auto sign = angle < 0.0f ? -1.0f : 1.0f;
        const auto x = angle * sign;
        const auto parabola = (4.0f / kPi) * x - (4.0f / (kPi * kPi)) * x * x;
        return sign * (parabola + 0.225f * (parabola * parabola - parabola));
    }

    [[nodiscard]] static float fastCos(float angle) noexcept
    {
        return fastSin(angle + 1.5707963267949f);
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

    [[nodiscard]] std::uint32_t descendKey() const noexcept
    {
        switch (GET_CONFIG_VAR(fly_vars::FlyKey)) {
        case 1: return sdl3::SDL_SCANCODE_LCTRL;
        case 2: return sdl3::SDL_SCANCODE_LALT;
        default: return sdl3::SDL_SCANCODE_LSHIFT;
        }
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

        const auto sinPitch = fastSin(pitch);
        const auto cosPitch = fastCos(pitch);
        const auto sinYaw = fastSin(yaw);
        const auto cosYaw = fastCos(yaw);
        const cs2::Vector forward{cosPitch * cosYaw, cosPitch * sinYaw, -sinPitch};
        const cs2::Vector right{sinYaw, -cosYaw, 0.0f};

        const auto& input = hookContext.inputState();
        if (input.keyDown(sdl3::SDL_SCANCODE_W)) target = target + forward * speed();
        if (input.keyDown(sdl3::SDL_SCANCODE_S)) target = target + forward * (-speed());
        if (input.keyDown(sdl3::SDL_SCANCODE_D)) target = target + right * speed();
        if (input.keyDown(sdl3::SDL_SCANCODE_A)) target = target + right * (-speed());
        if (input.keyDown(sdl3::SDL_SCANCODE_SPACE)) target.z += verticalSpeed();
        if (input.keyDown(descendKey())) target.z -= verticalSpeed();

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
