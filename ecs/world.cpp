#include "world.hpp"

#include <ranges> // std::ranges
#include <stdexcept> // std::overflow_error

namespace engine::ecs {

    Entity World::createEntity() {
        const Entity entity = nextEntity;
        if (entity == 0) throw std::overflow_error("Entity IDs are exhausted");

        entities.insert(entity);
        // TODO: костыль костылёвич, надо будет захерачить использование свободных айди
        ++nextEntity;
        return entity;
    }

    bool World::isAlive(Entity entity) const {
        return entities.contains(entity);
    }

    bool World::destroyEntity(Entity entity) {
        if (!isAlive(entity)) return false;

        for (auto &storage: storages | std::views::values) {
            storage->remove(entity);
        }
        entities.erase(entity);
        return true;
    }

} // namespace engine::ecs
