#pragma once

enum class RenderPass
{
	OpaqueGeometry = 0, // Opaque + AlphaTest
	OpaqueLighting = 2,
	Skybox = 3,
	Transparent = 4 // TransparentMix + TransparentAdd + TransparentSub
};

enum class RenderCommandType
{
	Control = 0,
	Draw = 1
};

enum class RenderControlType
{
	BlendingEnable,
	BlendingDisable,

	Viewport, // Data: x, y, w, h
	DepthRange, // Data: near, far

	DepthTestEnable,
	DepthTestDisable,
	DepthTestFunction, // Data: function enum

	DepthWriteEnable,
	DepthWriteDisable,

	CullFaceEnable,
	CullFaceDisable,
	CullFaceFront,
	CullFaceBack,

	Clear, // Data: mask
	ClearColor, // Data: r, g, b, a
	ClearDepth, // Data: depth

	// Data: target, framebuffer ID
	BindFramebuffer,

	// Data: s_x0, s_y0, s_x1, s_y1, d_x0, d_y0, d_x1, d_y1, mask, filter
	BlitFramebuffer
};