#pragma once
#include <string>
#include <glm/glm.hpp>


class Shader {
public:
	Shader() = default;
	~Shader();

	// Creates and links a shader program from vertex and fragment source.
	bool Create(const char* vertexSrc, const char* fragmentSrc);
	// Create by loading shader text from files.
	bool CreateFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
	// Delets the shader program if it exists
	void Destroy();

	// Makes this shader the active program for furture draw calls
	void Bind() const;
	// Unbinds any active shader program.
	void Unbind() const;

	void SetInt(const char* name, int value) const;
	void SetFloat(const char* name, float value) const;
	void SetMat4(const char* name, const glm::mat4& value) const;
	void SetVec4(const char* name, const glm::vec4& value) const;
	void SetVec3(const char* name, const glm::vec3& value) const;

	unsigned int GetProgramID() const { return m_ProgramID; }
	bool IsValid() const { return m_ProgramID != 0; }

private:
	unsigned int CompileShader(unsigned int type, const char* source);

	std::string ReadTextFile(const std::string& path) const;

private:
	unsigned int m_ProgramID = 0;
};