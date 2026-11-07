#pragma once
#include <functional>
#include <vector>

#include "ecs/world.hpp"


namespace engine {

class Simulation {
    using System = std::function<void(ecs::World&, float)>;

    ecs::World world;
    std::vector<System> systems;
    static constexpr float fixedDt = 1.0f / 60.0f;

public:
    static constexpr float getFixedDt() noexcept { return fixedDt; }
    ecs::World& getWorld();
    void addSystem(const System &system);
    void step();
};

}
