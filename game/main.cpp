#include "ecs/components.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer.hpp"
#include "simulation/simulation.hpp"
#include "systems/movement.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <cmath>

using namespace engine;

ecs::Entity createScene(Simulation& simulation) {
    auto& world = simulation.getWorld();

    const auto robot = world.createEntity();
    world.addComponent(robot, ecs::Position{.x = 0, .y = 0});
    world.addComponent(robot, ecs::Velocity{.x = 0.0f, .y = 0.0f});
    world.addComponent(robot, ecs::PointCollider{});

    const auto wall = world.createEntity();
    world.addComponent(wall, ecs::Position{.x = 1.0f, .y = 0.0f});
    world.addComponent(wall, ecs::BoxCollider{.halfWidth = 0.25f, .halfHeight = 0.25f});

    return robot;
}

void updateRobotControl(ecs::World& world, ecs::Entity robot, const Platform& platform) {
    float x = platform.isKeyDown(Keys::D) -
              platform.isKeyDown(Keys::A);

    float y = platform.isKeyDown(Keys::W) -
              platform.isKeyDown(Keys::S);

    const float length = std::hypot(x, y);
    if (length > 1.0f) {
        x /= length;
        y /= length;
    }

    constexpr float speed = 0.5f;
    auto& velocity = world.getComponent<ecs::Velocity>(robot);
    velocity.x = x * speed;
    velocity.y = y * speed;
}

int main() {
    try {
        Platform platform("Robot Game", 800, 600);
        Renderer renderer(platform);

        Simulation simulation;
        const auto robot = createScene(simulation);

        simulation.addSystem(
            [robot, &platform](ecs::World& world, float) {
                updateRobotControl(world, robot, platform);
            }
        );

        simulation.addSystem(systems::Movement::update);

        auto previous = std::chrono::steady_clock::now();
        double accumulator = 0.0;
        constexpr double stepDuration = Simulation::getFixedDt();

        while (platform.pollEvents()) {
            const auto now = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(now - previous).count();
            previous = now;

            // после долгой паузы не догоняем всё пропущенное время, а четверть секунды.
            accumulator += std::min(elapsed, 0.25);
            // время кадра != времени симуляции, регулирем это тут
            while (accumulator >= stepDuration) {
                simulation.step();
                accumulator -= stepDuration;
            }

            // цикл рендера одного кадра
            renderer.clear();
            renderer.drawDebugWorld(simulation.getWorld());
            renderer.present();
        }

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
