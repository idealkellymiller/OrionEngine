#pragma once


class CameraData {
public:
    // Mat4 viewMatrix;
    // Mat4 projectionMatrix;
    // Vec3 position;
    float nearPlane;
    float farPlane;
    float fov;
    float aspectRatio;
    bool isPersective;

    void RecalculateMatrices();
    // Frustum BuildFrustum() const;
};