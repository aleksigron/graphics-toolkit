#pragma once

#include <cstdint>

struct Shader;
struct Texture;

class ResourceManager
{
private:
	// Shaders

	Shader* shaders = nullptr;
	unsigned int shaderCount = 0;
	unsigned int shaderAllocated = 0;

	bool LoadShader(Shader& shader, const char* configPath);

	// Textures

	Texture* textures = nullptr;
	unsigned int textureCount = 0;
	unsigned int textureAllocated = 0;

	bool LoadTexture(Texture* texture, const char* texturePath);

public:
	ResourceManager();
	~ResourceManager();

	Shader* GetShader(uint32_t hash) const;
	Shader* GetShader(const char* path);

	Texture* GetTexture(uint32_t hash) const;
	Texture* GetTexture(const char* path);
	Texture* CreateTexture();
};
