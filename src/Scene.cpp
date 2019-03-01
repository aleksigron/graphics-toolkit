#include "Scene.hpp"

Scene::Scene(unsigned int sceneId) : id(sceneId), activeCamera(nullptr)
{
	this->objectBatch = new SceneObjectBatch;

	// Conceptual root object uses the index 0
	objectBatch->used = 1;

	// Set root object world transform
	objectBatch->worldTransforms[Scene::Root] = Mat4x4f();
}

Scene::~Scene()
{
	delete this->objectBatch;
}

Scene& Scene::operator=(Scene&& other)
{
	this->id = other.id;
	this->objectBatch = other.objectBatch;

	other.id = 0;
	other.objectBatch = nullptr;

	return *this;
}

unsigned int Scene::AddSceneObject()
{
	unsigned int id;

	if (objectBatch->used < SceneObjectBatch::BatchSize)
	{
		id = objectBatch->used++;
	}
	else
	{
		// TODO: Allocate a new batch
		id = 0;
	}

	objectBatch->parentIds[id] = Scene::Root;

	return id;
}

void Scene::SetParent(unsigned int object, unsigned int parent)
{
	objectBatch->parentIds[object] = parent;
}

void Scene::SetLocalTransform(unsigned int object, const Mat4x4f& transform)
{
	objectBatch->localTransforms[object] = transform;
}

void Scene::CalculateWorldTransforms()
{
	Mat4x4f* local = objectBatch->localTransforms;
	Mat4x4f* world = objectBatch->worldTransforms;
	unsigned int* parent = objectBatch->parentIds;

	for (unsigned i = 1; i < objectBatch->used; ++i)
	{
		world[i] = world[parent[i]] * local[i];
	}
}

void Scene::SetActiveCamera(Camera* camera)
{
	this->activeCamera = camera;
}
