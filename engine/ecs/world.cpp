#include "world.hpp"

#include <ranges> // std::ranges
#include <stdexcept> // std::overflow_error

namespace engine::ecs {

    Entity World::createEntity() {
        if (!freeIndices.empty()) {
            const auto index = freeIndices.back();
            freeIndices.pop_back();

            auto&[generation, alive] = entitySlots[index];
            alive = true;

            return Entity{.index = index, .generation = generation};
        }

        if (entitySlots.size() > std::numeric_limits<std::uint32_t>::max())
            throw std::overflow_error("Entity indices are exhausted");

        const auto index = static_cast<std::uint32_t>(entitySlots.size());

        entitySlots.push_back(EntitySlot{1, true});
        return Entity{.index = index, .generation = 1};
    }

    bool World::isAlive(const Entity entity) const {
        return entity.index < entitySlots.size()
            && entitySlots[entity.index].alive
            && entitySlots[entity.index].generation == entity.generation;
    }

    bool World::destroyEntity(Entity entity) {
        if (!isAlive(entity)) return false;

        for (auto &storage: storages | std::views::values) {
            storage->remove(entity);
        }

        auto&[generation, alive] = entitySlots[entity.index];

        // Исчерпавшее поколения место больше не переиспользуем.
        if (generation < std::numeric_limits<std::uint32_t>::max()) {
            freeIndices.push_back(entity.index);
            ++generation;
        }

        alive = false;
        return true;
    }
} // namespace engine::ecs
