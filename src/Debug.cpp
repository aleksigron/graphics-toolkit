#include "Debug.hpp"

#include <cstdio>

#include "DebugTextRenderer.hpp"
#include "DebugLogView.hpp"
#include "DebugLog.hpp"

#include "KeyboardInput.hpp"

Debug::Debug(KeyboardInput* keyboardInput) :
	keyboardInput(keyboardInput),
	mode(DebugMode::None)
{
	textRenderer = new DebugTextRenderer;
	logView = new DebugLogView(textRenderer);
	log = new DebugLog(logView);
}

Debug::~Debug()
{
	delete log;
	delete logView;
	delete textRenderer;
}

void Debug::Render()
{
	// Update mode
	if (keyboardInput->GetKeyDown(Key::N_1))
	{
		this->mode = DebugMode::None;
	}
	else if (keyboardInput->GetKeyDown(Key::N_2))
	{
		this->mode = DebugMode::LogView;
	}

	char modeNoneChar = (mode == DebugMode::None) ? '*' : ' ';
	char modeLogChar = (mode == DebugMode::LogView) ? '*' : ' ';

	// Draw debug mode guide
	char buffer[32];
	sprintf(buffer, "Debug mode: [1]None%c [2]Log%c", modeNoneChar, modeLogChar);
	textRenderer->AddText(StringRef(buffer), Vec2f(0.0f, 0.0f), true);

	// Draw mode content
	switch (this->mode)
	{
		case DebugMode::None:
			break;

		case DebugMode::LogView:
			logView->DrawToTextRenderer();
			break;
	}

	// Draw debug texts
	textRenderer->Render();
}
