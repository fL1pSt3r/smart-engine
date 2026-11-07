#include "simulation.hpp"

namespace engine {
    void Simulation::step() {
        for (auto& system: systems) {
            system(world, fixedDt);
        }
    }

    void Simulation::addSystem(const System &system) {
        systems.push_back(system);
    }

    ecs::World &Simulation::getWorld() {
        return world;
    }
} // engine