#pragma once

struct SDL_Window;
struct SDL_Gamepad;

namespace engine {

    enum class Keys {
        W,
        A,
        S,
        D
    };

enum class GamepadAxis {
    LeftX,
    LeftY,
    RightY
};

class Platform {
    SDL_Window* window = nullptr;
    SDL_Gamepad* gamepad = nullptr;
    bool running = true;

public:
    Platform(const char* title, int width, int height);
    ~Platform();

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    bool pollEvents();
    [[nodiscard]] SDL_Window* getWindow() const noexcept;

    [[nodiscard]] bool isKeyDown(Keys key) const;
    [[nodiscard]] bool hasGamepad() const;
    [[nodiscard]] float getGamepadAxis(GamepadAxis axis) const;
};

}
