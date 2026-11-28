#pragma once

#include <bit>
#include <cstdint>

namespace cs2
{

struct Vector {
    float x, y, z;

    [[nodiscard]] constexpr float squareDistTo(const Vector& other) const noexcept
    {
        return (*this - other).squareLength();
    }

    [[nodiscard]] constexpr float squareLength() const noexcept
    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] friend constexpr Vector operator-(const Vector& a, const Vector& b) noexcept
    {
        return Vector{a.x - b.x, a.y - b.y, a.z - b.z};
    }

    [[nodiscard]] friend constexpr Vector operator+(const Vector& a, const Vector& b) noexcept
    {
        return Vector{a.x + b.x, a.y + b.y, a.z + b.z};
    }

    [[nodiscard]] friend constexpr Vector operator*(const Vector& a, float s) noexcept
    {
        return Vector{a.x * s, a.y * s, a.z * s};
    }

    [[nodiscard]] friend constexpr Vector operator-(const Vector& a, float s) noexcept
    {
        return Vector{a.x - s, a.y - s, a.z - s};
    }

    [[nodiscard]] Vector& operator+=(const Vector& other) noexcept
    {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    [[nodiscard]] bool operator==(const Vector&) const = default;

    [[nodiscard]] static float distance(const Vector& a, const Vector& b) noexcept
    {
        const auto squaredDistance = a.squareDistTo(b);
        if (squaredDistance <= 0.0f)
            return 0.0f;

        auto bits = std::bit_cast<std::uint32_t>(squaredDistance);
        bits = 0x5F3759DFu - (bits >> 1);
        auto inverseSqrt = std::bit_cast<float>(bits);
        const auto halfDistance = squaredDistance * 0.5f;
        inverseSqrt *= 1.5f - halfDistance * inverseSqrt * inverseSqrt;
        inverseSqrt *= 1.5f - halfDistance * inverseSqrt * inverseSqrt;
        return squaredDistance * inverseSqrt;
    }
};

}
