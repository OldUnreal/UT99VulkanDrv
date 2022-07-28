#pragma once

#include "VulkanObjects.h"
#include "mat.h"

class UVulkanRenderDevice;

struct SceneVertex
{
	uint32_t Flags;
	vec3 Position;
	vec2 TexCoord;
	vec2 TexCoord2;
	vec2 TexCoord3;
	vec2 TexCoord4;
	vec4 Color;
	ivec4 TextureBinds;
};

struct ScenePushConstants
{
	mat4 objectToProjection;
};

struct PresentPushConstants
{
	float InvGamma;
	float padding1;
	float padding2;
	float padding3;
};

class ShaderManager
{
public:
	ShaderManager(UVulkanRenderDevice* renderer);
	~ShaderManager();

	struct SceneShaders
	{
		std::unique_ptr<VulkanShader> VertexShader;
		std::unique_ptr<VulkanShader> FragmentShader;
		std::unique_ptr<VulkanShader> FragmentShaderAlphaTest;
	} Scene, SceneBindless;

	std::unique_ptr<VulkanShader> ppVertexShader;
	std::unique_ptr<VulkanShader> ppFragmentPresentShader;

	static std::string LoadShaderCode(const std::string& filename, const std::string& defines = {});

private:
	UVulkanRenderDevice* renderer = nullptr;
};
