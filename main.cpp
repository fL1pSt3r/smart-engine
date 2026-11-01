#include "ecs/world.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

using engine::ecs::Entity;
using engine::ecs::World;

struct Position { int x, y; };
struct Velocity { int dx, dy; };
struct Tag { int value; };
struct Missing {};

struct Owned {
    std::unique_ptr<int> value;
};

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template<typename Exception, typename Action>
void expectThrow(Action&& action, const char* message) {
    bool caught = false;

    try {
        action();
    } catch (const Exception&) {
        caught = true;
    }

    check(caught, message);
}

void testEntities() {
    World world;

    check(!world.isAlive(Entity{}), "Default Entity must be invalid");
    check(!world.isAlive(Entity{999, 1}), "Unknown Entity must be invalid");
    check(!world.destroyEntity(Entity{}), "Cannot destroy invalid Entity");
    check(world.query<Position>().empty(), "Empty World query");

    const auto old = world.createEntity();
    const auto neighbor = world.createEntity();

    check(old != neighbor, "Entities must have different IDs");
    check(world.isAlive(old), "Created Entity must be alive");

    world.addComponent(old, Position{10, 20});
    world.addComponent(old, Velocity{1, 2});
    world.addComponent(neighbor, Position{30, 40});

    check(world.destroyEntity(old), "Destroy Entity");
    check(!world.destroyEntity(old), "Repeated destruction");
    check(!world.isAlive(old), "Destroyed Entity must be dead");

    // Удаление old не должно испортить перенесённый компонент neighbor.
    check(world.getComponent<Position>(neighbor).x == 30,
          "Destroy corrupted another Entity");

    const auto fresh = world.createEntity();

    check(fresh.index == old.index, "Freed index was not reused");
    check(fresh.generation != old.generation, "Generation was not changed");
    check(world.isAlive(fresh), "Reused Entity must be alive");
    check(!world.isAlive(old), "Old generation became alive");

    check(!world.hasComponent<Position>(fresh), "Position was inherited");
    check(!world.hasComponent<Velocity>(fresh), "Velocity was inherited");

    world.addComponent(fresh, Position{50, 60});

    check(!world.destroyEntity(old), "Old ID destroyed new Entity");
    check(!world.removeComponent<Position>(old),
          "Old ID removed new component");

    expectThrow<std::out_of_range>(
        [&] { world.getComponent<Position>(old); },
        "Getting component through old ID must throw"
    );

    expectThrow<std::out_of_range>(
        [&] { world.addComponent(old, Position{0, 0}); },
        "Adding component through old ID must throw"
    );

    check(world.getComponent<Position>(fresh).x == 50,
          "Old ID operations corrupted new Entity");
}

void testComponents() {
    World world;

    const auto a = world.createEntity();
    const auto b = world.createEntity();
    const auto c = world.createEntity();

    auto& position = world.addComponent(a, Position{1, 2});
    position.x = 10;

    check(world.getComponent<Position>(a).x == 10,
          "addComponent must return a reference");

    world.addComponent(b, Position{20, 0});
    world.addComponent(c, Position{30, 0});
    world.addComponent(a, Velocity{5, 0});

    expectThrow<std::invalid_argument>(
        [&] { world.addComponent(a, Position{999, 999}); },
        "Duplicate component must throw"
    );

    check(world.getComponent<Position>(a).x == 10,
          "Duplicate addition changed the component");

    const World& readOnly = world;

    static_assert(std::is_same_v<
        decltype(readOnly.getComponent<Position>(a)),
        const Position&
    >);

    check(readOnly.getComponent<Position>(a).x == 10,
          "Const getComponent");

    check(!world.hasComponent<Missing>(a), "Missing component");
    check(!world.removeComponent<Missing>(a), "Remove missing storage");

    expectThrow<std::out_of_range>(
        [&] { world.getComponent<Missing>(a); },
        "Getting from missing storage must throw"
    );

    // Удаляем средний элемент: последний должен занять его место.
    check(world.removeComponent<Position>(b), "Remove middle component");
    check(!world.removeComponent<Position>(b), "Repeated component removal");
    check(world.isAlive(b), "Component removal destroyed Entity");

    check(world.getComponent<Position>(a).x == 10,
          "First component was corrupted");
    check(world.getComponent<Position>(c).x == 30,
          "Moved component index was not updated");

    expectThrow<std::out_of_range>(
        [&] { world.getComponent<Position>(b); },
        "Getting removed component must throw"
    );

    // После переноса c находится последним.
    check(world.removeComponent<Position>(c), "Remove last component");
    check(world.removeComponent<Position>(a), "Remove final component");
    check(world.query<Position>().empty(), "Empty existing storage query");

    check(world.hasComponent<Velocity>(a),
          "Removing Position also removed Velocity");

    world.addComponent(b, Position{70, 80});
    check(world.getComponent<Position>(b).x == 70,
          "Adding component after removal");
}

void testQueries() {
    World world;

    const auto a = world.createEntity();
    const auto b = world.createEntity();
    const auto c = world.createEntity();
    const auto d = world.createEntity();

    world.addComponent(a, Position{1, 0});
    world.addComponent(b, Position{2, 0});
    world.addComponent(c, Position{3, 0});

    // Порядок Velocity специально отличается от Position.
    world.addComponent(d, Velocity{40, 0});
    world.addComponent(b, Velocity{20, 0});
    world.addComponent(a, Velocity{10, 0});

    world.addComponent(a, Tag{7});
    world.addComponent(c, Tag{9});

    check(world.query<Missing>().empty(), "Missing storage query");
    check(world.query<Position, Missing>().empty(),
          "Missing second storage query");
    check(world.query<Missing, Position>().empty(),
          "Missing first storage query");

    std::size_t count = 0;
    bool foundD = false;

    for (auto [entity, velocity] : world.query<Velocity>()) {
        check(&velocity == &world.getComponent<Velocity>(entity),
              "Single component query must return references");
        foundD = foundD || entity == d;
        ++count;
    }

    check(count == 3 && foundD,
          "Velocity query must include Entity without Position");

    bool foundA = false;
    bool foundB = false;
    count = 0;

    // Сохраняем диапазон: он должен работать после возврата из query().
    auto selection = world.query<Position, Velocity>();

    for (auto [entity, pos, vel] : selection) {
        check((entity == a && !foundA) || (entity == b && !foundB),
              "Query returned unexpected or duplicate Entity");

        if (entity == a) foundA = true;
        if (entity == b) foundB = true;

        check(&pos == &world.getComponent<Position>(entity),
              "Position must be a reference");
        check(&vel == &world.getComponent<Velocity>(entity),
              "Velocity must be a reference");

        pos.x += vel.dx;
        ++count;
    }

    check(count == 2 && foundA && foundB, "Two-component intersection");
    check(world.getComponent<Position>(a).x == 11, "Query mutation of a");
    check(world.getComponent<Position>(b).x == 22, "Query mutation of b");
    check(world.getComponent<Position>(c).x == 3, "Unmatched Entity changed");

    count = 0;

    for (auto [entity, pos, vel, tag] :
         world.query<Position, Velocity, Tag>()) {
        check(entity == a && pos.x == 11 && vel.dx == 10 && tag.value == 7,
              "Three-component query");
        ++count;
    }

    check(count == 1, "Three-component intersection size");

    count = 0;

    for (auto [entity, tag, vel, pos] :
         world.query<Tag, Velocity, Position>()) {
        check(entity == a && tag.value == 7 && vel.dx == 10 && pos.x == 11,
              "Query component order");
        ++count;
    }

    check(count == 1, "Reordered query intersection size");
}

void testGrowth() {
    World world;
    constexpr std::size_t count = 2000;

    std::vector<Entity> original;
    std::vector<Entity> replacements;

    for (std::size_t i = 0; i < count; ++i) {
        const auto entity = world.createEntity();
        original.push_back(entity);
        world.addComponent(entity, Position{static_cast<int>(i), 0});
    }

    // Многократные удаления из середины плотного массива.
    for (std::size_t i = 0; i < count; i += 2) {
        check(world.destroyEntity(original[i]), "Mass destruction");
    }

    for (std::size_t i = 1; i < count; i += 2) {
        check(world.getComponent<Position>(original[i]).x ==
                  static_cast<int>(i),
              "Mass removal corrupted surviving component");
    }

    for (std::size_t i = 0; i < count / 2; ++i) {
        const auto entity = world.createEntity();
        replacements.push_back(entity);

        check(!world.hasComponent<Position>(entity),
              "Reused slot inherited a component");

        world.addComponent(
            entity, Position{10000 + static_cast<int>(i), 0}
        );
    }

    for (std::size_t i = 0; i < count; i += 2) {
        check(!world.isAlive(original[i]), "Old generation became valid");
    }

    for (std::size_t i = 0; i < replacements.size(); ++i) {
        check(world.getComponent<Position>(replacements[i]).x ==
                  10000 + static_cast<int>(i),
              "Replacement component was corrupted");
    }

    std::size_t found = 0;

    for (auto [entity, pos] : world.query<Position>()) {
        check(world.isAlive(entity), "Query returned dead Entity");
        check(&pos == &world.getComponent<Position>(entity),
              "Query reference after growth");
        ++found;
    }

    check(found == count, "Query count after growth and reuse");

    for (Entity entity : original) {
        if (world.isAlive(entity)) {
            check(world.destroyEntity(entity), "Cleanup original Entity");
        }
    }

    for (Entity entity : replacements) {
        check(world.destroyEntity(entity), "Cleanup replacement Entity");
    }

    check(world.query<Position>().empty(), "Query after full cleanup");
}

void testMoveOnlyComponent() {
    World world;

    const auto a = world.createEntity();
    const auto b = world.createEntity();

    world.addComponent(a, Owned{std::make_unique<int>(10)});
    world.addComponent(b, Owned{std::make_unique<int>(20)});

    check(world.removeComponent<Owned>(a), "Remove move-only component");
    check(*world.getComponent<Owned>(b).value == 20,
          "Moving last component lost its resource");

    std::size_t count = 0;

    for (auto [entity, owned] : world.query<Owned>()) {
        check(entity == b, "Move-only query Entity");
        *owned.value = 30;
        ++count;
    }

    check(count == 1, "Move-only query count");
    check(*world.getComponent<Owned>(b).value == 30,
          "Move-only query reference");
}

int main() {
    try {
        testEntities();
        std::cout << "[OK] Entities and generations\n";

        testComponents();
        std::cout << "[OK] Component operations\n";

        testQueries();
        std::cout << "[OK] Queries and references\n";

        testGrowth();
        std::cout << "[OK] Growth and ID reuse\n";

        testMoveOnlyComponent();
        std::cout << "[OK] Move-only components\n";

        std::cout << "All World tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }

    return 0;
}