#pragma once

#include "StringRef.hpp"
#include "Rectangle.hpp"
#include "Vec2.hpp"

class BitmapFont;
struct Mesh;

class DebugTextRenderer
{
private:
	struct RenderData
	{
		StringRef string;
		Rectangle area;
	};

	BitmapFont* font;

	char* stringData;
	unsigned int stringDataUsed;
	unsigned int stringDataAllocated;

	RenderData* renderData;
	unsigned int renderDataCount;
	unsigned int renderDataAllocated;

	Vec2f frameSize;

	void CreateAndUploadData(Mesh& mesh);

public:
	DebugTextRenderer();
	~DebugTextRenderer();

	bool LoadBitmapFont(const char* filePath);
	bool HasValidFont() const { return font != nullptr; }

	void SetFrameSize(const Vec2f& size) { frameSize = size; }

	/**
	 * Add a text to be rendered this frame at a specified position.
	 * String data will be copied, there's no need to keep it around.
	 */
	void AddText(StringRef str, Vec2f position);

	void Render();
};
