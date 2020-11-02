
#include "Precomp.h"
#include "SceneBuffers.h"
#include "VulkanBuilders.h"

SceneBuffers::SceneBuffers(VulkanDevice *device, int width, int height, int multisample) : width(width), height(height)
{
	sceneSamples = getBestSampleCount(device, multisample);

	BufferBuilder bufbuild;
	bufbuild.setSize(sizeof(SceneUniforms));

	bufbuild.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	sceneUniforms = bufbuild.create(device);

	bufbuild.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	stagingSceneUniforms = bufbuild.create(device);

	createImage(colorBuffer, colorBufferView, device, width, height, sceneSamples, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	createImage(depthBuffer, depthBufferView, device, width, height, sceneSamples, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	colorBuffer->SetDebugName("SceneBuffers.colorBuffer");
	colorBufferView->SetDebugName("SceneBuffers.colorBufferView");

	depthBuffer->SetDebugName("SceneBuffers.depthBuffer");
	depthBufferView->SetDebugName("SceneBuffers.depthBufferView");
}

SceneBuffers::~SceneBuffers()
{
}

void SceneBuffers::createImage(std::unique_ptr<VulkanImage> &image, std::unique_ptr<VulkanImageView> &view, VulkanDevice *device, int width, int height, VkSampleCountFlagBits samples, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect)
{
	ImageBuilder imgbuild;
	imgbuild.setFormat(format);
	imgbuild.setUsage(usage);
	imgbuild.setSize(width, height);
	imgbuild.setSamples(samples);
	image = imgbuild.create(device);

	ImageViewBuilder viewbuild;
	viewbuild.setImage(image.get(), format, aspect);
	view = viewbuild.create(device);
}

VkSampleCountFlagBits SceneBuffers::getBestSampleCount(VulkanDevice* device, int multisample)
{
	const auto& limits = device->physicalDevice.properties.limits;
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
