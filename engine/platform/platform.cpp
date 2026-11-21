#include "platform.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace engine {

Platform::Platform(const char* title, const int width, const int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }
    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow: " + error);
    }
}

Platform::~Platform() {
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// обработка событий платформы: клавиатура, мышь, взаимодействие с окном
bool Platform::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_GAMEPAD_ADDED && !gamepad) {
            gamepad = SDL_OpenGamepad(event.gdevice.which);
            if (gamepad) {
                SDL_Log("Gamepad connected: %s", SDL_GetGamepadName(gamepad));
            } else {
                SDL_Log("Cannot open gamepad: %s", SDL_GetError());
            }
        }
        if (event.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad) {
            if (event.gdevice.which == SDL_GetGamepadID(gamepad)) {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
                SDL_Log("Gamepad disconnected");
            }
        }
        if (event.type == SDL_EVENT_QUIT || // выход из приложения
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && // закрытие окна
             event.window.windowID == SDL_GetWindowID(window))) {
            running = false; // приложение должно завершить цикл
        }
    }
    return running;
}

SDL_Window* Platform::getWindow() const noexcept {
    return window;
}

bool Platform::hasGamepad() const {
    return gamepad && SDL_GamepadConnected(gamepad);
}

float Platform::getGamepadAxis(GamepadAxis axis) const {
    if (!hasGamepad() || SDL_GetKeyboardFocus() != window) return 0.0f;

    SDL_GamepadAxis sdlAxis;
    switch (axis) {
        case GamepadAxis::LeftX: sdlAxis = SDL_GAMEPAD_AXIS_LEFTX; break;
        case GamepadAxis::LeftY: sdlAxis = SDL_GAMEPAD_AXIS_LEFTY; break;
        case GamepadAxis::RightY: sdlAxis = SDL_GAMEPAD_AXIS_RIGHTY; break;
        default: return 0.0f;
    }

    const auto raw = static_cast<float>(SDL_GetGamepadAxis(gamepad, sdlAxis));
    const float value = raw / (raw < 0.0f ? 32768.0f : 32767.0f);

    // Убираем дрейф около центра и плавно масштабируем оставшийся диапазон.
    constexpr float deadZone = 0.15f;
    if (std::abs(value) <= deadZone) return 0.0f;

    const float magnitude = std::clamp((std::abs(value) - deadZone) / (1.0f - deadZone), 0.0f, 1.0f);
    return std::copysign(magnitude, value);
}

bool Platform::isKeyDown(Keys key) const {
    if (SDL_GetKeyboardFocus() != window) return false;

    SDL_Scancode scancode;
    switch (key) {
        case Keys::W: scancode = SDL_SCANCODE_W; break;
        case Keys::A: scancode = SDL_SCANCODE_A; break;
        case Keys::S: scancode = SDL_SCANCODE_S; break;
        case Keys::D: scancode = SDL_SCANCODE_D; break;
        default: return false;
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    return keys[scancode];

}
}
