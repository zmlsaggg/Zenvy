# Fly runtime wiring audit

Date: 2026-11-28

## Scope

This is a source-level audit of the current `master` snapshot. It is not an in-game test and does not claim that a DLL was injected or exercised.

Files inspected:

- `Source/Features/Combat/Fly/Fly.h`
- `Source/EntryPoints/EntryPoints.h`
- `Source/EntryPoints/GuiEntryPoints.h`
- `Source/Features/FeaturesStates.h`

## Findings

1. `Fly::run()` exists, but the inspected main render entry point does not invoke it. `ViewRenderHook_onRenderStart()` runs the existing rendering/entity and HUD features, then the Panorama GUI/config update path; there is no Fly call in that sequence.
2. `FeaturesStates` has no Fly-specific state member. This alone does not prove a feature cannot work, but it means there is no obvious per-feature runtime state in the central state aggregate.
3. `Fly::input()` obtains input through `InWorldPanels`. That is a UI-panel subsystem dependency rather than an explicitly named game-input service. The source inspected does not establish that it returns reliable gameplay key state.
4. The UI/config controls therefore must not be treated as proof of an active runtime feature. A visible toggle can update config while no runtime consumer reads it.
5. The current source uses direct pawn velocity writes. This audit intentionally does not change or extend movement manipulation, game-memory access, or multiplayer movement behavior.

## Safe next diagnostics

- Trace the feature construction/registration graph and confirm whether a runtime owner ever constructs `Fly`.
- Add non-invasive lifecycle diagnostics in a local test harness: construction, config value changes, and feature callback invocation counts. Do not log player data or memory contents.
- Verify that disabling/unloading the module leaves no callbacks or UI panels behind.
- Keep the Fly controls marked unavailable until a supported, non-multiplayer test implementation and its invocation path are independently validated.

## Limitations

The GitHub code-search connector did not return usable repository-wide symbol results during this audit. The call-site finding above is limited to the inspected entry-point and state files; it is not a formal proof that no other call site exists anywhere in the repository. No memory dumps were collected, and no live game was run.
