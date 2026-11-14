#include "snapshots.hpp"

#include "ecs/components.hpp"
#include "ecs/world.hpp"

namespace engine {

RenderSnapshot buildRenderSnapshot(ecs::World& world) {

    RenderSnapshot snapshot;

    for (auto [entity, pos, box] : world.query<ecs::Position, ecs::BoxCollider>()) {
        snapshot.boxes.push_back(
            RenderBox{
                .x = pos.x,
                .y = pos.y,
                .halfWidth = box.halfWidth,
                .halfHeight = box.halfHeight
            }
        );
    }

    for (auto [entity, pos, point] : world.query<ecs::Position, ecs::PointCollider>()) {
        snapshot.points.push_back(
            RenderPoint{
                .x = pos.x,
                .y = pos.y
            }
        );
    }

    return snapshot;
}

}
