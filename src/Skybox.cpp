#include "Skybox.hpp"

#include "Mat4x4.hpp"

#include "Engine.hpp"
#include "Scene.hpp"
#include "SceneLayer.hpp"
#include "SceneManager.hpp"
#include "Renderer.hpp"
#include "MeshManager.hpp"
#include "EntityManager.hpp"
#include "ResourceManager.hpp"

#include "VertexFormat.hpp"

Skybox::Skybox() :
	renderSceneId(0)
{
	entity = Entity{};
}

Skybox::~Skybox()
{
}

void Skybox::Initialize(Scene* scene, unsigned int materialId)
{
	static Vertex3f vertexData[] = {
		Vertex3f{ Vec3f(-0.5f, -0.5f, -0.5f) },
		Vertex3f{ Vec3f(0.5f, -0.5f, -0.5f) },
		Vertex3f{ Vec3f(-0.5f, -0.5f, 0.5f) },
		Vertex3f{ Vec3f(0.5f, -0.5f, 0.5f) },
		Vertex3f{ Vec3f(-0.5f, 0.5f, -0.5f) },
		Vertex3f{ Vec3f(0.5f, 0.5f, -0.5f) },
		Vertex3f{ Vec3f(-0.5f, 0.5f, 0.5f) },
		Vertex3f{ Vec3f(0.5f, 0.5f, 0.5f) }
	};

	static unsigned short indexData[] = {
		0, 5, 4, 0, 1, 5,
		4, 7, 6, 4, 5, 7,
		5, 3, 7, 5, 1, 3,
		2, 1, 0, 2, 3, 1,
		0, 6, 2, 0, 4, 6,
		3, 6, 7, 3, 2, 6
	};

	Engine* engine = Engine::GetInstance();
	Renderer* renderer = engine->GetRenderer();
	EntityManager* entityManager = engine->GetEntityManager();
	MeshManager* meshManager = engine->GetMeshManager();

	MeshId meshId = meshManager->CreateMesh();

	IndexedVertexData<Vertex3f, unsigned short> data;
	data.primitiveMode = MeshPrimitiveMode::Triangles;
	data.vertData = vertexData;
	data.vertCount = sizeof(vertexData) / sizeof(Vertex3f);
	data.idxData = indexData;
	data.idxCount = sizeof(indexData) / sizeof(unsigned short);

	meshManager->Upload_3f(meshId, data);

	BoundingBox bounds;
	bounds.center = Vec3f(0.0f, 0.0f, 0.0f);
	bounds.extents = Vec3f(0.5f, 0.5f, 0.5f);
	meshManager->SetBoundingBox(meshId, bounds);

	this->renderSceneId = scene->GetSceneId();
	this->entity = entityManager->Create();

	// Add scene object
	scene->AddSceneObject(this->entity);

	// Add render object
	RenderObjectId renderObjectId = renderer->AddRenderObject(this->entity);
	renderer->SetMeshId(renderObjectId, meshId);
	renderer->SetMaterialId(renderObjectId, materialId);
	renderer->SetSceneLayer(renderObjectId, SceneLayer::Skybox);
}

void Skybox::UpdateTransform(const Vec3f& cameraPosition) const
{
	Mat4x4f skyboxTransform = Mat4x4f::Translate(cameraPosition);

	Scene* scene = Engine::GetInstance()->GetSceneManager()->GetScene(renderSceneId);
	SceneObjectId sceneObject = scene->Lookup(entity);
	scene->SetLocalTransform(sceneObject, skyboxTransform);
}
