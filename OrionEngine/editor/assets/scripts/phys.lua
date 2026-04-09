function OnStart()
    -- Jump on spawn
    Physics.AddImpulse(0, 10, 0)
end

function OnUpdate(dt)
    -- Continuous push forward
    if Input.IsKeyDown("W") then
        Physics.AddForce(0, 0, -10)
    end

    -- Read current velocity
    local vx, vy, vz = Physics.GetVelocity()
end

function OnCollision(otherID, isTrigger)
    -- Called when this entity collides with another
    -- otherEntityID: the entity we collided with
    -- isTrigger: true if either collider is marked as a trigger
    print("Hit entity " .. otherID)
end
