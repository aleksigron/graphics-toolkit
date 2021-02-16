#include "Rendering/TerrainInstance.hpp"

#include <cmath>

#include "Debug/Debug.hpp"

#include "Memory/Allocator.hpp"

#include "Rendering/RenderDevice.hpp"
#include "Rendering/StaticUniformBuffer.hpp"
#include "Rendering/VertexFormat.hpp"

#include "Resources/MeshManager.hpp"
#include "Resources/MaterialManager.hpp"
#include "Resources/ShaderManager.hpp"

#include "System/IncludeOpenGL.hpp"

TerrainInstance::TerrainInstance(
	Allocator* allocator,
	RenderDevice* renderDevice,
	MeshManager* meshManager,
	ShaderManager* shaderManager) :
	allocator(allocator),
	renderDevice(renderDevice),
	meshManager(meshManager),
	shaderManager(shaderManager),
	heightValues(allocator),
	terrainSize(64.0f),
	terrainResolution(64),
	minHeight(0.0f),
	maxHeight(2.0f),
	heightData(nullptr),
	vertexArrayId(0),
	textureId()
{
	meshId = MeshId{ 0 };
}

TerrainInstance::~TerrainInstance()
{
	allocator->Deallocate(heightData);

	if (vertexArrayId != 0)
		renderDevice->DestroyVertexArrays(1, &vertexArrayId);

	if (textureId != 0)
		renderDevice->DestroyTextures(1, &textureId);
}

void TerrainInstance::Initialize()
{
	// Create texture data

	size_t texSize = terrainResolution;
	float texSizeInv = 1.0f / texSize;
	float scale = 16.0f;
	size_t dataSizeBytes = texSize * texSize * sizeof(uint16_t);
	heightData = static_cast<uint16_t*>(allocator->Allocate(dataSizeBytes));

	for (size_t y = 0; y < texSize; ++y)
	{
		for (size_t x = 0; x < texSize; ++x)
		{
			size_t pixelIndex = y * texSize + x;
			float normalized = 0.5f + std::sin(x * texSizeInv * scale) * 0.25f + std::sin(y * texSizeInv * scale) * 0.25f;
			uint16_t height = static_cast<uint16_t>(normalized * UINT16_MAX);
			heightData[pixelIndex] = height;
		}
	}

	renderDevice->CreateTextures(1, &textureId);
	renderDevice->BindTexture(RenderTextureTarget::Texture2d, textureId);

	RenderCommandData::SetTextureStorage2D storage{
		RenderTextureTarget::Texture2d, 1, GL_R16, texSize, texSize
	};
	renderDevice->SetTextureStorage2D(&storage);

	RenderCommandData::SetTextureSubImage2D subimage{
		RenderTextureTarget::Texture2d, 0, 0, 0, texSize, texSize, GL_RED, GL_UNSIGNED_SHORT, heightData
	};
	renderDevice->SetTextureSubImage2D(&subimage);

	// Create vertex data

	size_t sideVerts = terrainResolution + 1;
	size_t vertCount = sideVerts * sideVerts;
	size_t vertexComponents = 2;
	size_t vertSize = sizeof(float) * vertexComponents;
	float* vertexData = static_cast<float*>(allocator->Allocate(vertCount * vertSize));

	size_t sideQuads = terrainResolution;
	size_t quadIndices = 3 * 2; // 3 indices per triangle, 2 triangles per quad
	size_t indexCount = sideQuads * sideQuads * quadIndices;
	unsigned short* indexData = static_cast<unsigned short*>(allocator->Allocate(indexCount * sizeof(unsigned short)));

	float origin = terrainSize * -0.5f;
	float quadSize = terrainSize / terrainResolution;

	// Set vertex data
	for (size_t x = 0; x < sideVerts; ++x)
	{
		for (size_t y = 0; y < sideVerts; ++y)
		{
			size_t vertIndex = y * sideVerts + x;
			vertexData[vertIndex * vertexComponents + 0] = origin + x * quadSize;
			vertexData[vertIndex * vertexComponents + 1] = origin + y * quadSize;
		}
	}

	// Set index data
	for (size_t x = 0; x < sideQuads; ++x)
	{
		for (size_t y = 0; y < sideQuads; ++y)
		{
			size_t quad = y * sideQuads + x;
			indexData[quad * quadIndices + 0] = y * sideVerts + x;
			indexData[quad * quadIndices + 1] = (y + 1) * sideVerts + x;
			indexData[quad * quadIndices + 2] = y * sideVerts + (x + 1);
			indexData[quad * quadIndices + 3] = (y + 1) * sideVerts + x;
			indexData[quad * quadIndices + 4] = (y + 1) * sideVerts + (x + 1);
			indexData[quad * quadIndices + 5] = y * sideVerts + (x + 1);
		}
	}

	VertexAttribute vertexAttributes[] = { VertexAttribute::pos2 };
	VertexFormat vertexFormatPos(vertexAttributes, sizeof(vertexAttributes) / sizeof(vertexAttributes[0]));

	IndexedVertexData data;
	data.vertexFormat = vertexFormatPos;
	data.primitiveMode = RenderPrimitiveMode::Triangles;
	data.vertexData = vertexData;
	data.vertexCount = vertCount;
	data.indexData = indexData;
	data.indexCount = indexCount;
	
	meshId = meshManager->CreateMesh();
	meshManager->UploadIndexed(meshId, data);

	BoundingBox bounds;
	bounds.center = Vec3f(0.0f, 0.0f, 0.0f);
	bounds.extents = Vec3f(terrainSize * 0.5f, terrainSize * 0.5f, terrainSize * 0.5f);
	meshManager->SetBoundingBox(meshId, bounds);

	allocator->Deallocate(indexData);
	allocator->Deallocate(vertexData);
}


void TerrainInstance::RenderTerrain(const MaterialData& material)
{
	renderDevice->UseShaderProgram(material.cachedShaderDeviceId);

	const ShaderData& shader = shaderManager->GetShaderData(material.shaderId);
	const TextureUniform* tu = shader.uniforms.FindTextureUniformByNameHash("height_map"_hash);
	if (tu != nullptr)
	{
		renderDevice->SetUniformInt(tu->uniformLocation, 0);
		renderDevice->SetActiveTextureUnit(0);
		renderDevice->BindTexture(RenderTextureTarget::Texture2d, textureId);
	}

	// Bind material uniform block to shader
	renderDevice->BindBufferBase(RenderBufferTarget::UniformBuffer, MaterialUniformBlock::BindingPoint, material.uniformBufferObject);


	MeshDrawData* draw = meshManager->GetDrawData(meshId);
	renderDevice->BindVertexArray(draw->vertexArrayObject);
	renderDevice->DrawIndexed(draw->primitiveMode, draw->count, draw->indexType);
}