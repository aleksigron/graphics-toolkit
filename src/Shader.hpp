#pragma once

#include <cstdint>

#include "Buffer.hpp"

struct StringRef;
class StackAllocator;

enum class ShaderRenderType
{
	Opaque,
	AlphaTest,
	Transparent
};

enum class ShaderUniformType
{
	Tex2D,
	TexCube,
	Mat4x4,
	Vec4,
	Vec3,
	Vec2,
	Float,
	Int
};

struct ShaderUniform
{
	int location;
	uint32_t nameHash;
	ShaderUniformType type;

	static const unsigned int TypeCount = 8;
	static const char* const TypeNames[TypeCount];
	static const unsigned int TypeSizes[TypeCount];
};

struct Shader
{
private:
	enum class ShaderType
	{
		Vertex,
		Fragment
	};

	bool Compile(ShaderType type, Buffer<char>& source, unsigned& idOut);
	bool CompileAndLink(Buffer<char>& vertexSource, Buffer<char>& fragmentSource);
	void AddMaterialUniforms(unsigned int count,
							 const ShaderUniformType* types,
							 const StringRef* names);

	StackAllocator* allocator;

public:
	uint32_t nameHash;

	unsigned int driverId;

	int uniformMatMVP;
	int uniformMatMV;
	int uniformMatVP;
	int uniformMatM;
	int uniformMatV;
	int uniformMatP;

	ShaderRenderType renderType;

	static const unsigned MaxMaterialUniforms = 8;

	unsigned int materialUniformCount;
	ShaderUniform materialUniforms[MaxMaterialUniforms];

	void SetAllocator(StackAllocator* allocator);
	
	bool LoadFromConfiguration(Buffer<char>& configuration);
};
