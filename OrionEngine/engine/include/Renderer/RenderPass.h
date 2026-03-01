#pragma once
#include <string>

#include "Renderer/IRenderBackend.h"
#include "Renderer/FrameContext.h"


// Base class for all render passes. Each pass will implement the Execute method to perform its specific rendering tasks. 
// The Renderer will manage a list of these passes and execute them in order each frame.
class RenderPass{
public:
    std::string name;
    bool enabled = true;

    virtual ~RenderPass() = default;
    virtual void Execute(IRenderBackend& backend, const FrameContext& contex) = 0; // Pure virtual function to be implemented by derived classes
    bool IsEnabled() const { return enabled; }
    const std::string& Name() const { return name; }
};