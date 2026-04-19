-- Script: New Script

function OnStart()
    -- Called once when play mode begins
end

function OnUpdate(dt)
    -- Called every frame with delta time

    if Input.IsKeyPressed("Q") then
        Application.Quit()
    end
end
