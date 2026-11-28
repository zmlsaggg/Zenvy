#pragma once

#include <MemoryPatterns/PatternTypes/EntityPatternTypes.h>
#include <MemorySearch/CodePattern.h>

struct EntityPatterns {
    [[nodiscard]] static consteval auto addClientPatterns(auto clientPatterns) noexcept
    {
        return clientPatterns
            .template addPattern<OffsetToGameSceneNode, CodePattern{"2C E0 49 8B 85 ? ? ? ?"}.add(5).read()>()
            .template addPattern<OffsetToHealth, CodePattern{"EC ? 8B ? ? ? ? ? 85 ? 7F ? 48"}.add(4).read()>()
            .template addPattern<OffsetToLifeState, CodePattern{"0F B6 97 ? ? ? ? 39 F2"}.add(3).read()>()
            .template addPattern<OffsetToTeamNumber, CodePattern{"? ? ? ? 02 48 8D 05 ? ? ? ? 74 ? 48"}.read()>()
            .template addPattern<OffsetToVData, CodePattern{"? ? ? ? 5A 59 48 85 C0 74 ? 4C"}.read()>()
            .template addPattern<OffsetToRenderComponent, CodePattern{"49 8B BC 24 ? ? ? ? 48 85 FF 74 ? 8B 47"}.add(4).read()>()
            .template addPattern<OffsetToOwnerEntity, CodePattern{"8B 8B ? ? ? ? 83 F9 FF 75"}.add(2).read()>()
            .template addPattern<GetAbsOriginFunction, CodePattern{"4C 8B ? E8 ? ? ? ? F3 0F 10 05"}.add(4).abs()>()
            .template addPattern<GetAbsVelocityFunction, CodePattern{"E8 ? ? ? ? F3 0F 10 ? ? ? ? ? F3 0F 11"}.add(1).abs()>() // TODO: снять байты с linux-бинаря
            .template addPattern<SetAbsVelocityFunction, CodePattern{"48 89 5C 24 ? 57 48 83 EC 20 48 8B FA 48 8B D9 E8"}.read()>(); // TODO: снять байты с linux-бинаря
    }
};
