#include "Renderer.hpp"

#include <cstring>
#include <cstdio>

#include "IncludeOpenGL.hpp"

#include "Debug.hpp"

#include "Engine.hpp"
#include "App.hpp"
#include "Window.hpp"

#include "ResourceManager.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Scene.hpp"

#include "Camera.hpp"
#include "Rectangle.hpp"
#include "ViewFrustum.hpp"
#include "BoundingBox.hpp"
#include "Intersect3D.hpp"
#include "BitPack.hpp"

#include "RenderPipeline.hpp"
#include "RenderCommandData.hpp"
#include "RenderCommandType.hpp"

#include "Sort.hpp"

Renderer::Renderer() :
	overrideRenderCamera(nullptr),
	overrideCullingCamera(nullptr),
	lightViewportIndex(0),
	fullscreenViewportIndex(0)
{
	lightingData = LightingData{};
	gbuffer = RendererFramebuffer{};
	data = InstanceData{};
	data.count = 1; // Reserve index 0 as RenderObjectId::Null value

	this->Reallocate(512);
}

Renderer::~Renderer()
{
	this->Deinitialize();

	operator delete[](data.buffer);
}

void Renderer::Initialize(Window* window)
{
	{
		// Create geometry framebuffer and textures

		Vec2f sf = window->GetFrameBufferSize();
		Vec2i s(static_cast<float>(sf.x), static_cast<float>(sf.y));
		gbuffer.resolution = s;

		// Create and bind framebuffer

		glGenFramebuffers(1, &gbuffer.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.framebuffer);

		gbuffer.textureCount = 3;
		glGenTextures(gbuffer.textureCount, gbuffer.textures);
		unsigned int colAtt[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

		// Normal buffer
		unsigned int norTexture = gbuffer.textures[NormalTextureIdx];
		glBindTexture(GL_TEXTURE_2D, norTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, s.x, s.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, colAtt[0], GL_TEXTURE_2D, norTexture, 0);

		// Albedo color + specular buffer
		unsigned int asTexture = gbuffer.textures[AlbedoSpecTextureIdx];
		glBindTexture(GL_TEXTURE_2D, asTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s.x, s.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, colAtt[1], GL_TEXTURE_2D, asTexture, 0);

		// Which color attachments we'll use for rendering
		glDrawBuffers(2, colAtt);

		// Create and attach depth buffer
		unsigned int depthTexture = gbuffer.textures[DepthTextureIdx];
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, s.x, s.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		// Finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	{
		Vec2i s(1024, 1024);
		shadowBuffer.resolution = s;

		// Create and bind framebuffer

		glGenFramebuffers(1, &shadowBuffer.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.framebuffer);

		// We aren't rendering to any color attachments
		glDrawBuffer(GL_NONE);

		// Create texture
		shadowBuffer.textureCount = 1;
		glGenTextures(shadowBuffer.textureCount, &shadowBuffer.textures[0]);

		// Create and attach depth buffer
		unsigned int depthTexture = shadowBuffer.textures[0];
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, s.x, s.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		// Finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	{
		// Create screen filling quad

		static const Vertex3f vertexData[] = {
			Vertex3f{ Vec3f(-1.0f, -1.0f, 0.0f) },
			Vertex3f{ Vec3f(1.0f, -1.0f, 0.0f) },
			Vertex3f{ Vec3f(-1.0f, 1.0f, 0.0f) },
			Vertex3f{ Vec3f(1.0f, 1.0f, 0.0f) }
		};

		static const unsigned short indexData[] = { 0, 1, 2, 1, 3, 2 };

		MeshManager* meshManager = Engine::GetInstance()->GetMeshManager();

		lightingData.dirMesh = meshManager->CreateMesh();

		IndexedVertexData<Vertex3f, unsigned short> data;
		data.primitiveMode = MeshPrimitiveMode::Triangles;
		data.vertData = vertexData;
		data.vertCount = sizeof(vertexData) / sizeof(Vertex3f);
		data.idxData = indexData;
		data.idxCount = sizeof(indexData) / sizeof(unsigned short);

		meshManager->Upload_3f(lightingData.dirMesh, data);

		Vec3f inverseDir(0.282166332f, 0.846498966f, 0.451466143f);

		lightingData.lightPos = inverseDir * 10.0f;
		lightingData.lightDir = -inverseDir;
		lightingData.lightCol = Vec3f(1.0f, 1.0f, 1.0f);

		MaterialManager* materialManager = Engine::GetInstance()->GetMaterialManager();
		const char* const matPath = "res/materials/depth.material.json";
		lightingData.shadowMaterial = materialManager->GetIdByPath(StringRef(matPath));
	}

	{
		ResourceManager* resManager = Engine::GetInstance()->GetResourceManager();

		static const char* const path = "res/shaders/lighting.shader.json";

		Shader* shader = resManager->GetShader(path);
		lightingData.dirShaderHash = shader->nameHash;
	}
}

void Renderer::Deinitialize()
{
	MeshManager* meshManager = Engine::GetInstance()->GetMeshManager();

	if (lightingData.dirMesh.IsValid())
		meshManager->RemoveMesh(lightingData.dirMesh);

	if (shadowBuffer.framebuffer != 0)
	{
		glDeleteTextures(shadowBuffer.textureCount, shadowBuffer.textures);
		glDeleteFramebuffers(1, &shadowBuffer.framebuffer);
	}

	if (gbuffer.framebuffer != 0)
	{
		glDeleteTextures(gbuffer.textureCount, gbuffer.textures);
		glDeleteFramebuffers(1, &gbuffer.framebuffer);
	}
}

Camera* Renderer::GetRenderCamera(Scene* scene)
{
	return overrideRenderCamera != nullptr ? overrideRenderCamera : scene->GetActiveCamera();
}

Camera* Renderer::GetCullingCamera(Scene* scene)
{
	return overrideCullingCamera != nullptr ? overrideCullingCamera : scene->GetActiveCamera();
}

void Renderer::Render(Scene* scene)
{
	Engine* engine = Engine::GetInstance();
	MeshManager* meshManager = engine->GetMeshManager();
	MaterialManager* materialManager = engine->GetMaterialManager();
	ResourceManager* res = engine->GetResourceManager();

	Camera* renderCamera = this->GetRenderCamera(scene);
	SceneObjectId renderCameraObject = scene->Lookup(renderCamera->GetEntity());
	Mat4x4f renderCameraTransform = scene->GetWorldTransform(renderCameraObject);
	Vec3f cameraPosition = (renderCameraTransform * Vec4f(0.0f, 0.0f, 0.0f, 1.0f)).xyz();

	if (scene->skybox.IsInitialized()) // Update skybox transform
		scene->skybox.UpdateTransform(cameraPosition);

	// Retrieve updated transforms
	scene->NotifyUpdatedTransforms(this);

	CreateDrawCalls(scene);

	uint64_t* itr = commandList.commands.GetData();
	uint64_t* end = itr + commandList.commands.GetCount();
	for (; itr != end; ++itr)
	{
		uint64_t command = *itr;

		// If command is not control command, draw object
		if (ParseControlCommand(command) == false)
		{
			RenderPass pass = static_cast<RenderPass>(renderOrder.viewportPass.GetValue(command));

			if (pass != RenderPass::OpaqueLighting)
			{
				unsigned int vpIdx = renderOrder.viewportIndex.GetValue(command);
				unsigned int objIdx = renderOrder.renderObject.GetValue(command);
				unsigned int mat = renderOrder.materialId.GetValue(command);
				MaterialId matId = MaterialId{mat};

				const MaterialUniformData& mu = materialManager->GetUniformData(matId);

				unsigned int shaderId = materialManager->GetShaderId(matId);
				Shader* shader = res->GetShader(shaderId);

				glUseProgram(shader->driverId);

				unsigned int usedTextures = 0;

				// Bind each material uniform with a value
				for (unsigned uIndex = 0; uIndex < mu.count; ++uIndex)
				{
					const MaterialUniform& u = mu.uniforms[uIndex];

					unsigned char* d = mu.data + u.dataOffset;

					switch (u.type)
					{
						case ShaderUniformType::Mat4x4:
							glUniformMatrix4fv(u.location, 1, GL_FALSE, reinterpret_cast<float*>(d));
							break;

						case ShaderUniformType::Vec4:
							glUniform4fv(u.location, 1, reinterpret_cast<float*>(d));
							break;

						case ShaderUniformType::Vec3:
							glUniform3fv(u.location, 1, reinterpret_cast<float*>(d));
							break;

						case ShaderUniformType::Vec2:
							glUniform2fv(u.location, 1, reinterpret_cast<float*>(d));
							break;

						case ShaderUniformType::Float:
							glUniform1f(u.location, *reinterpret_cast<float*>(d));
							break;

						case ShaderUniformType::Int:
							glUniform1i(u.location, *reinterpret_cast<int*>(d));
							break;

						case ShaderUniformType::Tex2D:
						case ShaderUniformType::TexCube:
						{
							uint32_t textureHash = *reinterpret_cast<uint32_t*>(d);
							Texture* texture = res->GetTexture(textureHash);

							glActiveTexture(GL_TEXTURE0 + usedTextures);
							glBindTexture(texture->targetType, texture->driverId);
							glUniform1i(u.location, usedTextures);

							++usedTextures;
						}
							break;
					}
				}

				const RendererViewportTransform& vptr = viewportTransforms[vpIdx];
				const Mat4x4f& model = data.transform[objIdx];

				if (shader->uniformMatMVP >= 0)
				{
					Mat4x4f mvp = vptr.viewProjection * model;
					glUniformMatrix4fv(shader->uniformMatMVP, 1, GL_FALSE, mvp.ValuePointer());
				}

				if (shader->uniformMatMV >= 0)
				{
					Mat4x4f mv = vptr.view * model;
					glUniformMatrix4fv(shader->uniformMatMV, 1, GL_FALSE, mv.ValuePointer());
				}

				if (shader->uniformMatVP >= 0)
				{
					glUniformMatrix4fv(shader->uniformMatVP, 1, GL_FALSE, vptr.viewProjection.ValuePointer());
				}

				if (shader->uniformMatM >= 0)
				{
					glUniformMatrix4fv(shader->uniformMatM, 1, GL_FALSE, model.ValuePointer());
				}

				if (shader->uniformMatV >= 0)
				{
					glUniformMatrix4fv(shader->uniformMatV, 1, GL_FALSE, vptr.view.ValuePointer());
				}

				if (shader->uniformMatP >= 0)
				{
					glUniformMatrix4fv(shader->uniformMatP, 1, GL_FALSE, vptr.projection.ValuePointer());
				}

				MeshId mesh = data.mesh[objIdx];
				MeshDrawData* draw = meshManager->GetDrawData(mesh);
				
				glBindVertexArray(draw->vertexArrayObject);

				glDrawElements(draw->primitiveMode, draw->indexCount, draw->indexElementType, nullptr);
			}
			else // Pass is OpaqueLighting
			{
				ResourceManager* resManager = Engine::GetInstance()->GetResourceManager();
				Shader* shader = resManager->GetShader(lightingData.dirShaderHash);

				const RendererViewportTransform& lvptr = viewportTransforms[lightViewportIndex];
				const RendererViewportTransform& fsvptr = viewportTransforms[fullscreenViewportIndex];

				Vec2f halfNearPlane;
				halfNearPlane.y = std::tan(renderCamera->parameters.height * 0.5f);
				halfNearPlane.x = halfNearPlane.y * renderCamera->parameters.aspect;

				const unsigned int shaderId = shader->driverId;

				int normLoc = glGetUniformLocation(shaderId, "g_norm");
				int albSpecLoc = glGetUniformLocation(shaderId, "g_alb_spec");
				int depthLoc = glGetUniformLocation(shaderId, "g_depth");
				
				int shadowMatLoc = glGetUniformLocation(shaderId, "shadow_mat");
				int shadowDepthLoc = glGetUniformLocation(shaderId, "shadow_depth");

				int halfNearPlaneLoc = glGetUniformLocation(shaderId, "half_near_plane");
				int persMatLoc = glGetUniformLocation(shaderId, "pers_mat");

				int invLightDirLoc = glGetUniformLocation(shaderId, "light.inverse_dir");
				int lightColLoc = glGetUniformLocation(shaderId, "light.color");

				glUseProgram(shaderId);

				glUniform1i(normLoc, 0);
				glUniform1i(albSpecLoc, 1);
				glUniform1i(depthLoc, 2);

				glUniform1i(shadowDepthLoc, 3);

				Vec3f wInvLightDir = -lightingData.lightDir;
				Vec3f viewDir = (fsvptr.view * Vec4f(wInvLightDir, 0.0f)).xyz();

				// Set light properties
				glUniform3f(invLightDirLoc, viewDir.x, viewDir.y, viewDir.z);

				Vec3f col = lightingData.lightCol;
				glUniform3f(lightColLoc, col.x, col.y, col.z);

				glUniform2f(halfNearPlaneLoc, halfNearPlane.x, halfNearPlane.y);

				// Set the perspective matrix
				glUniformMatrix4fv(persMatLoc, 1, GL_FALSE, fsvptr.projection.ValuePointer());

				Mat4x4f bias;
				bias[0] = 0.5; bias[1] = 0.0; bias[2] = 0.0; bias[3] = 0.0;
				bias[4] = 0.0; bias[5] = 0.5; bias[6] = 0.0; bias[7] = 0.0;
				bias[8] = 0.0; bias[9] = 0.0; bias[10] = 0.5; bias[11] = 0.0;
				bias[12] = 0.5; bias[13] = 0.5; bias[14] = 0.5; bias[15] = 1.0;

				Mat4x4f viewToLight = lvptr.viewProjection * renderCameraTransform;
				Mat4x4f shadowMat = bias * viewToLight;
				glUniformMatrix4fv(shadowMatLoc, 1, GL_FALSE, shadowMat.ValuePointer());

				// Bind textures

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, gbuffer.textures[NormalTextureIdx]);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, gbuffer.textures[AlbedoSpecTextureIdx]);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, gbuffer.textures[DepthTextureIdx]);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, shadowBuffer.textures[0]);

				// Draw fullscreen quad

				MeshDrawData* draw = meshManager->GetDrawData(lightingData.dirMesh);
				glBindVertexArray(draw->vertexArrayObject);

				glDrawElements(draw->primitiveMode, draw->indexCount, draw->indexElementType, nullptr);
			}
		}
	}

	glBindVertexArray(0);

	commandList.Clear();
}

bool Renderer::ParseControlCommand(uint64_t orderKey)
{
	if (renderOrder.command.GetValue(orderKey) == static_cast<uint64_t>(RenderCommandType::Draw))
		return false;

	uint64_t commandTypeInt = renderOrder.commandType.GetValue(orderKey);
	RenderControlType control = static_cast<RenderControlType>(commandTypeInt);

	switch (control)
	{
		case RenderControlType::BlendingEnable:
			RenderPipeline::BlendingEnable();
			break;

		case RenderControlType::BlendingDisable:
			RenderPipeline::BlendingDisable();
			break;

		case RenderControlType::Viewport:
		{
			unsigned int offset = renderOrder.commandData.GetValue(orderKey);
			uint8_t* data = commandList.commandData.GetData() + offset;
			auto* viewport = reinterpret_cast<RenderCommandData::ViewportData*>(data);
			RenderPipeline::Viewport(viewport);
		}
			break;

		case RenderControlType::DepthRange:
		{
			unsigned int offset = renderOrder.commandData.GetValue(orderKey);
			uint8_t* data = commandList.commandData.GetData() + offset;
			auto* depthRange = reinterpret_cast<RenderCommandData::DepthRangeData*>(data);
			RenderPipeline::DepthRange(depthRange);
		}
			break;

		case RenderControlType::DepthTestEnable:
			RenderPipeline::DepthTestEnable();
			break;

		case RenderControlType::DepthTestDisable:
			RenderPipeline::DepthTestDisable();
			break;

		case RenderControlType::DepthTestFunction:
		{
			unsigned int fn = renderOrder.commandData.GetValue(orderKey);
			RenderPipeline::DepthTestFunction(fn);
		}
			break;

		case RenderControlType::DepthWriteEnable:
			RenderPipeline::DepthWriteEnable();
			break;

		case RenderControlType::DepthWriteDisable:
			RenderPipeline::DepthWriteDisable();
			break;

		case RenderControlType::CullFaceEnable:
			RenderPipeline::CullFaceEnable();
			break;

		case RenderControlType::CullFaceDisable:
			RenderPipeline::CullFaceDisable();
			break;

		case RenderControlType::CullFaceFront:
			RenderPipeline::CullFaceFront();
			break;

		case RenderControlType::CullFaceBack:
			RenderPipeline::CullFaceBack();
			break;

		case RenderControlType::Clear:
		{
			unsigned int mask = renderOrder.commandData.GetValue(orderKey);
			RenderPipeline::Clear(mask);
		}
			break;

		case RenderControlType::ClearColor:
		{
			unsigned int offset = renderOrder.commandData.GetValue(orderKey);
			uint8_t* data = commandList.commandData.GetData() + offset;
			auto* color = reinterpret_cast<RenderCommandData::ClearColorData*>(data);
			RenderPipeline::ClearColor(color);
		}
			break;

		case RenderControlType::ClearDepth:
		{
			unsigned int intDepth = renderOrder.commandData.GetValue(orderKey);
			float depth = *reinterpret_cast<float*>(&intDepth);
			RenderPipeline::ClearDepth(depth);
		}
			break;

		case RenderControlType::BindFramebuffer:
		{
			unsigned int offset = renderOrder.commandData.GetValue(orderKey);
			uint8_t* data = commandList.commandData.GetData() + offset;
			auto* bind = reinterpret_cast<RenderCommandData::BindFramebufferData*>(data);
			RenderPipeline::BindFramebuffer(bind);
		}
			break;
			
		case RenderControlType::BlitFramebuffer:
		{
			unsigned int offset = renderOrder.commandData.GetValue(orderKey);
			uint8_t* data = commandList.commandData.GetData() + offset;
			auto* blit = reinterpret_cast<RenderCommandData::BlitFramebufferData*>(data);
			RenderPipeline::BlitFramebuffer(blit);
		}
			break;
	}

	return true;
}

float CalculateDepth(const Vec3f& objPos, const Vec3f& eyePos, const Vec3f& eyeForward, const ProjectionParameters& params)
{
	return (Vec3f::Dot(objPos - eyePos, eyeForward) - params.near) / (params.far - params.near);
}

void Renderer::CreateDrawCalls(Scene* scene)
{
	using ctrl = RenderControlType;

	unsigned int viewportCount = 2; // 1 fullscreen viewport + 1 shadow casting light
	unsigned int lvp = 0;
	unsigned int fsvp = 1;
	
	fullscreenViewportIndex = fsvp;

	unsigned int colorAndDepthMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

	RenderPass g_pass = RenderPass::OpaqueGeometry;
	RenderPass l_pass = RenderPass::OpaqueLighting;
	RenderPass s_pass = RenderPass::Skybox;
	RenderPass t_pass = RenderPass::Transparent;

	// Before light shadow viewport

	// Set depth test on
	commandList.AddControl(lvp, g_pass, 0, ctrl::DepthTestEnable);

	// Set depth test function
	commandList.AddControl(lvp, g_pass, 1, ctrl::DepthTestFunction, GL_LESS);

	// Enable depth writing
	commandList.AddControl(lvp, g_pass, 2, ctrl::DepthWriteEnable);

	// Set face culling on
	commandList.AddControl(lvp, g_pass, 3, ctrl::CullFaceEnable);

	// Set face culling to cull back faces
	commandList.AddControl(lvp, g_pass, 4, ctrl::CullFaceBack);

	{
		// Set clear depth
		float depth = 1.0f;
		unsigned int* intDepthPtr = reinterpret_cast<unsigned int*>(&depth);

		commandList.AddControl(lvp, g_pass, 5, ctrl::ClearDepth, *intDepthPtr);
	}

	// Disable blending
	commandList.AddControl(lvp, g_pass, 6, ctrl::BlendingDisable);

	{
		// Set viewport size
		RenderCommandData::ViewportData data;
		data.x = 0;
		data.y = 0;
		data.w = shadowBuffer.resolution.x;
		data.h = shadowBuffer.resolution.y;

		commandList.AddControl(lvp, g_pass, 7, ctrl::Viewport, sizeof(data), &data);
	}

	{
		// Bind geometry framebuffer
		RenderCommandData::BindFramebufferData data;
		data.target = GL_FRAMEBUFFER;
		data.framebuffer = shadowBuffer.framebuffer;

		commandList.AddControl(lvp, g_pass, 8, ctrl::BindFramebuffer, sizeof(data), &data);
	}

	// Clear currently bound GL_FRAMEBUFFER
	commandList.AddControl(lvp, g_pass, 9, ctrl::Clear, GL_DEPTH_BUFFER_BIT);

	// Before fullscreen viewport

	// PASS: OPAQUE GEOMETRY

	{
		// Set clear color
		RenderCommandData::ClearColorData data;
		data.r = 0.0f;
		data.g = 0.0f;
		data.b = 0.0f;
		data.a = 0.0f;

		commandList.AddControl(fsvp, g_pass, 0, ctrl::ClearColor, sizeof(data), &data);
	}

	{
		// Set viewport size
		RenderCommandData::ViewportData data;
		data.x = 0;
		data.y = 0;
		data.w = gbuffer.resolution.x;
		data.h = gbuffer.resolution.y;

		commandList.AddControl(fsvp, g_pass, 1, ctrl::Viewport, sizeof(data), &data);
	}

	{
		// Bind geometry framebuffer
		RenderCommandData::BindFramebufferData data;
		data.target = GL_FRAMEBUFFER;
		data.framebuffer = gbuffer.framebuffer;

		commandList.AddControl(fsvp, g_pass, 2, ctrl::BindFramebuffer, sizeof(data), &data);
	}

	// Clear currently bound GL_FRAMEBUFFER
	commandList.AddControl(fsvp, g_pass, 3, ctrl::Clear, colorAndDepthMask);

	// PASS: OPAQUE LIGHTING

	{
		// Bind default framebuffer
		RenderCommandData::BindFramebufferData data;
		data.target = GL_FRAMEBUFFER;
		data.framebuffer = 0;

		commandList.AddControl(fsvp, l_pass, 0, ctrl::BindFramebuffer, sizeof(data), &data);
	}

	// Clear currently bound GL_FRAMEBUFFER
	commandList.AddControl(fsvp, l_pass, 1, ctrl::Clear, colorAndDepthMask);

	commandList.AddControl(fsvp, l_pass, 2, ctrl::DepthTestFunction, GL_ALWAYS);

	// Do lighting
	commandList.AddDraw(fsvp, l_pass, 0.0f, MaterialId{}, 0);

	// PASS: SKYBOX

	commandList.AddControl(fsvp, s_pass, 0, ctrl::DepthTestFunction, GL_EQUAL);

	// Disable depth writing
	commandList.AddControl(fsvp, s_pass, 1, ctrl::DepthWriteDisable);

	// PASS: TRANSPARENT

	// Before transparent objects

	commandList.AddControl(fsvp, t_pass, 0, ctrl::DepthTestFunction, GL_LESS);

	// Enable blending
	commandList.AddControl(fsvp, t_pass, 1, ctrl::BlendingEnable);

	// Create draw commands for render objects in scene

	Camera* renderCamera = this->GetRenderCamera(scene);
	SceneObjectId cameraObject = scene->Lookup(renderCamera->GetEntity());
	Mat4x4f cameraTransform = scene->GetWorldTransform(cameraObject);

	Camera* cullingCamera = this->GetCullingCamera(scene);
	SceneObjectId cullingCameraObject = scene->Lookup(cullingCamera->GetEntity());
	Mat4x4f cullingCameraTransform = scene->GetWorldTransform(cullingCameraObject);

	Vec3f cameraPos = (cameraTransform * Vec4f(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
	Vec3f cameraForward = (cameraTransform * Vec4f(0.0f, 0.0f, -1.0f, 0.0f)).xyz();

	ProjectionParameters lightParameters;
	lightParameters.aspect = 1.0f;
	lightParameters.height = 24.0f;
	lightParameters.near = 0.0f;
	lightParameters.far = 20.0f;
	lightParameters.projection = ProjectionType::Orthographic;

	Vec3f lightPos = lightingData.lightPos;
	Vec3f lightDir = lightingData.lightDir;
	Vec3f lightTarget = lightPos + lightDir;
	Vec3f up(0.0f, 1.0f, 0.0f);
	Mat4x4f lightTransform = Mat4x4f::LookAt(lightPos, lightTarget, up);
	MaterialId shadowMaterial = lightingData.shadowMaterial;

	RendererViewportTransform& lvptr = viewportTransforms[lvp];
	lvptr.view = Camera::GetViewMatrix(lightTransform);
	lvptr.projection = lightParameters.GetProjectionMatrix();
	lvptr.viewProjection = lvptr.projection * lvptr.view;

	RendererViewportTransform& fsvptr = viewportTransforms[fsvp];
	fsvptr.view = Camera::GetViewMatrix(cameraTransform);
	fsvptr.projection = renderCamera->parameters.GetProjectionMatrix();
	fsvptr.viewProjection = fsvptr.projection * fsvptr.view;

	FrustumPlanes frustum[MaxViewportCount];
	frustum[lvp].Update(lightParameters, lightTransform);
	frustum[fsvp].Update(cullingCamera->parameters, cullingCameraTransform);

	unsigned int visRequired = BitPack::CalculateRequired(data.count);
	objectVisibility.Resize(visRequired * viewportCount);

	BitPack* vis[MaxViewportCount];
	vis[lvp] = objectVisibility.GetData();
	vis[fsvp] = vis[lvp] + visRequired;

	Intersect::FrustumAABB(frustum[lvp], data.count, data.bounds, vis[lvp]);
	Intersect::FrustumAABB(frustum[fsvp], data.count, data.bounds, vis[fsvp]);

	for (unsigned int i = 1; i < data.count; ++i)
	{
		Vec3f objPos = (data.transform[i] * Vec4f(0.0f, 0.0f, 0.0f, 1.0f)).xyz();

		if (BitPack::Get(vis[lvp], i))
		{
			float depth = CalculateDepth(objPos, lightPos, lightDir, lightParameters);
			commandList.AddDraw(lvp, RenderPass::OpaqueGeometry, depth, shadowMaterial, i);
		}

		// Object is in potentially visible set for the particular viewport
		if (BitPack::Get(vis[fsvp], i))
		{
			const RenderOrderData& o = data.order[i];

			float depth = CalculateDepth(objPos, cameraPos, cameraForward, renderCamera->parameters);

			RenderPass pass = static_cast<RenderPass>(o.transparency);
			commandList.AddDraw(fsvp, pass, depth, o.material, i);
		}
	}

	commandList.Sort();
}

void Renderer::Reallocate(unsigned int required)
{
	if (required <= data.allocated)
		return;

	required = Math::UpperPowerOfTwo(required);

	// Reserve same amount in entity map
	entityMap.Reserve(required);

	InstanceData newData;
	unsigned int bytes = required * (sizeof(Entity) + sizeof(MeshId) + sizeof(RenderOrderData) +
		sizeof(BoundingBox) + sizeof(Mat4x4f));

	newData.buffer = operator new[](bytes);
	newData.count = data.count;
	newData.allocated = required;

	newData.entity = static_cast<Entity*>(newData.buffer);
	newData.mesh = reinterpret_cast<MeshId*>(newData.entity + required);
	newData.order = reinterpret_cast<RenderOrderData*>(newData.mesh + required);
	newData.bounds = reinterpret_cast<BoundingBox*>(newData.order + required);
	newData.transform = reinterpret_cast<Mat4x4f*>(newData.bounds + required);

	if (data.buffer != nullptr)
	{
		std::memcpy(newData.entity, data.entity, data.count * sizeof(Entity));
		std::memcpy(newData.mesh, data.mesh, data.count * sizeof(MeshId));
		std::memcpy(newData.order, data.order, data.count * sizeof(RenderOrderData));
		std::memcpy(newData.bounds, data.bounds, data.count * sizeof(BoundingBox));
		std::memcpy(newData.transform, data.transform, data.count * sizeof(Mat4x4f));

		operator delete[](data.buffer);
	}

	data = newData;
}

RenderObjectId Renderer::AddRenderObject(Entity entity)
{
	RenderObjectId id;
	this->AddRenderObject(1, &entity, &id);
	return id;
}

void Renderer::AddRenderObject(unsigned int count, Entity* entities, RenderObjectId* renderObjectIdsOut)
{
	if (data.count + count > data.allocated)
		this->Reallocate(data.count + count);

	for (unsigned int i = 0; i < count; ++i)
	{
		unsigned int id = data.count + i;

		Entity e = entities[i];

		auto mapPair = entityMap.Insert(e.id);
		mapPair->value.i = id;

		data.entity[id] = e;

		renderObjectIdsOut[i].i = id;
	}

	data.count += count;
}

void Renderer::NotifyUpdatedTransforms(unsigned int count, Entity* entities, Mat4x4f* transforms)
{
	MeshManager* meshManager = Engine::GetInstance()->GetMeshManager();

	for (unsigned int entityIdx = 0; entityIdx < count; ++entityIdx)
	{
		Entity entity = entities[entityIdx];
		RenderObjectId obj = this->Lookup(entity);

		if (obj.IsNull() == false)
		{
			unsigned int dataIdx = obj.i;

			// Recalculate bounding box
			BoundingBox* bounds = meshManager->GetBoundingBox(data.mesh[dataIdx]);
			data.bounds[dataIdx] = bounds->Transform(transforms[entityIdx]);

			// Set world transform
			data.transform[dataIdx] = transforms[entityIdx];
		}
	}
}
