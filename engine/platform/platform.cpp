#include "platform.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace engine {

    Platform::Platform(const char* title, const int width, const int height) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
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
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // обработка событий платформы: клавиатура, мышь, взаимодействие с окном
    bool Platform::pollEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
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
