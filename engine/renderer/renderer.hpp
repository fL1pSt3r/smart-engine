#pragma once

struct SDL_Renderer;

namespace engine {

class Platform;
namespace ecs { class World; }

class Renderer {
    SDL_Renderer* renderer = nullptr;
    bool vsyncEnabled = false;

public:
    // Platform должна существовать дольше Renderer.
    explicit Renderer(const Platform& platform);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void clear() const;
    void drawDebugWorld(ecs::World& world) const;
    void present() const;
};

}
