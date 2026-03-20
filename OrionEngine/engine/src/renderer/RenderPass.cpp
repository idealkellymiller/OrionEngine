#include "RenderPass.hpp"

#include "Camera.hpp"
#include "Shader.hpp"
#include "DirectionalLight.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
// #include "MaterialRenderState.hpp"

#include <glad/glad.h>


void RenderPass::SetupPass(const RenderPassDesc& pass)
{
	switch (pass.Type) {
	case RenderPassType::Opaque: {
		// Opaque pass usually writes depth and does not blend
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		break;
	}

	case RenderPassType::Transparent: {
		// Transparent pass usually keeps depth testing on,
		// but most materials will disable depth writes.
		// We do not force blending on here because material state
		// still controls the exact blend behavior
		break;
	}
	}
}

void RenderPass::TeardownPass(const RenderPassDesc& pass)
{
	switch (pass.Type) {
	case RenderPassType::Opaque: {
		// Nothing special to restore yet.
		break;
	}

	case RenderPassType::Transparent: {
		// Restore safe defaults after transparent rendering.
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		break;
	}
	}
}


void RenderPass::UploadFrameUniforms(Shader& shader, const Camera& camera)
{
	// Upload camera matrices used by every object this frame.
	shader.SetMat4("u_View", camera.GetViewMatrix());
	shader.SetMat4("u_Projection", camera.GetProjectionMatrix());

	// Camera position is useful for specular lighting and other view-dependent effects.
	shader.SetVec3("u_CameraPos", camera.GetPosition());
}

void RenderPass::UploadLightingUniforms(
	Shader& shader,
	const DirectionalLight& directionalLight,
	bool hasDirectionalLight,
	unsigned int shadowDepthTexture,
	const glm::mat4& lightSpaceMatrix
)
{
	if (hasDirectionalLight) {
		// Normalize direction so lighting math is stable and predictable
		glm::vec3 lightDir = glm::normalize(directionalLight.Direction);

		shader.SetInt("u_HasDirectionalLight", 1);
		shader.SetVec3("u_DirectionalLight.direction", lightDir);
		shader.SetVec3("u_DirectionalLight.color", directionalLight.Color);
		shader.SetFloat("u_DirectionalLight.intensity", directionalLight.Intensity);

		// Upload the matrix used to transform world positions into light space.
		shader.SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

		// Bind the shadow map to texture unit 1.
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

		shader.SetInt("u_ShadowMap", 1);
	}
	else {
		// Shader can fall back to ambient-only behavior
		shader.SetInt("u_HasDirectionalLight", 0);
		// printf("There is no light in this scene.");
	}
}

void RenderPass::UploadObjectUniforms(Shader& shader, const glm::mat4& modelMatrix)
{
	// Model matrix is unique per rendered object.
	shader.SetMat4("u_Model", modelMatrix);
}

void RenderPass::SetupShaderForPass(
	Shader& shader,
	const Camera& camera,
	const DirectionalLight& directionalLight,
	bool hasDirectionalLight,
	unsigned int shadowDepthTexture,
	const glm::mat4& lightSpaceMatrix)
{
	UploadFrameUniforms(shader, camera);
	UploadLightingUniforms(
		shader,
		directionalLight,
		hasDirectionalLight,
		shadowDepthTexture,
		lightSpaceMatrix
	);
}

void RenderPass::ApplyMaterialRenderState(const Material& material)
{
	const MaterialRenderState& state = material.GetRenderState();


	// Depth testing controls whether fragments are compared against the depth buffer.
	if (state.DepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	// Depth write controls whether the fragment updates the depth buffer.
	glDepthMask(state.DepthWrite ? GL_TRUE : GL_FALSE);

	// Configure face culling
	switch (state.Cull) {
	case CullMode::None:
		glDisable(GL_CULL_FACE);
		break;

	case CullMode::Back:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);	// Cull back-facing triangles
		break;

	case CullMode::Front:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);	// Cull front-facing trianlges
		break;
	}

	// Configure blending.
	if (state.Blend == BlendMode::Transparent) {
		glEnable(GL_BLEND);

		// Standard alpha blending:
		// finalColor = srcColor * srcAlpha + dstColor * (1-srcAlpha)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		glDisable(GL_BLEND);
	}
}

void RenderPass::IssueDraw(const Mesh& mesh)
{
	if (mesh.HasIndices()) {
		// Draw indexed triangles using the bound element/index buffer.

		mesh.GetVertexArray().Bind();

		// Indexed draw: read indices fro the element array buffer currently associated with the VAO.
		//
		// Parameters:
		// - GL_TRIANGLES: topology
		// - indexCount: how many indices to consume
		// - GL_UNSIGNED_INT: type of each index in the index buffer
		// - nullptr: start at offset 0 in the bound index buffer
		glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);

		mesh.GetVertexArray().Unbind();
	}
	else {
		// Draw non-indexed triangles directly from the buffer.

		// Bind the VAO, which contains the vertex layout configuration.
		mesh.GetVertexArray().Bind();

		// non-indexed draw: read vertices sequentially from the bound vertex buffer.
		glDrawArrays(GL_TRIANGLES, 0, mesh.GetVertexCount());

		// Optional cleanup
		mesh.GetVertexArray().Unbind();
	}
}

void RenderPass::ExecutePass(
	const RenderPassDesc& pass,
	std::vector<DrawCommand>& queue,
	const Camera& camera,
	const DirectionalLight& directionalLight,
	bool hasDirectionalLight,
	unsigned int shadowDepthTexture,
	const glm::mat4& lightSpaceMatrix)
{
	SetupPass(pass);

	Shader* currentShader = nullptr;

	for (const DrawCommand& cmd : queue)
	{
		if (!cmd.MeshPtr || !cmd.MaterialPtr)
			continue;

		Shader* shader = cmd.MaterialPtr->GetShader();
		if (!shader)
			continue;

		// Only upload shared pass/frame uniforms when the shader changes.
		if (shader != currentShader)
		{
			shader->Bind();

			SetupShaderForPass(
				*shader,
				camera,
				directionalLight,
				hasDirectionalLight,
				shadowDepthTexture,
				lightSpaceMatrix);

			currentShader = shader;
		}

		// Apply material-specific fixed-function state before drawing.
		ApplyMaterialRenderState(*cmd.MaterialPtr);

		// Bind textures and material uniforms.
		cmd.MaterialPtr->Bind();

		// Upload object transform.
		UploadObjectUniforms(*shader, cmd.ModelMatrix);

		// Submit the mesh draw.
		IssueDraw(*cmd.MeshPtr);
	}

	TeardownPass(pass);
}

void RenderPass::ExecuteShadowPass(
	const std::vector<DrawCommand>& queue,
	Shader* shadowShader,
	bool hasDirectionalLight,
	unsigned int shadowFBO,
	int shadowMapWidth,
	int shadowMapHeight,
	const glm::mat4& lightSpaceMatrix)
{
	if (!hasDirectionalLight || shadowShader == nullptr)
		return;

	// Save the currently bound framebuffer and viewport
	GLint previousFBO = 0;
	GLint previousViewport[4] = { 0, 0, 0, 0 };

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
	glGetIntegerv(GL_VIEWPORT, previousViewport);

	// Render into the shadow map depth texure.
	glViewport(0, 0, shadowMapWidth, shadowMapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Shadow pass state.
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Common trick to reduce shadow acne a bit.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	shadowShader->Bind();
	shadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

	for (const DrawCommand& cmd : queue)
	{
		if (!cmd.MeshPtr || !cmd.CastsShadows)
			continue;

		shadowShader->SetMat4("u_Model", cmd.ModelMatrix);
		IssueDraw(*cmd.MeshPtr);
	}

	// Restore normal culling.
	glCullFace(GL_BACK);

	// Return to default framebuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);

	// Restore previous viewport.
	glViewport(
		previousViewport[0],
		previousViewport[1],
		previousViewport[2],
		previousViewport[3]
	);
}