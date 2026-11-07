#include "collision.hpp"

#include <algorithm>

namespace engine::geometry {
    namespace {
        struct BoundingBox {
            float left;
            float right;
            float bottom;
            float top;

            [[nodiscard]] bool contains(const ecs::Position& point) const {
                return point.x >= left && point.x <= right &&
                       point.y >= bottom && point.y <= top;
            }
        };

        BoundingBox getBounds(const ecs::Position& position, const ecs::BoxCollider& box) {
            return {
                .left = position.x - box.halfWidth,
                .right = position.x + box.halfWidth,
                .bottom = position.y - box.halfHeight,
                .top = position.y + box.halfHeight
            };
        }

        std::optional<float> intersectSegment(const ecs::Position& start,
                                             const ecs::Position& end,
                                             const BoundingBox& bounds) {
            float tEnter = 0.0f;
            float tExit = 1.0f;

            // каждая ось сужает допустимый интервал движения.
            auto checkAxis = [&](float origin, float delta, float min, float max) {
                if (delta == 0.0f) {
                    return origin >= min && origin <= max;
                }

                float t1 = (min - origin) / delta;
                float t2 = (max - origin) / delta;
                if (t1 > t2) {
                    std::swap(t1, t2);
                }

                tEnter = std::max(tEnter, t1);
                tExit = std::min(tExit, t2);
                return tEnter <= tExit;
            };

            if (!checkAxis(start.x, end.x - start.x, bounds.left, bounds.right) ||
                !checkAxis(start.y, end.y - start.y, bounds.bottom, bounds.top)) {
                return std::nullopt;
            }

            return tEnter;
        }
    }

    bool containsPoint(const ecs::Position& point,
                       const ecs::Position& boxPosition,
                       const ecs::BoxCollider& box) {
        return getBounds(boxPosition, box).contains(point);
    }

    std::optional<float> segmentBoxHit(const ecs::Position& start,
                                      const ecs::Position& end,
                                      const ecs::Position& boxPosition,
                                      const ecs::BoxCollider& box) {
        return intersectSegment(start, end, getBounds(boxPosition, box));
    }

    std::optional<float> blockingSegmentBoxHit(const ecs::Position& start,
                                              const ecs::Position& end,
                                              const ecs::Position& boxPosition,
                                              const ecs::BoxCollider& box) {
        const auto bounds = getBounds(boxPosition, box);
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;

        // касание в начале не блокирует уход наружу или движение вдоль границы.
        if (bounds.contains(start) &&
            ((start.x == bounds.left && dx <= 0.0f) ||
             (start.x == bounds.right && dx >= 0.0f) ||
             (start.y == bounds.bottom && dy <= 0.0f) ||
             (start.y == bounds.top && dy >= 0.0f))) {
            return std::nullopt;
        }

        return intersectSegment(start, end, bounds);
    }
}
