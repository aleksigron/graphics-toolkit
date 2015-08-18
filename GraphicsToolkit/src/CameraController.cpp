#include "CameraController.h"

#include "App.h"
#include "Time.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

void CameraController::KeyCallback(GLFWwindow* window, int key, int code, int act, int mods)
{
	if (key == GLFW_KEY_SPACE && act == GLFW_PRESS)
	{
		CameraController* cc = static_cast<CameraController*>(glfwGetWindowUserPointer(window));

		if (cc->mouseControlEnable)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cc->mouseControlEnable = false;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cc->mouseControlEnable = true;
		}
	}
}

void CameraController::SetControlledCamera(Camera* camera)
{
	controlledCamera = camera;

	GLFWwindow* window = App::GetMainWindow()->GetWindowHandle();
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, CameraController::KeyCallback);
}

void CameraController::Update()
{
	GLFWwindow* window = App::GetMainWindow()->GetWindowHandle();

	Transform& cameraTransform = controlledCamera->transform;

	if (mouseControlEnable)
	{
		Vec2d cursorPos;
		glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);

		Vec2d move = (cursorPos - prevCursorPos) * 0.005;
		prevCursorPos = cursorPos;

		cameraTransform.rotation = Matrix::Rotate3(cameraTransform.Right(), static_cast<float>(-move.y)) *
		Matrix::Rotate3(cameraTransform.Up(), static_cast<float>(-move.x)) * cameraTransform.rotation;
	}

	KeyboardInput* kb = &(App::GetInput()->keyboard);

	Vec3f dir;

	dir -= float(int(kb->GetKey(Key::W))) * cameraTransform.Forward();
	dir -= float(int(kb->GetKey(Key::A))) * cameraTransform.Right();
	dir += float(int(kb->GetKey(Key::S))) * cameraTransform.Forward();
	dir += float(int(kb->GetKey(Key::D))) * cameraTransform.Right();

	if (dir.SqrMagnitude() > 1.0f)
		dir.Normalize();

	cameraVelocity += (dir * cameraMaximumSpeed - cameraVelocity) * 0.15f;

	cameraTransform.position += cameraVelocity * Time::GetDeltaTime();
}
