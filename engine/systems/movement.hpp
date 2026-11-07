#pragma once
#include "ecs/world.hpp"

namespace engine::systems {

class Movement {
public:
    static void update(ecs::World& world, float dt);
};

}