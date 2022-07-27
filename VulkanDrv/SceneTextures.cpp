
#include "Precomp.h"
#include "SceneTextures.h"
#include "VulkanBuilders.h"
#include "UVulkanRenderDevice.h"

SceneTextures::SceneTextures(UVulkanRenderDevice* renderer, int width, int height, int multisample) : width(width), height(height)
{
	sceneSamples = GetBestSampleCount(renderer->Device, multisample);

	colorBuffer = ImageBuilder()
		.Size(width, height)
		.Samples(sceneSamples)
		.Format(VK_FORMAT_R16G16B16A16_SFLOAT)
		.Usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
		.DebugName("colorBuffer")
		.Create(renderer->Device);

	colorBufferView = ImageViewBuilder()
		.Image(colorBuffer.get(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT)
		.DebugName("colorBufferView")
		.Create(renderer->Device);

	depthBuffer = ImageBuilder()
		.Size(width, height)
		.Samples(sceneSamples)
		.Format(VK_FORMAT_D32_SFLOAT)
		.Usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		.DebugName("depthBuffer")
		.Create(renderer->Device);

	depthBufferView = ImageViewBuilder()
		.Image(depthBuffer.get(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT)
		.DebugName("depthBufferView")
		.Create(renderer->Device);

	ppImage = ImageBuilder()
		.Size(width, height)
		.Samples(VK_SAMPLE_COUNT_1_BIT)
		.Format(VK_FORMAT_R16G16B16A16_SFLOAT)
		.Usage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.DebugName("ppImage")
		.Create(renderer->Device);
	
	ppImageView = ImageViewBuilder()
		.Image(ppImage.get(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT)
		.DebugName("ppImageView")
		.Create(renderer->Device);

	PipelineBarrier()
		.AddImage(
			colorBuffer.get(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT)
		.AddImage(
			depthBuffer.get(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT)
		.Execute(
			renderer->Commands->GetDrawCommands(),
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
}

SceneTextures::~SceneTextures()
{
}

VkSampleCountFlagBits SceneTextures::GetBestSampleCount(VulkanDevice* device, int multisample)
{
	const auto& limits = device->PhysicalDevice.Properties.limits;
	int requestedSamples = clamp(multisample, 0, 64);
	VkSampleCountFlags deviceSampleCounts = limits.sampledImageColorSampleCounts & limits.sampledImageDepthSampleCounts & limits.sampledImageStencilSampleCounts;

	int samples = 1;
	VkSampleCountFlags bit = VK_SAMPLE_COUNT_1_BIT;
	VkSampleCountFlags best = bit;
	while (samples < requestedSamples)
	{
		if (deviceSampleCounts & bit)
		{
			best = bit;
		}
		samples <<= 1;
		bit <<= 1;
	}

	return (VkSampleCountFlagBits)best;
}
