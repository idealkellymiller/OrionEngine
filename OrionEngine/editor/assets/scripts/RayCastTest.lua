function OnUpdate(dt)
    -- "Am I grounded?" = short downward ray
    local origin_x, origin_y, origin_z = Transform.GetPosition()
    local hit = Physics.Raycast(origin_x, origin_y, origin_z, 0, -1, 0, 1.1)
    if hit then
        Log.Info("Standing on entity " .. hit.entity)
    end

    -- "Shoot from camera forward" = left click to fire
    if Input.IsMouseButtonPressed(0) then
        local fx, fy, fz = Transform.GetForward()
        local px, py, pz = Transform.GetPosition()
        local h = Physics.Raycast(px, py, pz, fx, fy, fz, 100)
        if h then
            Log.Info("Hit " .. h.entity .. " at " .. h.distance .. "m")
            -- Push the target away
            Physics.AddImpulseToEntity(h.entity, fx * 50, fy * 50, fz * 50)
        end
    end
end