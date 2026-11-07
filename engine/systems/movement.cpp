#include "movement.hpp"

#include "ecs/components.hpp"
#include "geometry/collision.hpp"

namespace engine::systems {
    void Movement::update(ecs::World &world, const float dt) {
        auto walls = world.query<ecs::Position, ecs::BoxCollider>();

        for (auto [entity, pos, vel] : world.query<ecs::Position, ecs::Velocity>()) {
            const float dx = vel.x * dt;
            const float dy = vel.y * dt;
            if (dx == 0.0f && dy == 0.0f) {
                continue;
            }

            const ecs::Position end{pos.x + dx, pos.y + dy};
            float allowedPart = 1.0f;
            bool collided = false;

            // обработка столкновений точек с прямоугольниками
            if (world.hasComponent<ecs::PointCollider>(entity)) {
                for (auto [wallEntity, wallPos, box] : walls) {
                    if (wallEntity == entity) {
                        continue;
                    }

                    const auto hit = geometry::blockingSegmentBoxHit(pos, end, wallPos, box);
                    if (hit && *hit <= allowedPart) {
                        allowedPart = *hit;
                        collided = true;
                    }
                }
            }

            pos.x += dx * allowedPart;
            pos.y += dy * allowedPart;

            // Останавливаемся при столкновении
            if (collided) {
                vel.x = 0.0f;
                vel.y = 0.0f;
            }
        }
    }
}
