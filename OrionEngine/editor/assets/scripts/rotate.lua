-- rotate.lua
-- Spins this entity around the Y axis and bobs it up and down.
-- Attach to any entity with a TransformComponent.

-- Called once when play mode begins.
function OnStart()
    -- Store the starting Y position so we can bob relative to it.
    local x, y, z = Transform.GetPosition()
    startY = y

    -- Rotation speed in radians per second.
    rotationSpeed = 2.0

    -- Bob amplitude and speed.
    bobAmplitude = 0.5
    bobSpeed = 3.0
end

-- Called every frame with delta time (seconds since last frame).
function OnUpdate(dt)
    -- Rotate around Y axis
    local rx, ry, rz = Transform.GetRotation()
    ry = ry + rotationSpeed * dt
    Transform.SetRotation(rx, ry, rz)

    -- Bob up and down using a sine wave
    local elapsed = Time.elapsed()
    local x, y, z = Transform.GetPosition()
    y = startY + math.sin(elapsed * bobSpeed) * bobAmplitude
    Transform.SetPosition(x, y, z)

    -- Example: move forward when W is held
    if Input.IsKeyDown("W") then
        local px, py, pz = Transform.GetPosition()
        pz = pz - 5.0 * dt
        Transform.SetPosition(px, py, pz)
    end
end
