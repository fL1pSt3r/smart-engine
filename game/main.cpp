#include "ecs/components.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer.hpp"
#include "simulation/simulation.hpp"
#include "systems/movement.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <cmath>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "renderer/snapshots.hpp"

using namespace engine;

std::vector<ecs::Entity> createScene(Simulation& simulation) {
    auto& world = simulation.getWorld();

    std::vector<ecs::Entity> robots;
    for (int row = 0; row < 5; ++row) {
        for (int column = 0; column < 2; ++column) {
            const auto robot = world.createEntity();
            world.addComponent(robot, ecs::Position{
                .x = -1.4f + column * 0.6f,
                .y = -0.8f + row * 0.4f
            });
            world.addComponent(robot, ecs::Velocity{});
            world.addComponent(robot, ecs::PointCollider{});
            robots.push_back(robot);
        }
    }

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

struct SharedState {
    std::mutex mutex;
    ControlState control;
    RenderSnapshot snapshot;
    std::exception_ptr error;
};

ControlState readControl(const Platform& platform) {
    float x = platform.isKeyDown(Keys::D) - platform.isKeyDown(Keys::A);

    float y = platform.isKeyDown(Keys::W) - platform.isKeyDown(Keys::S);

    const float length = std::hypot(x, y);
    if (length > 1.0f) {
        x /= length;
        y /= length;
    }

    return ControlState{.x = x, .y = y};
}

void updateRobotControl(ecs::World& world, ecs::Entity robot, const ControlState& control) {
    constexpr float speed = 0.5f;
    auto& velocity = world.getComponent<ecs::Velocity>(robot);
    velocity.x = control.x * speed;
    velocity.y = control.y * speed;
}

// получаем объекты по ссылке -> не создаем их копий
void runSimulation(Simulation& simulation, ControlState& control, SharedState& shared, std::atomic<bool>& running) {
    try {
        auto previous = std::chrono::steady_clock::now();
        // накопитель прошедшего, не обработанного времени
        double accumulator = 0.0;
        constexpr double stepDuration = Simulation::getFixedDt();

        while (running.load()) {
            const auto now = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(now - previous).count();
            previous = now;
            // после долгой паузы не догоняем всё пропущенное время, а четверть секунды
            accumulator += std::min(elapsed, 0.25);

            bool updated = false;
            while (running.load() && accumulator >= stepDuration) {
                {
                    std::lock_guard lock(shared.mutex);
                    control = shared.control;
                }

                simulation.step();
                accumulator -= stepDuration;
                updated = true;
            }

            if (updated) {
                auto snapshot = buildRenderSnapshot(simulation.getWorld());
                std::lock_guard lock(shared.mutex);
                shared.snapshot = std::move(snapshot);
            }

            // между шагами поток спит, а не занимает процессор пустым циклом
            if (running.load()) {
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(stepDuration - accumulator)
                );
            }
        }
    } catch (...) {
        // главный поток прочитает ошибку только после джойна
        shared.error = std::current_exception();
        running.store(false);
    }
}

int main() {
    try {
        Platform platform("Robot Game", 800, 600);
        Renderer renderer(platform);

        ControlState control;
        Simulation simulation;
        const auto robots = createScene(simulation);

        simulation.addSystem(
            [robots, &control](ecs::World& world, float) {
                for (const auto robot : robots) {
                    updateRobotControl(world, robot, control);
                }
            }
        );

        simulation.addSystem(systems::Movement::update);

        SharedState shared;
        shared.snapshot = buildRenderSnapshot(simulation.getWorld());
        std::atomic<bool> running{true};

        std::thread simulationThread([&] {
            runSimulation(simulation, control, shared, running);
        });

        try {
            while (running.load() && platform.pollEvents()) {
                const auto input = readControl(platform);
                RenderSnapshot snapshot;
                {
                    std::lock_guard lock(shared.mutex);
                    shared.control = input;
                    snapshot = shared.snapshot;
                }

                renderer.clear();
                renderer.drawSnapshot(snapshot);
                renderer.present();
            }
        } catch (...) {
            running.store(false);
            simulationThread.join();
            throw;
        }

        running.store(false);
        simulationThread.join();
        if (shared.error) {
            std::rethrow_exception(shared.error);
        }

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Unknown error\n";
        return 1;
    }

    return 0;
}
