#pragma once
#include <string>
#include <glad/glad.h>


class Texture {
public:
	Texture();

	// Loads image data from disk and uploads it to OpenGL
	bool LoadFromFile(const std::string& path);

	// Binds this texture to a texture slot/unit
	void Bind(unsigned int slot = 0) const;

	// Unbinds any texture from GL_TEXTURE_2D target
	void Unbind() const;

	// Deletes the OpenGL texture object
	void Destroy();

	unsigned int GetID() const { return m_TextureID; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	const std::string& GetPath() const { return m_FilePath; }

private:
	unsigned int m_TextureID;
	int m_Width;
	int m_Height;
	int m_Channels;
	std::string m_FilePath;
};