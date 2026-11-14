#pragma once

struct SDL_Renderer;

namespace engine {

class Platform;
struct RenderSnapshot;

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
    void drawSnapshot(const RenderSnapshot& snapshot) const;
    void present() const;
};

}
