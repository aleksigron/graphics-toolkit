#include "Rendering/ScreenSpaceAmbientOcclusion.hpp"

#include <random>

#include "Math/Math.hpp"
#include "Math/Projection.hpp"

#include "Rendering/RenderDevice.hpp"
#include "Rendering/RenderViewport.hpp"

#include "Resources/ShaderManager.hpp"

#include "System/IncludeOpenGL.hpp"

ScreenSpaceAmbientOcclusion::ScreenSpaceAmbientOcclusion(
	Allocator* allocator,
	RenderDevice* renderDevice,
	ShaderManager* shaderManager) :
	allocator(allocator),
	renderDevice(renderDevice),
	shaderManager(shaderManager),
	kernel(allocator),
	kernelSize(16),
	occlusionPass(PassInfo{ }),
	blurPass(PassInfo{ }),
	noiseTextureId(0)
{
}

ScreenSpaceAmbientOcclusion::~ScreenSpaceAmbientOcclusion()
{
	Deinitialize();
}

void ScreenSpaceAmbientOcclusion::Initialize(Vec2i framebufferResolution)
{
	framebufferSize = framebufferResolution;

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;

	kernel.Resize(kernelSize);
	for (unsigned int i = 0; i < kernelSize;)
	{
		Vec3f vec(randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator));

		if (vec.SqrMagnitude() <= 1.0f)
		{
			float scale = i / static_cast<float>(kernelSize);
			scale = Math::Lerp(0.1f, 1.0f, scale * scale);
			kernel[i] = vec.GetNormalized() * scale;
			i += 1;
		}
	}

	// Noise texture

	const unsigned int noisePixels = NoiseTextureSize * NoiseTextureSize;

	Array<uint16_t> noise(allocator);
	noise.Resize(noisePixels * 2);
	for (unsigned int i = 0; i < noisePixels; i++)
	{
		noise[i * 2 + 0] = static_cast<uint16_t>(randomFloats(generator) * UINT16_MAX);
		noise[i * 2 + 1] = static_cast<uint16_t>(randomFloats(generator) * UINT16_MAX);
	}

	renderDevice->CreateTextures(1, &noiseTextureId);
	renderDevice->BindTexture(RenderTextureTarget::Texture2d, noiseTextureId);
	renderDevice->SetTextureMinFilter(RenderTextureTarget::Texture2d, RenderTextureFilterMode::Nearest);
	renderDevice->SetTextureMagFilter(RenderTextureTarget::Texture2d, RenderTextureFilterMode::Nearest);
	renderDevice->SetTextureWrapModeU(RenderTextureTarget::Texture2d, RenderTextureWrapMode::Repeat);
	renderDevice->SetTextureWrapModeV(RenderTextureTarget::Texture2d, RenderTextureWrapMode::Repeat);

	RenderCommandData::SetTextureStorage2D noiseStorage{
		RenderTextureTarget::Texture2d, 1, GL_RG16, NoiseTextureSize, NoiseTextureSize
	};
	renderDevice->SetTextureStorage2D(&noiseStorage);

	RenderCommandData::SetTextureSubImage2D noiseImage{
		RenderTextureTarget::Texture2d, 0, 0, 0, NoiseTextureSize, NoiseTextureSize, GL_RG, GL_UNSIGNED_SHORT, noise.GetData()
	};
	renderDevice->SetTextureSubImage2D(&noiseImage);

	// Per pass resources

	StringRef occlusionShaderPath("res/shaders/post_process/ssao_occlusion.shader.json");
	StringRef blurShaderPath("res/shaders/post_process/ssao_blur.shader.json");

	CreatePassResources(occlusionShaderPath, sizeof(OcclusionUniformBlock), occlusionPass);
	CreatePassResources(blurShaderPath, sizeof(BlurUniformBlock), blurPass);
}

void ScreenSpaceAmbientOcclusion::Deinitialize()
{
	DestroyPassResources(blurPass);
	DestroyPassResources(occlusionPass);

	if (noiseTextureId != 0)
	{
		renderDevice->DestroyTextures(1, &noiseTextureId);
		noiseTextureId = 0;
	}
}

void ScreenSpaceAmbientOcclusion::UpdateOcclusionUniformBuffer(const ProjectionParameters& projection) const
{
	float noiseSizef = static_cast<float>(NoiseTextureSize);

	Mat4x4f projectionMat = projection.GetProjectionMatrix();

	OcclusionUniformBlock uniforms;
	uniforms.projection = projectionMat;
	uniforms.halfNearPlane.y = std::tan(projection.height * 0.5f);
	uniforms.halfNearPlane.x = uniforms.halfNearPlane.y * projection.aspect;
	uniforms.noiseScale = Vec2f(framebufferSize.x / noiseSizef, framebufferSize.y / noiseSizef);
	uniforms.sampleRadius = 0.5f;
	uniforms.kernelSize = kernelSize;

	for (size_t i = 0; i < kernelSize; ++i)
		uniforms.kernel[i] = kernel[i];

	renderDevice->BindBuffer(RenderBufferTarget::UniformBuffer, occlusionPass.uniformBufferId);
	renderDevice->SetBufferSubData(RenderBufferTarget::UniformBuffer, 0, sizeof(OcclusionUniformBlock), &uniforms);
}

void ScreenSpaceAmbientOcclusion::UpdateBlurUniformBuffer() const
{
	BlurUniformBlock uniforms;
	uniforms.textureScale = Vec2f(1.0f / framebufferSize.x, 1.0f / framebufferSize.y);

	renderDevice->BindBuffer(RenderBufferTarget::UniformBuffer, blurPass.uniformBufferId);
	renderDevice->SetBufferSubData(RenderBufferTarget::UniformBuffer, 0, sizeof(BlurUniformBlock), &uniforms);
}

void ScreenSpaceAmbientOcclusion::CreatePassResources(StringRef shaderPath, size_t uniformSize, PassInfo& passInfoOut)
{
	// Shader

	passInfoOut.shaderId = shaderManager->GetIdByPath(shaderPath);

	// Uniform buffer

	renderDevice->CreateBuffers(1, &passInfoOut.uniformBufferId);
	renderDevice->BindBuffer(RenderBufferTarget::UniformBuffer, passInfoOut.uniformBufferId);
	renderDevice->SetBufferData(RenderBufferTarget::UniformBuffer, uniformSize, nullptr, RenderBufferUsage::DynamicDraw);

	// Framebuffer and backing image

	renderDevice->CreateFramebuffers(1, &passInfoOut.framebufferId);
	renderDevice->BindFramebuffer(RenderFramebufferTarget::Framebuffer, passInfoOut.framebufferId);

	renderDevice->CreateTextures(1, &passInfoOut.textureId);
	renderDevice->BindTexture(RenderTextureTarget::Texture2d, passInfoOut.textureId);
	renderDevice->SetTextureMinFilter(RenderTextureTarget::Texture2d, RenderTextureFilterMode::Nearest);
	renderDevice->SetTextureMagFilter(RenderTextureTarget::Texture2d, RenderTextureFilterMode::Nearest);
	renderDevice->SetTextureWrapModeU(RenderTextureTarget::Texture2d, RenderTextureWrapMode::ClampToEdge);
	renderDevice->SetTextureWrapModeV(RenderTextureTarget::Texture2d, RenderTextureWrapMode::ClampToEdge);

	RenderCommandData::SetTextureStorage2D frameStorage{
		RenderTextureTarget::Texture2d, 1, GL_R8, framebufferSize.x, framebufferSize.y
	};
	renderDevice->SetTextureStorage2D(&frameStorage);

	RenderCommandData::AttachFramebufferTexture2D attachTexture{
		RenderFramebufferTarget::Framebuffer, GL_COLOR_ATTACHMENT0, RenderTextureTarget::Texture2d, passInfoOut.textureId, 0
	};
	renderDevice->AttachFramebufferTexture2D(&attachTexture);
}

void ScreenSpaceAmbientOcclusion::DestroyPassResources(PassInfo& passInfoInOut)
{
	if (passInfoInOut.framebufferId != 0)
	{
		renderDevice->DestroyFramebuffers(1, &passInfoInOut.framebufferId);
		passInfoInOut.framebufferId = 0;
	}

	if (passInfoInOut.textureId != 0)
	{
		renderDevice->DestroyTextures(1, &passInfoInOut.textureId);
		passInfoInOut.textureId = 0;
	}
}
