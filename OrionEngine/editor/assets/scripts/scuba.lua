-- Script: New Script

function OnStart()
    -- Called once when play mode begins

    -- Rotation speed in radians per second.
    rotationSpeed = 2.0
end

function OnUpdate(dt)
    -- Called every frame with delta time

    -- Rotate around Y axis
    local rx, ry, rz = Transform.GetRotation()
    ry = ry + rotationSpeed * dt
    Transform.SetRotation(rx, ry, rz)
end
