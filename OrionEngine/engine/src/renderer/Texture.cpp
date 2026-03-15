#include "Texture.hpp"
#include <iostream>

// stb_image is a single-header image loader library
#define STB_IMAGE_IMPLEMENTATION // Only add this #define line once, and in a .cpp file
#include "stb_image/stb_image.h"

Texture::Texture() : m_TextureID(0), m_Width(0), m_Height(0), m_Channels(0)
{
}

bool Texture::LoadFromFile(const std::string& path)
{
	m_FilePath = path;

	// FLip images vertically while loading.
	// OpenGL UV origin is bottom-left, many image files are top-left.
	stbi_set_flip_vertically_on_load(1);

	// Load image pixels from disk
	unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);
	if (!data) {
		std::cout << "Failed to load texture: " << path << std::endl;
		std::cout << "stb_image reason: " << stbi_failure_reason() << std::endl;
		return false;
	}

	std::cout << "Loaded texture: " << path
		<< " | W: " << m_Width
		<< " H: " << m_Height
		<< " Channels: " << m_Channels << std::endl;


	GLenum format = GL_RGB;

	if (m_Channels == 1)
		format = GL_RED;
	else if (m_Channels == 3)
		format = GL_RGB;
	else if (m_Channels == 4)
		format = GL_RGBA;
	else {
		std::cout << "Unsupported channel count: " << m_Channels << std::endl;
		stbi_image_free(data);
		return false;
	}

	// Generate one OpenGL texture object
	glGenTextures(1, &m_TextureID);

	// Bind this texture so all following texture calls affect this texture object
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	// Bind texture wrapping behavior when UVs go outside [0,1]
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // U direction
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // V direction
	
	// Set filtering behavior when texture is scaled
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // minification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // magnification

	// Upload raw pixel data into the currently bound OpenGL texture
	glTexImage2D(
		GL_TEXTURE_2D,	// target
		0,				// mip level
		format,			// internal format stored by OpenGL
		m_Width,		// texture width
		m_Height,		// texture height
		0,				// border, must be 0
		format,			// format of incoming pixel data
		GL_UNSIGNED_BYTE,//type of incoming pixel data
		data			// pointer to image pixels
	);

	// Generate mipmaps for the currently bound texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// Free CPU-side image data now that OpenGL has its own copy
	stbi_image_free(data);

	return true;
}

void Texture::Bind(unsigned int slot) const
{
	// Select which texture we want to bind to
	glActiveTexture(GL_TEXTURE0 + slot);

	// Bind this texture object to GL_TEXTURE_2D on that texture unit
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::Unbind() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Destroy()
{
	if (m_TextureID) {
		glDeleteTextures(1, &m_TextureID);
		m_TextureID = 0;
	}
}