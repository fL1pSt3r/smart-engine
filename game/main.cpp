#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <vector>

#include "ecs/components.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer.hpp"
#include "simulation/simulation.hpp"
#include "systems/movement.hpp"
#include "renderer/snapshots.hpp"
#include "runtime/runtime.hpp"

using namespace engine;

std::vector<ecs::Entity> createScene(Simulation& simulation) {
    auto& world = simulation.getWorld();

    std::vector<ecs::Entity> robots;
    const auto robot = world.createEntity();
    world.addComponent(robot, ecs::Position{
        .x = -1.4f + 1 * 0.6f,
        .y = -0.8f + 1 * 0.4f
    });
    world.addComponent(robot, ecs::Velocity{});
    world.addComponent(robot, ecs::PointCollider{});
    robots.push_back(robot);

    auto addWall = [&](float x, float y, float halfWidth, float halfHeight) {
        const auto wall = world.createEntity();
        world.addComponent(wall, ecs::Position{.x = x, .y = y});
        world.addComponent(wall, ecs::BoxCollider{
            .halfWidth = halfWidth, .halfHeight = halfHeight
        });
    };

    // Границы сцены.
    addWall(-1.8f, 0.0f, 0.05f, 1.25f);
    addWall(1.8f, 0.0f, 0.05f, 1.25f);
    addWall(0.0f, -1.2f, 1.8f, 0.05f);
    addWall(0.0f, 1.2f, 1.8f, 0.05f);

    // По одному препятствию напротив каждого ряда роботов.
    for (int row = 0; row < 5; ++row) {
        addWall(0.65f, -0.8f + row * 0.4f, 0.12f, 0.12f);
    }

    return robots;
}

struct ControlState {
    float x = 0.0f;
    float y = 0.0f;
};

ControlState readControl(const Platform& platform) {
    float x = platform.isKeyDown(Keys::D) - platform.isKeyDown(Keys::A);
    float y = platform.isKeyDown(Keys::W) - platform.isKeyDown(Keys::S);

    // Клавиатура работает и при подключённом геймпаде.
    if (platform.hasGamepad() && x == 0.0f && y == 0.0f) {
        x = platform.getGamepadAxis(GamepadAxis::LeftX);
        y = -platform.getGamepadAxis(GamepadAxis::LeftY);
    }

    const float scale = std::max(1.0f, std::hypot(x, y));
    return ControlState{.x = x / scale, .y = y / scale};
}

void updateRobotControl(ecs::World& world, ecs::Entity robot, const ControlState& control) {
    constexpr float maxSpeed = 0.5f;
    auto& velocity = world.getComponent<ecs::Velocity>(robot);
    velocity.x = control.x * maxSpeed;
    velocity.y = control.y * maxSpeed;
}

int main() {
    try {
        Platform platform("Robot Game", 800, 600);
        Renderer renderer(platform);

        Simulation simulation;
        const auto robots = createScene(simulation);

        simulation.addSystem(systems::Movement::update);
        Runtime runtime(simulation);
        runtime.start();

        while (runtime.isRunning() && platform.pollEvents()) {
            const auto input = readControl(platform);

            runtime.setBeforeStep([robots, input](ecs::World& world) {
                for (const auto robot : robots) {
                    updateRobotControl(world, robot, input);
                }
            });

            const auto snapshot = runtime.getSnapshot();

            renderer.clear();
            renderer.drawSnapshot(snapshot);
            renderer.present();
        }

        runtime.stop();
        runtime.rethrowIfFailed();

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Unknown error\n";
        return 1;
    }

    return 0;
}
