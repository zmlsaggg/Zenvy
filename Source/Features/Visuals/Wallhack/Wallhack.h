#pragma once

#include <array>
#include <cstddef>

#include <CS2/Classes/Vector.h>
#include <Features/Common/InWorldPanels.h>
#include <GameClient/Entities/PlayerPawn.h>
#include <GameClient/Panorama/PanelHandle.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <GameClient/WorldToScreen/WorldToClipSpaceConverter.h>
#include <Config/ConfigVariable.h>

#include "WallhackConfigVariables.h"
#include "WallhackState.h"

template <typename HookContext>
class Wallhack {
public:
    explicit Wallhack(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void drawForPlayer(auto&& playerPawn) const noexcept
    {
        const auto index = skeletonIndexOf(playerPawn);
        if (index >= WallhackState::kMaxPlayers)
            return;

        auto& skeleton = state().skeletons[index];
        skeleton.active = true;
        if (!enabled() || !shouldDraw(playerPawn)) {
            hideSkeleton(skeleton);
            return;
        }

        const auto absOrigin = playerPawn.absOrigin();
        if (!absOrigin.hasValue()) {
            hideSkeleton(skeleton);
            return;
        }

        if (!hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(absOrigin.value()).onScreen()) {
            hideSkeleton(skeleton);
            return;
        }

        if (showInfo())
            drawInfoPanel(playerPawn, absOrigin.value());

        if (showSkeleton())
            drawSkeleton(playerPawn, absOrigin.value(), skeleton);
        else
            hideSkeleton(skeleton);
    }

    void beginFrame() const noexcept
    {
        for (auto& skeleton : state().skeletons)
            skeleton.active = false;
    }

    void endFrame() const noexcept
    {
        for (auto& skeleton : state().skeletons) {
            if (!skeleton.active)
                hideSkeleton(skeleton);
        }
    }

    void onUnload() const noexcept
    {
        for (auto& skeleton : state().skeletons) {
            for (auto& handle : skeleton.jointHandles) {
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(handle);
                handle = {};
            }
            skeleton.active = false;
        }
    }

private:
    [[nodiscard]] bool enabled() const noexcept { return GET_CONFIG_VAR(wallhack_vars::Enabled); }
    [[nodiscard]] bool showEnemies() const noexcept { return GET_CONFIG_VAR(wallhack_vars::ShowEnemies); }
    [[nodiscard]] bool showTeammates() const noexcept { return GET_CONFIG_VAR(wallhack_vars::ShowTeammates); }
    [[nodiscard]] bool showSkeleton() const noexcept { return GET_CONFIG_VAR(wallhack_vars::ShowSkeleton); }
    [[nodiscard]] bool showInfo() const noexcept { return GET_CONFIG_VAR(wallhack_vars::ShowInfo); }

    [[nodiscard]] bool shouldDraw(auto&& playerPawn) const noexcept
    {
        if (!playerPawn.isAlive().value_or(false))
            return false;
        if (playerPawn.isControlledByLocalPlayer())
            return false;
        if (playerPawn.isEnemy().value_or(true))
            return showEnemies();
        return showTeammates();
    }

    void drawInfoPanel(auto&& playerPawn, const cs2::Vector& position) const noexcept
    {
        auto&& panel = hookContext.template make<InWorldPanels>().getNextPlayerInfoPanel();
        panel.drawPlayerInfo(playerPawn);
        panel.updatePosition(position);
    }

    void drawSkeleton(auto&& playerPawn, const cs2::Vector& origin, auto& skeleton) const noexcept
    {
        skeleton.active = true;

        static constexpr float kOffsets[WallhackState::kJoints]{
            64.0f, 56.0f, 48.0f, 38.0f, 28.0f, 18.0f, 10.0f, 0.0f, -20.0f, -44.0f
        };

        for (std::size_t joint = 0; joint < WallhackState::kJoints; ++joint) {
            const auto point = origin + cs2::Vector{0.0f, 0.0f, kOffsets[joint]};
            const auto clip = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(point);
            if (!clip.onScreen()) {
                hideJointPanel(skeleton, joint);
                continue;
            }

            auto&& panel = jointPanel(skeleton, joint);
            if (!panel)
                continue;

            const auto distance = distanceTo(origin);
            const auto size = distance > 0.0f ? (2600.0f / distance) : 4.0f;

            panel.setVisible(true);
            panel.setWidth(cs2::CUILength{size, cs2::CUILength::k_EUILengthLength});
            panel.setHeight(cs2::CUILength{size, cs2::CUILength::k_EUILengthLength});
            panel.setPosition(cs2::CUILength::percent(50.0f), cs2::CUILength::percent(50.0f));
            panel.setBackgroundColor(playerPawn.isEnemy().value_or(true)
                ? cs2::Color{255, 80, 80, 220}
                : cs2::Color{120, 200, 255, 220});
            positionPanel(panel, clip);
        }
    }

    void hideSkeleton(auto& skeleton) const noexcept
    {
        for (std::size_t joint = 0; joint < WallhackState::kJoints; ++joint)
            hideJointPanel(skeleton, joint);
        skeleton.active = false;
    }

    void hideJointPanel(auto& skeleton, std::size_t joint) const noexcept
    {
        auto& handle = skeleton.jointHandles[joint];
        if (!handle.isValid())
            return;

        auto&& panel = hookContext.template make<PanelHandle>(handle).get();
        if (panel)
            panel.setVisible(false);
    }

    void positionPanel(auto&& panel, const auto& clip) const noexcept
    {
        const auto ndc = clip.toNormalizedDeviceCoordinates();
        PanoramaTransformations{
            hookContext.panoramaTransformFactory().scale(1.0f),
            hookContext.panoramaTransformFactory().translate(ndc.getX(), ndc.getY())
        }.applyTo(panel);
    }

    [[nodiscard]] auto jointPanel(auto& skeleton, std::size_t joint) const noexcept
    {
        auto&& handle = hookContext.template make<PanelHandle>(skeleton.jointHandles[joint]);
        return handle.getOrInit([this] {
            auto&& panel = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
            panel.fitParent();
            return panel;
        });
    }

    [[nodiscard]] std::size_t skeletonIndexOf(auto&& playerPawn) const noexcept
    {
        const auto entityIndex = playerPawn.baseEntity().handle().index();
        if (!entityIndex.isValid())
            return WallhackState::kMaxPlayers;
        return static_cast<std::size_t>(entityIndex.value) % WallhackState::kMaxPlayers;
    }

    [[nodiscard]] float distanceTo(const cs2::Vector& position) const noexcept
    {
        auto&& local = hookContext.activeLocalPlayerPawn();
        if (!local)
            return 0.0f;
        return cs2::Vector::distance(local.absOrigin().valueOr(cs2::Vector{}), position);
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.featuresStates().visualFeaturesStates.wallhackState;
    }

    HookContext& hookContext;
};