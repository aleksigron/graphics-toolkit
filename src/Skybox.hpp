#pragma once

#include "Vec3.hpp"

class Scene;

class Skybox
{
private:
	Scene* renderScene;

	unsigned int renderObjectId;
	unsigned int sceneObjectId;

public:
	Skybox();
	~Skybox();

	void Initialize(Scene* scene, unsigned int materialId);
	void UpdateTransform(const Vec3f& cameraPosition) const;
};
