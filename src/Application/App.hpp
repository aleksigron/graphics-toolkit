#pragma once

#include "Application/AppSettings.hpp"
#include "Application/CameraController.hpp"
#include "Rendering/Camera.hpp"

class Allocator;

class App
{
private:
	static App* instance;

	Allocator* allocator;

	AppSettings settings;

	Camera mainCamera;
	CameraController cameraController;
	bool cameraControllerEnable;
	
public:
	App(Allocator* allocator);
	~App();
	
	void Initialize();
	void Update();

	void SetCameraControllerEnable(bool enable)
	{
		cameraControllerEnable = enable;
	}

	AppSettings* GetSettings() { return &settings; }

	static App* GetInstance() { return App::instance; }
};
