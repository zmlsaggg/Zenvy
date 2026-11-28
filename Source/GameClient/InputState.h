#pragma once

#include <cstddef>
#include <cstdint>

struct InputState {
    static constexpr std::size_t kKeyCount = 256;

    void setKey(std::uint32_t key, bool down) noexcept
    {
        if (key < kKeyCount)
            keys[key] = down;
    }

    [[nodiscard]] bool keyDown(std::uint32_t key) const noexcept
    {
        return key < kKeyCount && keys[key];
    }

    void clear() noexcept
    {
        for (auto& key : keys)
            key = false;
    }

private:
    bool keys[kKeyCount]{};
};