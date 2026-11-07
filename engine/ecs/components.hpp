#ifndef SMART_ENGINE_COMPONENTS_HPP
#define SMART_ENGINE_COMPONENTS_HPP

namespace engine::ecs {
    struct Position {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Velocity {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct PointCollider {

    };

    // половины размеров прямоугольника, чтобы было легче считать его границы (центр + половины)
    struct BoxCollider {
        float halfWidth;
        float halfHeight;
    };

};


#endif //SMART_ENGINE_COMPONENTS_HPP
