#pragma once

class RenderStats {
public:
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t visibleObjects = 0;
    uint32_t culledObjects = 0;
    float cpuFrameTime = 0.0f;
    float gpuFrameTime = 0.0f;

    void Reset() {
        drawCalls = 0;
        triangleCount = 0;
        visibleObjects = 0;
        culledObjects = 0;
        cpuFrameTime = 0.0f;
        gpuFrameTime = 0.0f;
    }
};