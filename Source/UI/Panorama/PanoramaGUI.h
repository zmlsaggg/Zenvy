#pragma once

#include <CS2/Panorama/CUIPanel.h>
#include <GameClient/Entities/PreviewPlayer.h>
#include <Features/Visuals/ModelGlow/Preview/PlayerModelGlowPreview.h>
#include <Features/Visuals/ModelGlow/Preview/PlayerModelGlowPreviewColorMode.h>
#include <Features/Visuals/ModelGlow/Preview/WeaponModelGlowPreview.h>
#include <GameClient/Entities/TeamNumber.h>
#include <GlobalContext/UnloadFlag.h>
#include <GameClient/Panorama/PanoramaLabel.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <GameClient/Panorama/Ui3dPanel.h>
#include <Utils/StringBuilder.h>

#include "PanoramaCommandDispatcher.h"
#include "CombatTab.h"
#include "HudTab.h"
#include "SoundTab.h"
#include "VisualsTab.h"
#include "Tabs/VisualsTab/ViewmodelModPreviewPanel.h"

template <typename HookContext>
class PlayerModelGlowPreviewPanel {
public:
    PlayerModelGlowPreviewPanel(HookContext& hookContext, cs2::CUIPanel* panel, TeamNumber teamNumber) noexcept
        : hookContext{hookContext}
        , panel{panel}
        , teamNumber{teamNumber}
    {
    }

    void update() const noexcept
    {
        if (!state().shouldUpdatePanel)
            return;

        StringBuilderStorage<100> storage;
        auto builder = storage.builder();
        builder.put(playerName(), ' ', teamName());

        if (state().colorMode == PlayerModelGlowPreviewColorMode::PlayerOrTeamColor) {
            if (const auto colorString = colorIndexToString()) {
                builder.put(" - ");
                builder.put(colorString);
            }
        } else if (state().colorMode == PlayerModelGlowPreviewColorMode::HealthBased) {
            builder.put(" - ", state().previewPlayerHealth, " HP");
        }

        labelPanel().setText(builder.cstring());
    }

private:
    [[nodiscard]] const char* playerName() const noexcept
    {
        switch (state().enemyTeam) {
        case EnemyTeam::Both: return "Enemy";
        case EnemyTeam::CT: return teamNumber == TeamNumber::CT ? "Enemy" : "Ally";
        case EnemyTeam::T: return teamNumber == TeamNumber::TT ? "Enemy" : "Ally";
        default: return "Player";
        }
    }

    [[nodiscard]] const char* teamName() const noexcept
    {
        return teamNumber == TeamNumber::TT ? "T" : "CT";
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.playerModelGlowPreviewState();
    }

    [[nodiscard]] decltype(auto) labelPanel() const noexcept
    {
        return hookContext.template make<PanoramaUiPanel>(panel).clientPanel().template as<PanoramaLabel>();
    }

    [[nodiscard]] const char* colorIndexToString() const noexcept
    {
        switch (state().previewPlayerColorIndex) {
        using enum cs2::PlayerColorIndex;
        case Blue: return "Blue";
        case Green: return "Green";
        case Yellow: return "Yellow";
        case Orange: return "Orange";
        case Purple: return "Purple";
        default: return nullptr;
        }
    }

    HookContext& hookContext;
    cs2::CUIPanel* panel;
    TeamNumber teamNumber;
};

template <typename HookContext>
class PanoramaGUI {
public:
    explicit PanoramaGUI(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    [[nodiscard]] bool init(auto&& mainMenu) noexcept
    {
        if (!mainMenu)
            return false;

        if (state().guiPanelHandle.isValid())
            return true;

        // Ensure the settings tab is loaded because the menu uses its CSS classes.
        uiEngine().runScript(mainMenu, "if (!$('#JsSettings')) MainMenu.NavigateToTab('JsSettings', 'settings/settings');");

        const auto settings = mainMenu.findChildInLayoutFile("JsSettings");
        if (!settings)
            return false;

        state().settingsPanelHandle = settings.getHandle();

        // The root panel is first created under JsSettings, then moved into
        // JsMainMenuContent. Check both locations before retrying creation.
        if (!settings.findChildInLayoutFile("FunBunMenuTab") && !mainMenu.findChildInLayoutFile("FunBunMenuTab"))
            uiEngine().runScript(settings, reinterpret_cast<const char*>(
#include "CreateGUI.js"
));

        // This script is safe to retry: it reparents an existing tab and creates
        // the navbar button only when it does not already exist.
        uiEngine().runScript(mainMenu, R"(
(function () {
  var context = $.GetContextPanel();
  var settings = context.FindChildTraverse('JsSettings');
  var content = context.FindChildTraverse('JsMainMenuContent');
  var menuTab = settings && settings.FindChildInLayoutFile('FunBunMenuTab');
  if (menuTab && content)
    menuTab.SetParent(content);

  var openMenuButton = context.FindChildTraverse('FunBunOpenMenuButton');
  if (!openMenuButton) {
    var settingsButton = context.FindChildTraverse('MainMenuNavBarSettings');
    var navBar = settingsButton && settingsButton.GetParent();
    if (!navBar)
      return;

    openMenuButton = $.CreatePanel('RadioButton', navBar, 'FunBunOpenMenuButton', {
      class: "mainmenu-top-navbar__radio-iconbtn",
      group: "NavBar",
      onactivate: "MainMenu.NavigateToTab('FunBunMenuTab', '');"
    });

    // Keep the navbar control self-contained: do not fetch branding assets
    // from GitHub (or any other remote host) at game runtime.
    $.CreatePanel('Label', openMenuButton, 'FunBunBrandMark', {
      class: "mainmenu-top-navbar__radio-btn__icon",
      text: "FB",
      style: "width: 28px; height: 28px; font-size: 15px; font-weight: bold; text-align: center; vertical-align: center; color: white;"
    });

    $.DispatchEvent('Activated', context.FindChildTraverse("MainMenuNavBarHome"), 'mouse');
  }
})();
)");

        const auto guiButtonPanel = mainMenu.findChildInLayoutFile("FunBunOpenMenuButton");
        const auto guiPanel = mainMenu.findChildInLayoutFile("FunBunMenuTab");
        if (!guiButtonPanel || !guiPanel)
            return false;

        const auto previewLabelTT = guiPanel.findChildInLayoutFile("ModelGlowPreviewPlayerTTLabel");
        const auto previewLabelCT = guiPanel.findChildInLayoutFile("ModelGlowPreviewPlayerCTLabel");
        const auto viewmodelPreview = guiPanel.findChildInLayoutFile("ViewmodelPreview");
        if (!previewLabelTT || !previewLabelCT || !viewmodelPreview)
            return false;

        // Publish handles only after every panel used by the render/update path exists.
        state().guiButtonHandle = guiButtonPanel.getHandle();
        state().modelGlowPreviewPlayerLabelHandleTT = previewLabelTT.getHandle();
        state().modelGlowPreviewPlayerLabelHandleCT = previewLabelCT.getHandle();
        state().viewmodelPreviewPanelHandle = viewmodelPreview.getHandle();
        state().guiPanelHandle = guiPanel.getHandle();

        hookContext.template make<CombatTab>().init(guiPanel);
        hookContext.template make<HudTab>().init(guiPanel);
        hookContext.template make<VisualsTab>().init(guiPanel);
        hookContext.template make<SoundTab>().init(guiPanel);

        updateFromConfig();
        return true;
    }

    template <typename ConfigVariable>
    void onHueSliderValueChanged(const char* panelId, float value) const
    {
        const auto newVariableValue = handleHueSlider(panelId, value, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, GET_CONFIG_VAR(ConfigVariable));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});
    }

    template <typename ConfigVariable>
    void onHueSliderTextEntrySubmit(const char* panelId, const char* value) const noexcept
    {
        const auto newVariableValue = handleHueTextEntry(panelId, value, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, GET_CONFIG_VAR(ConfigVariable));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});
    }

    [[nodiscard]] decltype(auto) modelGlowPreviewPanel(const char* panelId) const noexcept
    {
        auto&& guiPanel = uiEngine().getPanelFromHandle(state().guiPanelHandle);
        return guiPanel.findChildInLayoutFile(panelId).clientPanel().template as<Ui3dPanel>();
    }

    void run(UnloadFlag& unloadFlag) const noexcept
    {
        if (!state().guiPanelHandle.isValid())
            return;

        auto&& guiPanel = uiEngine().getPanelFromHandle(state().guiPanelHandle);
        if (!guiPanel)
            return;

        auto&& playerModelGlowPreview = hookContext.template make<PlayerModelGlowPreview>();
        if (!playerModelGlowPreview.isPreviewPlayerSetTT())
            playerModelGlowPreview.setPreviewPlayerTT(guiPanel.findChildInLayoutFile("ModelGlowPreviewPlayerTT").clientPanel().template as<Ui3dPanel>().portraitWorld().findPreviewPlayer());
        if (!playerModelGlowPreview.isPreviewPlayerSetCT())
            playerModelGlowPreview.setPreviewPlayerCT(guiPanel.findChildInLayoutFile("ModelGlowPreviewPlayerCT").clientPanel().template as<Ui3dPanel>().portraitWorld().findPreviewPlayer());

        const auto cmdSymbol = uiEngine().makeSymbol(0, "cmd");
        const auto cmd = guiPanel.getAttributeString(cmdSymbol, "");
        PanoramaCommandDispatcher{cmd, unloadFlag, hookContext}();
        guiPanel.setAttributeString(cmdSymbol, "");

        hookContext.template make<PlayerModelGlowPreview>().update();
        hookContext.template make<PlayerModelGlowPreview>().hookPreviewPlayersSceneObjectUpdaters();

        hookContext.template make<WeaponModelGlowPreview>().updateSceneObjectUpdaterHooks();

        auto&& viewmodelModPreviewPanel = uiEngine().getPanelFromHandle(state().viewmodelPreviewPanelHandle).clientPanel().template as<ViewmodelModPreviewPanel>();
        viewmodelModPreviewPanel.setupPreviewModel();
        viewmodelModPreviewPanel.setFov();

        hookContext.template make<PlayerModelGlowPreviewPanel>(uiEngine().getPanelFromHandle(state().modelGlowPreviewPlayerLabelHandleTT), TeamNumber::TT).update();
        hookContext.template make<PlayerModelGlowPreviewPanel>(uiEngine().getPanelFromHandle(state().modelGlowPreviewPlayerLabelHandleCT), TeamNumber::CT).update();
    }

    void updateFromConfig() noexcept
    {
        const auto mainMenuPointer = hookContext.patternSearchResults().template get<MainMenuPanelPointer>();
        auto&& mainMenu = hookContext.template make<ClientPanel>(mainMenuPointer ? *mainMenuPointer : nullptr).uiPanel();
        hookContext.template make<CombatTab>().updateFromConfig(mainMenu);
        hookContext.template make<HudTab>().updateFromConfig(mainMenu);
        hookContext.template make<VisualsTab>().updateFromConfig(mainMenu);
        hookContext.template make<SoundTab>().updateFromConfig(mainMenu);
    }

    void onUnload() const noexcept
    {
        uiEngine().deletePanelByHandle(state().guiButtonHandle);
        uiEngine().deletePanelByHandle(state().guiPanelHandle);

        if (auto&& settingsPanel = uiEngine().getPanelFromHandle(state().settingsPanelHandle))
            uiEngine().runScript(settingsPanel, "delete $.FunBun");
    }

private:
    [[nodiscard]] decltype(auto) getHueSlider(const char* sliderId) const noexcept
    {
        auto&& guiPanel = uiEngine().getPanelFromHandle(state().guiPanelHandle);
        return hookContext.template make<HueSlider>(guiPanel.findChildInLayoutFile(sliderId));
    }

    [[nodiscard]] color::HueInteger handleHueTextEntry(const char* sliderId, const char* value, color::HueInteger min, color::HueInteger max, color::HueInteger current) const noexcept
    {
        auto&& hueSlider = getHueSlider(sliderId);
        color::HueInteger::UnderlyingType hueIntegral;
        if (!StringParser{value}.parseInt(hueIntegral) || hueIntegral < min || hueIntegral > max) {
            hueSlider.updateTextEntry(current);
            return current;
        }

        if (hueIntegral == current)
            return current;

        const color::HueInteger hue{hueIntegral};
        hueSlider.updateSlider(hue);
        hueSlider.updateColorPreview(hue);
        return hue;
    }

    [[nodiscard]] color::HueInteger handleHueSlider(const char* sliderId, float value, color::HueInteger min, color::HueInteger max, color::HueInteger current) const noexcept
    {
        const auto hueIntegral = static_cast<color::HueInteger::UnderlyingType>(value);
        if (hueIntegral < min || hueIntegral > max || hueIntegral == current)
            return current;

        const auto hue = color::HueInteger{hueIntegral};
        auto&& hueSlider = getHueSlider(sliderId);
        hueSlider.updateTextEntry(hue);
        hueSlider.updateColorPreview(hue);
        return hue;
    }

    [[nodiscard]] decltype(auto) uiEngine() const noexcept
    {
        return hookContext.template make<PanoramaUiEngine>();
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.panoramaGuiState();
    }

    HookContext& hookContext;
};
