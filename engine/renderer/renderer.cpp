#include "renderer.hpp"
#include "snapshots.hpp"

#include "platform/platform.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace engine {

Renderer::Renderer(const Platform& platform) {
    // передаем nullptr, для выбора api рендера самим SDL
    renderer = SDL_CreateRenderer(platform.getWindow(), nullptr);
    if (!renderer) {
        throw std::runtime_error(std::string("SDL_CreateRenderer: ") + SDL_GetError());
    }
    vsyncEnabled = SDL_SetRenderVSync(renderer, 1);
}

Renderer::~Renderer() {
    SDL_DestroyRenderer(renderer);
}

// Подготовка фона
void Renderer::clear() const {
    SDL_SetRenderDrawColor(renderer, 20, 40, 80, 255);
    SDL_RenderClear(renderer); // заполняет фон
}

// Отрисовка мира в бекбуфере
void Renderer::drawSnapshot(const RenderSnapshot& snapshot) const {
    int width = 0;
    int height = 0;
    // получаем размеры окна
    SDL_GetRenderOutputSize(renderer, &width, &height);

    // Начало координат в центре окна, Y мира направлена вверх.
    // Одна единица мира 200 пикселей
    constexpr float pixelsPerUnit = 200.0f;
    // центр окна
    const float originX = static_cast<float>(width) / 2.0f;
    const float originY = static_cast<float>(height) / 2.0f;

    // Рисуем прямоугольники
    SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
    for (const auto [x, y, halfWidth, halfHeight] : snapshot.boxes) {
        const SDL_FRect rect{
            .x = originX + (x - halfWidth) * pixelsPerUnit,
            .y = originY - (y + halfHeight) * pixelsPerUnit,
            .w = 2.0f * halfWidth * pixelsPerUnit,
            .h = 2.0f * halfHeight * pixelsPerUnit
        };
        SDL_RenderFillRect(renderer, &rect);
    }

    // Рисуем точки
    SDL_SetRenderDrawColor(renderer, 80, 220, 255, 255);
    for (const auto [x, y] : snapshot.points) {
        // Фактически эта точка, но для наглядности делаем ее больше.
        const SDL_FRect marker{
            .x = originX + x * pixelsPerUnit - 4.0f,
            .y = originY - y * pixelsPerUnit - 4.0f,
            .w = 4.0f, .h = 4.0f
        };
        SDL_RenderFillRect(renderer, &marker);
    }
}

// Выводим из бекбуфера в графику
void Renderer::present() const {
    SDL_RenderPresent(renderer);
    if (!vsyncEnabled) {
        // Даём процессору передышку, если драйвер не поддерживает VSync.
        SDL_Delay(1);
    }
}

}
