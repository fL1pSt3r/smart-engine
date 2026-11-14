#pragma once
#include <vector>

namespace engine {

namespace ecs { class World; }

struct RenderPoint {
    float x;
    float y;
};

struct RenderBox {
    float x;
    float y;
    float halfWidth;
    float halfHeight;
};

struct RenderSnapshot {
    std::vector<RenderPoint> points;
    std::vector<RenderBox> boxes;
};

RenderSnapshot buildRenderSnapshot(ecs::World& world);

}
