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

    struct Rotation {
        float angle = 0.0f;
    };

    struct DifferentialDrive {
        float leftSpeed = 0.0f;
        float rightSpeed = 0.0f;
        float wheelDistance = 0.2f;
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
