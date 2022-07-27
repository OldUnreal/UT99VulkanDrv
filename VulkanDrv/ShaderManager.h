#pragma once

#include "VulkanObjects.h"
#include "mat.h"

class UVulkanRenderDevice;

struct SceneVertex
{
	uint32_t flags;
	float x, y, z;
	float u, v;
	float u2, v2;
	float u3, v3;
	float u4, v4;
	float r, g, b, a;
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

	std::unique_ptr<VulkanShader> vertexShader;
	std::unique_ptr<VulkanShader> fragmentShader;
	std::unique_ptr<VulkanShader> fragmentShaderAlphaTest;

	std::unique_ptr<VulkanShader> ppVertexShader;
	std::unique_ptr<VulkanShader> ppFragmentPresentShader;

	static std::string LoadShaderCode(const std::string& filename, const std::string& defines = {});

private:
	UVulkanRenderDevice* renderer = nullptr;
};
