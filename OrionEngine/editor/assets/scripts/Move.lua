function OnUpdate(dt)
    local force = 50

    if Input.IsKeyDown("W") then Physics.AddForce(0, 0, -force) end
    if Input.IsKeyDown("S") then Physics.AddForce(0, 0,  force) end
    if Input.IsKeyDown("A") then Physics.AddForce(-force, 0, 0) end
    if Input.IsKeyDown("D") then Physics.AddForce( force, 0, 0) end
    if Input.IsKeyDown("Space") then Physics.AddImpulse(0, 400, 0) end
end

function OnCollision(other, isTrigger)
    Audio.PlayOneShot("audio/fah.mp3")
end