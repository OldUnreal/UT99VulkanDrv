#pragma once

#include "ShaderManager.h"
#include "mat.h"
#include "vec.h"

class UVulkanRenderDevice;

class SceneTextures
{
public:
	SceneTextures(UVulkanRenderDevice* renderer, int width, int height, int multisample);
	~SceneTextures();

	// Current active multisample setting
	VkSampleCountFlagBits SceneSamples = VK_SAMPLE_COUNT_1_BIT;

	// Scene framebuffer color image
	std::unique_ptr<VulkanImage> ColorBuffer;
	std::unique_ptr<VulkanImageView> ColorBufferView;

	// Scene framebuffer depth buffer
	std::unique_ptr<VulkanImage> DepthBuffer;
	std::unique_ptr<VulkanImageView> DepthBufferView;

	// Post processing image buffers
	std::unique_ptr<VulkanImage> PPImage;
	std::unique_ptr<VulkanImageView> PPImageView;

	// Size of the scene framebuffer
	int width = 0;
	int height = 0;

private:
	static VkSampleCountFlagBits GetBestSampleCount(VulkanDevice* device, int multisample);
};
