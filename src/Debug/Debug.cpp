#include "Debug/Debug.hpp"

#include <cassert>
#include <cstdio>

#include "Engine/Engine.hpp"

#include "Debug/DebugVectorRenderer.hpp"
#include "Debug/DebugTextRenderer.hpp"
#include "Debug/DebugGraph.hpp"
#include "Debug/DebugCulling.hpp"
#include "Debug/DebugConsole.hpp"
#include "Debug/DebugLog.hpp"
#include "Debug/DebugMemoryStats.hpp"
#include "Debug/LogHelper.hpp"

#include "Rendering/Renderer.hpp"
#include "Rendering/RenderDevice.hpp"

#include "Resources/BitmapFont.hpp"
#include "Resources/ShaderManager.hpp"

#include "Scene/Scene.hpp"

#include "System/InputManager.hpp"
#include "System/KeyboardInputView.hpp"
#include "System/Time.hpp"
#include "System/Window.hpp"

static void RenderDebugCallback(const RenderDevice::DebugMessage& message)
{
	Log::Log(
		message.severity == RenderDebugSeverity::Notification ? LogLevel::Info : LogLevel::Warning,
		message.message.str, message.message.len);
}

Debug::Debug(
	Allocator* allocator,
	AllocatorManager* allocManager,
	Window* window,
	RenderDevice* renderDevice) :
	allocator(allocator),
	renderDevice(renderDevice),
	window(nullptr),
	currentFrameRate(0.0),
	nextFrameRateUpdate(-1.0),
	mode(DebugMode::None)
{
	vectorRenderer = allocator->MakeNew<DebugVectorRenderer>(allocator, renderDevice);
	textRenderer = allocator->MakeNew<DebugTextRenderer>(allocator, renderDevice);
	graph = allocator->MakeNew<DebugGraph>(allocator, vectorRenderer);
	culling = allocator->MakeNew<DebugCulling>(textRenderer, vectorRenderer);
	console = allocator->MakeNew<DebugConsole>(allocator, window, textRenderer, vectorRenderer);
	log = allocator->MakeNew<DebugLog>(allocator, console);

	// Set up log instance in LogHelper
	Log::SetLogInstance(log);

	memoryStats = allocator->MakeNew<DebugMemoryStats>(allocManager, textRenderer);
}

Debug::~Debug()
{
	allocator->MakeDelete(memoryStats);

	// Clear log instance in LogHelper
	Log::SetLogInstance(nullptr);

	allocator->MakeDelete(log);
	allocator->MakeDelete(console);
	allocator->MakeDelete(culling);
	allocator->MakeDelete(graph);
	allocator->MakeDelete(textRenderer);
	allocator->MakeDelete(vectorRenderer);
}

void Debug::Initialize(Window* window, Renderer* renderer,
	MeshManager* meshManager, ShaderManager* shaderManager, SceneManager* sceneManager)
{
	renderDevice->SetDebugMessageCallback(RenderDebugCallback);

	this->window = window;

	textRenderer->Initialize(shaderManager, meshManager);
	vectorRenderer->Initialize(meshManager, shaderManager, sceneManager, window);

	Vec2f frameSize = this->window->GetFrameBufferSize().As<float>();
	float screenCoordScale = this->window->GetScreenCoordinateScale();

	textRenderer->SetFrameSize(frameSize);
	textRenderer->SetScaleFactor(screenCoordScale);

	float scaledLineHeight = 0;
	const BitmapFont* font = textRenderer->GetFont();

	if (font != nullptr)
		scaledLineHeight = static_cast<float>(font->GetLineHeight());

	float pixelLineHeight = scaledLineHeight * screenCoordScale;

	Vec2f trScaledFrameSize = textRenderer->GetScaledFrameSize();

	Rectanglef textArea;
	textArea.position.x = 0.0f;
	textArea.position.y = scaledLineHeight;
	textArea.size.x = trScaledFrameSize.x;
	textArea.size.y = trScaledFrameSize.y - scaledLineHeight;

	console->SetDrawArea(textArea);
	memoryStats->SetDrawArea(textArea);

	Rectanglef graphArea;
	graphArea.position.x = 0.0f;
	graphArea.position.y = pixelLineHeight;
	graphArea.size.x = frameSize.x;
	graphArea.size.y = frameSize.y - pixelLineHeight;
	graph->SetDrawArea(graphArea);

	culling->SetRenderer(renderer);
	culling->SetGuideTextPosition(Vec2f(0.0f, scaledLineHeight));
}

void Debug::Deinitialize()
{
	vectorRenderer->Deinitialize();
}

void Debug::Render(Scene* scene)
{
	bool vsync = false;

	if (window != nullptr)
	{
		DebugMode oldMode = this->mode;

		KeyboardInputView* keyboard = window->GetInputManager()->GetKeyboardInputView();

		// Update mode
		if (keyboard->GetKeyDown(Key::Escape))
		{
			this->mode = DebugMode::None;
		}
		else if (keyboard->GetKeyDown(Key::F1))
		{
			this->mode = DebugMode::Console;
		}
		else if (keyboard->GetKeyDown(Key::F2))
		{
			this->mode = DebugMode::FrameTime;
		}
		else if (keyboard->GetKeyDown(Key::F3))
		{
			this->mode = DebugMode::Culling;
		}
		else if (keyboard->GetKeyDown(Key::F4))
		{
			this->mode = DebugMode::MemoryStats;
		}

		// Check vsync switching

		vsync = window->GetSwapInterval() != 0;
		if (keyboard->GetKeyDown(Key::F8))
		{
			vsync = !vsync;
			window->SetSwapInterval(vsync ? 1 : 0);
		}

		// Check console switching

		if (oldMode != DebugMode::Console && this->mode == DebugMode::Console)
			console->RequestFocus();
		else if (oldMode == DebugMode::Console && this->mode != DebugMode::Console)
			console->ReleaseFocus();

		// Check culling camera controller switching

		if (oldMode != DebugMode::Culling && this->mode == DebugMode::Culling)
		{
			// Disable main camera controller
			culling->SetLockCullingCamera(true);
		}
		else if (oldMode == DebugMode::Culling && this->mode != DebugMode::Culling)
		{
			// Enable main camera controller
			culling->SetLockCullingCamera(false);
		}
	}

	char logChar = (mode == DebugMode::Console) ? '*' : ' ';
	char timeChar = (mode == DebugMode::FrameTime) ? '*' : ' ';
	char cullChar = (mode == DebugMode::Culling) ? '*' : ' ';
	char memChar = (mode == DebugMode::MemoryStats) ? '*' : ' ';

	char vsyncChar = vsync ? 'Y' : 'N';

	double now = Time::GetRunningTime();
	if (now > nextFrameRateUpdate)
	{
		currentFrameRate = 1.0 / graph->GetAverageOverLastSeconds(0.15);
		nextFrameRateUpdate = now + 0.15;
	}

	unsigned int errs = console->GetTotalErrorCount();
	unsigned int wrns = console->GetTotalWarningCount();

	const BitmapFont* font = textRenderer->GetFont();
	int glyphWidth = font->GetGlyphWidth();
	int lineHeight = font->GetLineHeight();

	if (errs > 0)
	{
		Rectanglef rect;
		rect.position.x = 1.0f;
		rect.position.y = 0.0f;
		rect.size.x = static_cast<float>(6 * glyphWidth) - 1.0f;
		rect.size.y = static_cast<float>(lineHeight);

		Color red(1.0f, 0.0f, 0.0f);
		vectorRenderer->DrawRectangleScreen(rect, red);
	}

	if (wrns > 0)
	{
		Rectanglef rect;
		rect.position.x = static_cast<float>(7 * glyphWidth) + 1.0f;
		rect.position.y = 0.0f;
		rect.size.x = static_cast<float>(6 * glyphWidth) - 1.0f;
		rect.size.y = static_cast<float>(lineHeight);

		Color yellow(1.0f, 1.0f, 0.0f);
		vectorRenderer->DrawRectangleScreen(rect, yellow);
	}

	// Draw debug mode guide
	char buffer[128];
	const char* format = "E: %-3u W: %-3u [F1]Console%c [F2]FrameTime%c [F3]Culling%c [F4]Memory%c [F8]Vsync: %c, %.1f fps";
	std::snprintf(buffer, sizeof(buffer), format, errs, wrns, logChar, timeChar, cullChar, memChar, vsyncChar, currentFrameRate);
	textRenderer->AddText(StringRef(buffer), Vec2f(0.0f, 0.0f));

	// Add frame time to debug graph
	graph->AddDataPoint(Time::GetDeltaTime());
	graph->Update();

	if (mode == DebugMode::Console)
		console->UpdateAndDraw();

	if (mode == DebugMode::FrameTime)
		graph->DrawToVectorRenderer();

	if (mode == DebugMode::Culling)
		culling->UpdateAndDraw(scene);

	if (mode == DebugMode::MemoryStats)
		memoryStats->UpdateAndDraw();

	vectorRenderer->Render(scene->GetActiveCamera());
	textRenderer->Render();
}
