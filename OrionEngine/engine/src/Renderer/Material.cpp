#include "Renderer/Material.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

#include <Renderer/Renderer.h>


namespace Orion {

    Material::Material() : m_Shader(nullptr), m_DiffuseTexture(nullptr), m_Color(1.0f, 1.0f, 1.0f, 1.0f)
    {
    }

    void Material::Bind() const
    {
        if (!m_Shader)
            return;

        // Make this shader the active OpenGL shader
        m_Shader->Bind();

        // upload base material color
        m_Shader->SetVec4("u_Material.Color", m_Color);

        // Upload specular settings used by the lighting shader
        m_Shader->SetVec3("u_Material.SpecularColor", m_SpecularColor);
        m_Shader->SetFloat("u_Material.Shininess", m_Shininess);

        // If a texture exists, bind it to slot 0 and tell the shader to use it
        if (m_DiffuseTexture) {
            m_DiffuseTexture->Bind(0);
            m_Shader->SetInt("u_DiffuseTexture", 0);
            m_Shader->SetInt("u_UseTexture", 1);
        }
        else {
            if (m_Shader) {
                m_Shader->SetInt("u_UseTexture", 0);
            }
        }
    }

    void Material::SetTransparent(bool transparent)
    {
        if (transparent) {
            m_RenderState.Blend = BlendMode::Transparent;
            m_RenderState.DepthTest = true;
            m_RenderState.DepthWrite = false;
            m_RenderState.Cull = CullMode::Back;
        }
        else {
            m_RenderState.Blend = BlendMode::Opaque;
            m_RenderState.DepthTest = true;
            m_RenderState.DepthWrite = true;
            m_RenderState.Cull = CullMode::Back;
        }
    }
}
