#include "ecs/world.hpp"
#include "ecs/components.hpp"
#include "systems/movement.hpp"

#include <iostream>
#include <thread>

#include "simulation/simulation.hpp"
using namespace engine;

int main() {
    Simulation simulation;
    simulation.addSystem(systems::Movement::update);

    ecs::World& world = simulation.getWorld();

    ecs::Entity robot = world.createEntity();
    world.addComponent(robot, ecs::Position{.x = 0, .y = 0});
    world.addComponent(robot, ecs::Velocity{.x = 2.f, .y = 0.f});
    world.addComponent(robot, ecs::PointCollider{});


    ecs::Entity wall = world.createEntity(); // (1,25; 0,25) to (0,75; -0,25)
    world.addComponent(wall, ecs::Position{.x = 1.f, .y = 0.f});
    world.addComponent(wall, ecs::BoxCollider{.halfWidth = 0.25f, .halfHeight = 0.25f});

    for (std::size_t i = 0; i < 60; i++) {
        simulation.step();

        const auto pos = world.getComponent<ecs::Position>(robot);
        std::cout << pos.x << ' ' << pos.y << std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(1)); чет не работает
    }

    return !world.isAlive(robot);
}