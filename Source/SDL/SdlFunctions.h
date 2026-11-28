#pragma once

#include <cstdint>

namespace sdl3
{

struct SDL_Window;
using SDL_ShowSimpleMessageBox = int(std::uint32_t flags, const char* title, const char* message, SDL_Window* window);
using SDL_PeepEvents = int(void* events, int numevents, int action, unsigned minType, unsigned maxType);
using SDL_GetKeyboardState = const bool*(int* numkeys);

inline constexpr int SDL_SCANCODE_A = 4;
inline constexpr int SDL_SCANCODE_D = 7;
inline constexpr int SDL_SCANCODE_S = 22;
inline constexpr int SDL_SCANCODE_W = 26;
inline constexpr int SDL_SCANCODE_SPACE = 44;
inline constexpr int SDL_SCANCODE_LCTRL = 224;
inline constexpr int SDL_SCANCODE_LSHIFT = 225;
inline constexpr int SDL_SCANCODE_LALT = 226;
inline constexpr int SDL_SCANCODE_RCTRL = 228;
inline constexpr int SDL_SCANCODE_RSHIFT = 229;
inline constexpr int SDL_SCANCODE_RALT = 230;

}
