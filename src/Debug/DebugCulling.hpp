#pragma once

#include "Math/Vec2.hpp"
#include "Rendering/Camera.hpp"
#include "Application/CameraController.hpp"

class DebugTextRenderer;
class DebugVectorRenderer;

class DebugCulling
{
private:
	DebugTextRenderer* textRenderer;
	DebugVectorRenderer* vectorRenderer;

	bool cullingCameraIsLocked;

	Vec2f guideTextPosition;

public:
	DebugCulling(DebugTextRenderer* textRenderer, DebugVectorRenderer* vectorRenderer);
	~DebugCulling();

	void UpdateAndDraw(Scene* scene);

	void SetLockCullingCamera(bool lockCullingCamera);
	void SetGuideTextPosition(const Vec2f& pos) { guideTextPosition = pos; }
};
