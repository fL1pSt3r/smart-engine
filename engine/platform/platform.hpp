#pragma once

struct SDL_Window;

namespace engine {

    enum class Keys {
        W,
        A,
        S,
        D
    };

class Platform {
    SDL_Window* window = nullptr;
    bool running = true;

public:
    Platform(const char* title, int width, int height);
    ~Platform();

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    bool pollEvents();
    [[nodiscard]] SDL_Window* getWindow() const noexcept;

    [[nodiscard]] bool isKeyDown(Keys key) const;
};

}
