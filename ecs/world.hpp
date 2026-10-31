#pragma once

#include <cstdint> // std::uint64_t
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <vector> // std::vector
#include <cstddef>   // std::size_t
#include <limits>    // std::numeric_limits
#include <stdexcept> // std::out_of_range
#include <utility> // std::move
#include <type_traits> // std::is_nothrow_move_assignable_v
#include <typeindex> // std::type_index
#include <memory>    // std::unique_ptr
#include <tuple> // std::tuple
#include <algorithm> // std::min

namespace engine::ecs {

using Entity = std::uint64_t;

class World {
    Entity nextEntity = 1;
    std::unordered_set<Entity> entities;

    // для доступа к разным типам ComponentStorage
    struct IComponentStorage {
        virtual ~IComponentStorage() = default;

        virtual bool remove(Entity entity) = 0;
    };

    template<typename T>
    class ComponentStorage : public IComponentStorage {
        friend class World;

        static constexpr std::size_t invalidIndex = std::numeric_limits<std::size_t>::max();
        // хранилище компонентов заданного типа
        std::vector<T> components;
        // хранилище владельцев компонентов
        std::vector<Entity> owners;
        // индексы компонентов по владельцу, потому что его компоненты могут находиться на разных позициях
        // т.к. компоненты могут удаляться вместе с овнерами, удаление занимает много времени.
        std::vector<std::size_t> indices;
    public:
        bool has(const Entity entity) const {
            return entity < indices.size() && indices[entity] != invalidIndex;
        }

        T& get(const Entity entity) {
            if (!has(entity)) throw std::out_of_range("Component not found");

            return components[indices[entity]];
        }

        const T& get(const Entity entity) const {
            if (!has(entity)) throw std::out_of_range("Component not found");

            return components[indices[entity]];
        }

        T& add(Entity entity, T component) {
            // проверяем повторное добавление – не более одного компонента каждого типа
            if (has(entity)) throw std::invalid_argument("Component already exists");

            // расширяем вектор индексов
            if (entity >= indices.size()) {
                if (entity >= indices.max_size()) throw std::length_error("Entity ID is too large");

                indices.resize(entity + 1, invalidIndex);
            }

            // запоминаем индекс для компонента
            const std::size_t index = components.size();
            owners.push_back(entity);

            // добавляем компонент в вектор
            try {
                components.push_back(std::move(component));
            } catch (...) {
                owners.pop_back();
                throw;
            }

            indices[entity] = index; // связываем владельца и компонент
            return components.back();
        }

        bool remove(Entity entity) override {
            // проверяем, что объекту типа T можно присвоить другой объект перемещением без исключений
            static_assert(std::is_nothrow_move_assignable_v<T>, "Component move assignment must be noexcept");

            if (!has(entity)) return false;

            const std::size_t component_index = indices[entity];
            const std::size_t lastIndex = components.size() - 1;

            // если текущий компонент не последний, то
            // присваиваем на его место последний, тем самым экономим время и память
            // ну и соотвественно не забываем владельца поменять и вектор связи владельца и компонента
            if (component_index != lastIndex) {
                components[component_index] = std::move(components[lastIndex]);
                owners[component_index] = owners[lastIndex];

                indices[owners[component_index]] = component_index;
            }

            components.pop_back();
            owners.pop_back();

            indices[entity] = invalidIndex;
            return true;
        }
    };

    std::unordered_map<
        std::type_index, // GOD SAVE C++17
        std::unique_ptr<IComponentStorage>
    > storages;

    template<typename T>
    ComponentStorage<T>& getStorage() {
        const std::type_index type(typeid(T));
        auto it = storages.find(type);

        if (it == storages.end()) {
            auto storage = std::make_unique<ComponentStorage<T>>();
            // добавляем, если его нету
            it = storages.emplace(type, std::move(storage)).first;
        }

        return *static_cast<ComponentStorage<T>*>(it->second.get());
    }

    template<typename T>
    ComponentStorage<T>* findStorage() {
        const auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) return nullptr;

        return static_cast<ComponentStorage<T>*>(it->second.get());
    }
public:
    Entity createEntity();

    bool isAlive(Entity entity) const;

    bool destroyEntity(Entity entity);

    template<typename T>
    T& addComponent(Entity entity, T component) {
        if (!isAlive(entity)) throw std::out_of_range("Entity is not alive");

        return getStorage<T>().add(entity, std::move(component));
    }

    template<typename T>
    bool hasComponent(Entity entity) const {
        if (!isAlive(entity)) return false;

        // есть ли вообще такое хранилище компонентов
        const auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) return false;

        const auto* storage = static_cast<const ComponentStorage<T>*>(it->second.get());
        return storage->has(entity);
    }

    template<typename T>
    T& getComponent(Entity entity) {
        if (!isAlive(entity)) throw std::out_of_range("Entity is not alive");

        const auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) throw std::out_of_range("Component storage not found");

        auto* storage = static_cast<ComponentStorage<T>*>(it->second.get());
        return storage->get(entity);
    }

    template<typename T>
    const T& getComponent(Entity entity) const {
        if (!isAlive(entity)) throw std::out_of_range("Entity is not alive");

        const auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) throw std::out_of_range("Component storage not found");

        const auto* storage = static_cast<ComponentStorage<T>*>(it->second.get());
        return storage->get(entity);
    }

    template<typename T>
    bool removeComponent(Entity entity) {
        if (!isAlive(entity)) return false;

        const auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) return false;

        return it->second->remove(entity);
    }

    template<typename... Components>
    auto query() {
        std::vector<std::tuple<Entity, Components&...>> result;
        // получаем указатели на хранилища выбранных компонентов
        const auto pointers = std::tuple{findStorage<Components>()...};

        std::apply([&](auto*... storage) {
            // проверяем, есть ли вообще хранилища для наших компонентов
            if ((!storage || ...)) {
                return;
            }

            // оптимизация заключается в том, что любая подходящая сущность должна присутсвовать во всех хранилищах,
            // включая самое маленькое которое легче всего обойти.
            // candidates - вектор владельцев
            const auto* candidates = std::min(
                {&storage->owners...},
                [](const auto* a, const auto* b) {
                    return a->size() < b->size();
                }
            );

            // заранее выделяем есто
            result.reserve(candidates->size());

            // перебираем для всех владельцев меньшего хранилища их нахождение во всех хранилищах
            // и добавляем в результат, если есть все вхождения у этого владельца
            for (Entity entity : *candidates) {
                if ((storage->has(entity) && ...)) {
                    result.emplace_back(entity, storage->get(entity)...);
                }
            }
        }, pointers); // передаем pointers в storage

        return result;
    }
};

} // namespace engine::ecs
