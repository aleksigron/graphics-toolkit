#include "Application/CameraController.hpp"

#include <cstdio>
#include <cstdlib>

#include "Core/String.hpp"

#include "Math/Mat3x3.hpp"
#include "Math/Mat4x4.hpp"

#include "Engine/Engine.hpp"
#include "System/Time.hpp"
#include "System/Window.hpp"
#include "System/InputManager.hpp"
#include "System/PointerInput.hpp"
#include "System/KeyboardInputView.hpp"

#include "Scene/SceneManager.hpp"
#include "Scene/Scene.hpp"

#include "App.hpp"
#include "Rendering/Camera.hpp"

CameraController::CameraController(SceneManager* sceneManager, Window* window) :
	sceneManager(sceneManager),
	window(window),
	controlledCamera(nullptr)
{
}

CameraController::~CameraController()
{
}

void CameraController::SetControlledCamera(Camera* camera)
{
	controlledCamera = camera;
}

void CameraController::VerifySensitityIsLoaded()
{
	if (cameraAimSensitivity < 0.0f)
	{
		AppSettings* settings = App::GetInstance()->GetSettings();

		double sensitivity = 0.0;
		if (settings->TryGetDouble("camera_aim_sensitivity", sensitivity))
		{
			this->cameraAimSensitivity = static_cast<float>(sensitivity);
		}
		else
		{
			this->cameraAimSensitivity = 1.0f;

			settings->SetDouble("camera_aim_sensitivity", this->cameraAimSensitivity);
			settings->SaveToFile();
		}
	}
}

void CameraController::Update()
{
	static const int MouseButtonGrab = 0;
	static const int MouseButtonLook = 1;

	this->VerifySensitityIsLoaded();

	Scene* scene = sceneManager->GetScene(sceneManager->GetPrimarySceneId());
	InputManager* inputManager = window->GetInputManager();
	PointerInput* pi = inputManager->GetPointerInput();
	KeyboardInputView* kb = inputManager->GetKeyboardInputView();

	// Update mouseLookActive state
	if (mouseLookActive == false && pi->GetMouseButtonDown(MouseButtonLook))
	{
		mouseLookActive = true;
		pi->SetCursorMode(PointerInput::CursorMode::Disabled);
	}
	else if (mouseLookActive == true && pi->GetMouseButtonUp(MouseButtonLook))
	{
		mouseLookActive = false;
		pi->SetCursorMode(PointerInput::CursorMode::Normal);
	}

	// Update mouse look
	if (mouseLookActive == true)
	{
		Vec2f movement = pi->GetCursorMovement() * 0.003f * this->cameraAimSensitivity;

		cameraYaw += movement.x;

		if (cameraYaw < -Math::Const::Pi)
			cameraYaw += Math::Const::Tau;
		else if (cameraYaw >= Math::Const::Pi)
			cameraYaw -= Math::Const::Tau;

		cameraPitch += movement.y;

		if (cameraPitch < Math::DegreesToRadians(-80.0f))
			cameraPitch = Math::DegreesToRadians(-80.0f);
		else if (cameraPitch > Math::DegreesToRadians(80.0f))
			cameraPitch = Math::DegreesToRadians(80.0f);
	}

	Mat3x3f rotation = Mat3x3f::RotateAroundAxis(Vec3f(0.0f, 1.0f, 0.0f), -cameraYaw) *
	Mat3x3f::RotateAroundAxis(Vec3f(1.0f, 0.0f, 0.0f), -cameraPitch);

	Vec3f up = rotation.Up();
	Vec3f right = rotation.Right();
	Vec3f forward = rotation.Forward();

	Entity cameraEntity = controlledCamera->GetEntity();
	SceneObjectId cameraSceneObject = scene->Lookup(cameraEntity);
	Mat4x4f currentTransform = scene->GetLocalTransform(cameraSceneObject);
	Vec3f position = (currentTransform * Vec4f(0.0f, 0.0f, 0.0f, 1.0f)).xyz();

	if (mouseLookActive == false)
	{
		if (pi->GetMouseButtonDown(MouseButtonGrab))
			mouseGrabActive = true;
		else if (pi->GetMouseButtonUp(MouseButtonGrab))
			mouseGrabActive = false;

		if (mouseGrabActive)
		{
			Vec2f movement = pi->GetCursorMovement() * 0.015f;
			position += right * -movement.x + up * movement.y;
		}
	}

	Vec3f dir;

	dir -= float(int(kb->GetKey(Key::Q))) * up;
	dir += float(int(kb->GetKey(Key::W))) * forward;
	dir += float(int(kb->GetKey(Key::E))) * up;
	dir -= float(int(kb->GetKey(Key::A))) * right;
	dir -= float(int(kb->GetKey(Key::S))) * forward;
	dir += float(int(kb->GetKey(Key::D))) * right;

	if (dir.SqrMagnitude() > 1.0f)
		dir.Normalize();

	float targetSpeed = cameraSpeed;

	if (kb->GetKey(Key::LeftShift))
		targetSpeed *= 4.0f;

	if (kb->GetKey(Key::LeftControl))
		targetSpeed *= 0.25f;

	cameraVelocity += (dir * targetSpeed - cameraVelocity) * 0.15f;
	position += cameraVelocity * Time::GetDeltaTime();

	Mat4x4f newTransform = Mat4x4f::Translate(position) * Mat4x4f(rotation);
	scene->SetLocalTransform(cameraSceneObject, newTransform);
}
