#version 460 core

layout(location = 0) out uint outEntityID;

uniform uint u_EntityID;

void main()
{
    outEntityID = u_EntityID;
}