#pragma once

#include "Vec2.hpp"
#include "Mat4x4.hpp"

struct GLFWwindow;

class InputManager;

class Window
{
private:
	GLFWwindow* windowHandle;
	
	InputManager* inputManager;

	int currentSwapInterval;
	
public:
	Window();
	~Window();
	
	bool Initialize(const char* windowTitle);
	bool ShouldClose();
	void UpdateInput();
	void Swap();
	
	/*
	Get framebuffer size in pixels
	*/
	Vec2f GetFrameBufferSize();

	/*
	Get window content area in screen coordinates
	*/
	Vec2f GetWindowSize();

	/*
	Get screen coordinate scale compared to pixels
	*/
	float GetScreenCoordinateScale();

	/*
	Get orthographic projection matrix for screen coordinate space
	*/
	Mat4x4f GetScreenSpaceProjectionMatrix();

	/*
	Set the number of screen refresh to wait for until swapping buffers.
	0: vsync off, 1: vsync every refresh, n: vsync once every n refreshes
	*/
	void SetSwapInterval(int swapInterval);
	int GetSwapInterval() const { return currentSwapInterval; }

	InputManager* GetInputManager() { return inputManager; }
	
	static Window* GetWindowObject(GLFWwindow* windowHandle);
};
