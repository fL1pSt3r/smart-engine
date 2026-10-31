#include <iostream> // std::cout
#include "ecs/world.hpp"


int main() {

    engine::ecs::World world;

    struct TestComponent {
        int x;
    };

    engine::ecs::Entity entity = world.createEntity();
    world.addComponent(entity, TestComponent{10});

    for (auto [entity, test] : world.query<TestComponent>()) {
        std::cout << entity << ' ' << test.x;
    }
}