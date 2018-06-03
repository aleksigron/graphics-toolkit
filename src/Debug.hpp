#pragma once

class DebugVectorRenderer;
class DebugTextRenderer;
class DebugGraph;
class DebugConsole;
class DebugLog;

class Window;

class Debug
{
private:
	DebugVectorRenderer* vectorRenderer;
	DebugTextRenderer* textRenderer;
	DebugGraph* graph;
	DebugConsole* console;
	DebugLog* log;

	Window* window;

	enum class DebugMode
	{
		None,
		Console,
		FrameTime
	}
	mode;

	void DrawFrameTimeStats();

public:
	Debug();
	~Debug();

	void SetWindow(Window* window);
	
	void Render();

	void CheckOpenGlErrors();

	DebugLog* GetLog() { return log; }
	DebugConsole* GetConsole() { return console; }
	DebugTextRenderer* GetTextRenderer() { return textRenderer; }
	DebugVectorRenderer* GetVectorRenderer() { return vectorRenderer; }
};
