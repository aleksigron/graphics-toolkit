#pragma once

#include "Core/Array.hpp"
#include "Core/BitPack.hpp"
#include "Core/HashMap.hpp"

#include "Entity/Entity.hpp"

#include "Math/Mat4x4.hpp"
#include "Math/Vec3.hpp"
#include "Math/Vec2.hpp"

#include "Resources/MeshData.hpp"
#include "Resources/MaterialData.hpp"
#include "Resources/ShaderId.hpp"

#include "Rendering/CustomRenderer.hpp"
#include "Rendering/Light.hpp"
#include "Rendering/RenderCommandList.hpp"
#include "Rendering/RendererData.hpp"
#include "Rendering/RenderOrder.hpp"

#include "Scene/ITransformUpdateReceiver.hpp"

class Allocator;
class Camera;
class LightManager;
class ShaderManager;
class MeshManager;
class MaterialManager;
class EntityManager;
class RenderDevice;
class Scene;
class Window;
class DebugVectorRenderer;
class CustomRenderer;
class ScreenSpaceAmbientOcclusion;
class BloomEffect;
class PostProcessRenderer;
class RenderTargetContainer;

struct BoundingBox;
struct RendererFramebuffer;
struct RenderViewport;
struct MaterialData;
struct ShaderData;
struct ProjectionParameters;
struct LightingUniformBlock;
struct PostProcessRenderPass;

class Renderer : public ITransformUpdateReceiver, public CustomRenderer
{
private:

	static const unsigned int MaxViewportCount = 8;
	static const unsigned int MaxFramebufferCount = 4;
	static const unsigned int MaxFramebufferTextureCount = 16;

	static const unsigned int FramebufferIndexGBuffer = 0;
	static const unsigned int FramebufferIndexShadow = 1;
	static const unsigned int FramebufferIndexLightAcc = 2;

	static const unsigned int ObjectUniformBufferSize = 512 * 1024;

	Allocator* allocator;
	RenderDevice* device;
	RenderTargetContainer* renderTargetContainer;
	PostProcessRenderer* postProcessRenderer;

	ScreenSpaceAmbientOcclusion* ssao;
	BloomEffect* bloomEffect;

	RendererFramebuffer* framebufferData;
	unsigned int framebufferCount;

	unsigned int* framebufferTextures;
	unsigned int framebufferTextureCount;

	RenderViewport* viewportData;
	unsigned int viewportCount;
	unsigned int viewportIndexFullscreen;

	MeshId fullscreenMesh;
	ShaderId lightingShaderId;
	ShaderId tonemappingShaderId;
	MaterialId shadowMaterial;
	unsigned int lightingUniformBufferId;
	unsigned int tonemapUniformBufferId;

	unsigned int gBufferAlbedoTextureIndex;
	unsigned int gBufferNormalTextureIndex;
	unsigned int gBufferMaterialTextureIndex;
	unsigned int fullscreenDepthTextureIndex;
	unsigned int shadowDepthTextureIndex;
	unsigned int lightAccumulationTextureIndex;

	Array<unsigned int> objectUniformBuffers;

	size_t objectUniformBlockStride;
	unsigned int objectsPerUniformBuffer;

	unsigned int deferredLightingCallback;
	unsigned int postProcessCallback;

	struct InstanceData
	{
		unsigned int count;
		unsigned int allocated;
		void *buffer;

		Entity* entity;
		MeshId* mesh;
		RenderOrderData* order;
		BoundingBox* bounds;
		Mat4x4f* transform;
	}
	data;

	HashMap<unsigned int, RenderObjectId> entityMap;

	RenderOrderConfiguration renderOrder;

	LightManager* lightManager;
	ShaderManager* shaderManager;
	MeshManager* meshManager;
	MaterialManager* materialManager;

	bool lockCullingCamera;
	Mat4x4f lockCullingCameraTransform;

	RenderCommandList commandList;
	Array<BitPack> objectVisibility;

	Array<LightId> lightResultArray;

	Array<CustomRenderer*> customRenderers;

	Entity skyboxEntity;

	void ReallocateRenderObjects(unsigned int required);

	void BindMaterialTextures(const MaterialData& material) const;
	void BindTextures(const ShaderData& shader, unsigned int count,
		const uint32_t* nameHashes, const unsigned int* textures);

	void UpdateLightingDataToUniformBuffer(
		const ProjectionParameters& projection, const Scene* scene, LightingUniformBlock& uniformsOut);

	// Returns the number of object draw commands added
	unsigned int PopulateCommandList(Scene* scene);

	void UpdateUniformBuffers(unsigned int objectDrawCount);

	bool IsDrawCommand(uint64_t orderKey);
	bool ParseControlCommand(uint64_t orderKey);

	void RenderDeferredLighting(const CustomRenderer::RenderParams& params);
	void RenderPostProcess(const CustomRenderer::RenderParams& params);
	void RenderBloom(const CustomRenderer::RenderParams& params);
	void RenderTonemapping(const CustomRenderer::RenderParams& params);

	void DebugRender(DebugVectorRenderer* vectorRenderer);
	
public:
	Renderer(Allocator* allocator, RenderDevice* renderDevice,
		LightManager* lightManager, ShaderManager* shaderManager,
		MeshManager* meshManager, MaterialManager* materialManager);
	~Renderer();

	void Initialize(Window* window, EntityManager* entityManager);
	void Deinitialize();

	void SetLockCullingCamera(bool lockEnable) { lockCullingCamera = lockEnable; }
	const Mat4x4f& GetCullingCameraTransform() { return lockCullingCameraTransform; }

	// Render the specified scene to the active OpenGL context
	void Render(Scene* scene);

	virtual void NotifyUpdatedTransforms(unsigned int count, const Entity* entities, const Mat4x4f* transforms);

	// Render object management

	RenderObjectId Lookup(Entity e)
	{
		HashMap<unsigned int, RenderObjectId>::KeyValuePair* pair = entityMap.Lookup(e.id);
		return pair != nullptr ? pair->second : RenderObjectId{};
	}

	RenderObjectId AddRenderObject(Entity entity);
	void AddRenderObject(unsigned int count, const Entity* entities, RenderObjectId* renderObjectIdsOut);

	// Render object property management

	void SetMeshId(RenderObjectId id, MeshId meshId) { data.mesh[id.i] = meshId; }
	MeshId GetMeshId(RenderObjectId id) { return data.mesh[id.i]; }

	void SetOrderData(RenderObjectId id, const RenderOrderData& order)
	{
		data.order[id.i] = order;
	}

	virtual void RenderCustom(const CustomRenderer::RenderParams& params) override final;

	// Custom renderer management
	unsigned int AddCustomRenderer(CustomRenderer* customRenderer);
	void RemoveCustomRenderer(unsigned int callbackId);
};
