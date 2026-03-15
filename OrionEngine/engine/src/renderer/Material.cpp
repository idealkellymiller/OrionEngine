#include "Material.hpp"
#include "Shader.hpp"
#include "Texture.hpp"


Material::Material() : m_Shader(nullptr), m_DiffuseTexture(nullptr), m_Color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

void Material::Bind() const
{
    if (!m_Shader)
        return;

    // Make this shader the active OpenGL shader
    // Bind the shader program so uniform updates affect this shader
    m_Shader->Bind();

    // upload base material color
    m_Shader->SetVec4("u_Material.Color", m_Color);

    // Upload scpecular settings used by the lighting shader
    m_Shader->SetVec3("u_Material.SpecularColor", m_SpecularColor);
    m_Shader->SetFloat("u_Material.Shininess", m_Shininess);

    // If a texture exists, bind it to slot 0 and tell the shader to use it
    if (m_DiffuseTexture) {
        // Bind texture to unit 0
        m_DiffuseTexture->Bind(0);

        // Tell the shader to sample from unit 0
        m_Shader->SetInt("u_DiffuseTexture", 0);
        // Set flag in shader to tell it this material has a texture
        m_Shader->SetInt("u_UseTexture", 1);    // 1 for true = it has a texture
        
    }
    else {
        if (m_Shader) {
            // If there is no texture, the shader should fall back to plain color
            m_Shader->SetInt("u_UseTexture", 0);
        }
    }
}

void Material::SetTransparent(bool transparent)
{
    if (transparent) {
        // Tarnsparent objects usually use alpha blending.
        m_RenderState.Blend = BlendMode::Transparent;

        // Keep depth testing on so transparent surfaces still test against depth.
        m_RenderState.DepthTest = true;

        // Usually diable writes for transparent surfaces so they do not
        // incorrectly block later transparent objects behind them.
        m_RenderState.DepthWrite = false;

        // Back-face culling is still usually fine unless material is double-sided.
        m_RenderState.Cull = CullMode::Back;
    }
    else {
        // Opaque objects do not need blending.
        m_RenderState.Blend = BlendMode::Opaque;
        m_RenderState.DepthTest = true;
        m_RenderState.DepthWrite = true;
        m_RenderState.Cull = CullMode::Back;
    }
}