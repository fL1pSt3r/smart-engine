#pragma once
#include <optional>

#include "ecs/components.hpp"

namespace engine::geometry {

    bool containsPoint(const ecs::Position& point,
        const ecs::Position& boxPosition,
        const ecs::BoxCollider& box);

    // рассчитываем, когда будет столкновение
    std::optional<float> segmentBoxHit(const ecs::Position& start,
        const ecs::Position& end,
        const ecs::Position& boxPosition,
        const ecs::BoxCollider& box);

    // Доля пути до блокирующего контакта. Уход с границы наружу и вдоль неё разрешён.
    std::optional<float> blockingSegmentBoxHit(const ecs::Position& start,
        const ecs::Position& end,
        const ecs::Position& boxPosition,
        const ecs::BoxCollider& box);
}
