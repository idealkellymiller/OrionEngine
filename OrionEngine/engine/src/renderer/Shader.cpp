#include "Shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>


Shader::~Shader()
{
	Destroy();
}

bool Shader::Create(const char* vertexSrc, const char* fragmentSrc)
{
	Destroy();

	unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
	if (vertexShader == 0)
		return false;

	unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
	if (fragmentShader == 0)
		return false;

	// Create a program object that will link the shader stages together
	unsigned int program = glCreateProgram();

	// Attach both compiled objects to the program
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	// Link the attached shaders into a single usable GPU program
	glLinkProgram(program);

	int success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cout << "Shader link failed:\n" << infoLog << "\n";

		glDeleteProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return false;
	}

	// Once the program is linked, the standalone shader objects can eb deleted.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	m_ProgramID = program;
	return true;
}

std::string Shader::ReadTextFile(const std::string& path) const
{
	// Open the file in text mode.
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cout << "Failed to open shader file: " << path << "\n";
		return "";
	}

	// Read the entire file into a string stream.
	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

bool Shader::CreateFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
	// Load shader source text from disk
	std::string vertexSrc = ReadTextFile(vertexPath);
	std::string fragmentSrc = ReadTextFile(fragmentPath);

	if (vertexSrc.empty() || fragmentSrc.empty())
	{
		return false;
	}

	// Reuse existing Create(...) path
	return Create(vertexSrc.c_str(), fragmentSrc.c_str());
}

void Shader::Destroy()
{
	if (m_ProgramID == 0) {
		// Delete the GPU shader program object.
		glDeleteProgram(m_ProgramID);
		m_ProgramID = 0;
	}
}

void Shader::Bind() const
{
	// Make this shader program active.
	glUseProgram(m_ProgramID);
}

void Shader::Unbind() const
{
	// Binds no shader program.
	glUseProgram(0);
}


void Shader::SetInt(const char* name, int value) const
{
	int location = glGetUniformLocation(m_ProgramID, name);
	glUniform1i(location, value);
}

void Shader::SetFloat(const char* name, float value) const
{
	int location = glGetUniformLocation(m_ProgramID, name);
	glUniform1f(location, value);
}

void Shader::SetMat4(const char* name, const glm::mat4& value) const
{
	int location = glGetUniformLocation(m_ProgramID, name);
	// Uplaod a 4x4 float matrix to the shader uniform.
	//
	// Parameters:
	// - location: which uniforms we are setting
	// - 1: number of matrices being uploaded
	// - GL_FALSE: do not transpose; GLM already matches OpenGL's expected layout
	// - glm::value_ptr(value): pointer to the first float in the matrix
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec4(const char* name, const glm::vec4& value) const
{
	int location = glGetUniformLocation(m_ProgramID, name);
	glUniform4f(location, value.x, value.y, value.z, value.w);
}

void Shader::SetVec3(const char* name, const glm::vec3& value) const
{
	int location = glGetUniformLocation(m_ProgramID, name);
	glUniform3f(location, value.x, value.y, value.z);
}




unsigned int Shader::CompileShader(unsigned int type, const char* source)
{
	// Creates an empty shader object of the requested type
	unsigned int shader = glCreateShader(type);

	// Sets the GLSL source code for the shader
	glShaderSource(shader, 1, &source, nullptr);

	// Compiles the shader source into GPU-executable code.
	glCompileShader(shader);

	int success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);

		if (type == GL_VERTEX_SHADER)
			std::cout << "Vertex shader compile failed:\n" << infoLog << "\n";
		else if (type == GL_FRAGMENT_SHADER)
			std::cout << "Fragment shader compile failed:\n" << infoLog << "\n";
		else
			std::cout << "Shader compile failed:\n" << infoLog << "\n";

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

