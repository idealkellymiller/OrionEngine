// Instead of manually binding shader and texture everywhere,
// package them into one object

// Material is responsible for 
// - Binding the shader	
// - binding the texture
// - sending uniforms to the shader
#pragma once
#include <glm/glm.hpp>

#include "MaterialRenderState.hpp"


class Shader;
class Texture;


class Material {
public:
	Material();

	// Binds shader, textures, and uniforms needed for rendering
	void Bind() const;

	// Assign the shader used by this material
	void SetShader(Shader* shader) { m_Shader = shader; }
	// Assign the main diffuse texture
	void SetDiffuseTexture(Texture* texture) { m_DiffuseTexture = texture; }
	// Set a tint color for the material
	void SetColor(const glm::vec4& color) { m_Color = color; }

	// specular highlight color
	void SetSpecularColor(const glm::vec3& specularColor) { m_SpecularColor = specularColor; }

	// Higher shiniess = tighter/smaller highlights
	void SetShininess(float shininess) { m_Shininess = shininess; }

	Shader* GetShader() const { return m_Shader; }
	Texture* GetDiffuseTexture() const { return m_DiffuseTexture; }

	const glm::vec4& GetColor() const { return m_Color; }
	const glm::vec3& GetSpecularColor() const { return m_SpecularColor; }
	float GetShininess() const { return m_Shininess; }

	// Access render state so renderer can decide how to draw this material.
	MaterialRenderState& GetRenderState() { return m_RenderState; }
	const MaterialRenderState& GetRenderState() const { return m_RenderState; }

	// convenience helper for common transparancy setup
	void SetTransparent(bool transparent);

	// Helper used by renderer to choose pass
	bool IsTransparent() const
	{
		return m_RenderState.Blend == BlendMode::Transparent;
	}

private:
	Shader* m_Shader;			// Shader program used by this material
	Texture* m_DiffuseTexture;	// Main color texture (albedo/diffuse)
	
	// Diffuse/base surface color
	glm::vec4 m_Color;

	// Specular response
	glm::vec3 m_SpecularColor = glm::vec3(1.0f);
	float m_Shininess = 32.0f;

	MaterialRenderState m_RenderState;
};