
#include "Precomp.h"
#include "Renderer.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanObjects.h"
#include "VulkanBuilders.h"
#include "VulkanPostprocess.h"
#include "VulkanTexture.h"
#include "SceneBuffers.h"
#include "SceneRenderPass.h"
#include "SceneSamplers.h"
#include "FileResource.h"

static bool shaderbuilderinited = false;

Renderer::Renderer(HWND windowHandle, bool vsync, int vk_device, bool vk_debug, std::function<void(const char* typestr, const std::string& msg)> printLogCallback) : WindowHandle(windowHandle)
{
	if (!shaderbuilderinited)
	{
		ShaderBuilder::init();
		shaderbuilderinited = true;
	}

	Device = new VulkanDevice(WindowHandle, vk_device, vk_debug, printLogCallback);
	SwapChain = new VulkanSwapChain(Device, vsync);
	ImageAvailableSemaphore = new VulkanSemaphore(Device);
	RenderFinishedSemaphore = new VulkanSemaphore(Device);
	RenderFinishedFence = new VulkanFence(Device);
	TransferSemaphore = new VulkanSemaphore(Device);
	CommandPool = new VulkanCommandPool(Device, Device->graphicsFamily);
	FrameDeleteList = new DeleteList();
	SceneSamplers = new ::SceneSamplers(Device);
	CreateSceneVertexBuffer();
	CreateSceneDescriptorSetLayout();
	CreateScenePipelineLayout();
	CreateNullTexture();
	PostprocessModel = new ::Postprocess();
	Postprocess = new VulkanPostprocess(this);
}

Renderer::~Renderer()
{
	if (Device) vkDeviceWaitIdle(Device->device);
	DeleteFrameObjects();
	if (SceneVertices) { SceneVertexBuffer->Unmap(); SceneVertices = nullptr; }
	delete SceneVertexBuffer; SceneVertexBuffer = nullptr; SceneVertices = nullptr; SceneVertexPos = 0;
	delete NullTextureView; NullTextureView = nullptr;
	delete NullTexture; NullTexture = nullptr;
	delete SceneSamplers; SceneSamplers = nullptr;
	delete Postprocess; Postprocess = nullptr;
	delete PostprocessModel; PostprocessModel = nullptr;
	delete SceneRenderPass; SceneRenderPass = nullptr;
	delete SceneBuffers; SceneBuffers = nullptr;
	delete SceneDescriptorSetLayout; SceneDescriptorSetLayout = nullptr;
	delete ScenePipelineLayout; ScenePipelineLayout = nullptr;
	ClearTextureCache();
	delete ImageAvailableSemaphore; ImageAvailableSemaphore = nullptr;
	delete RenderFinishedSemaphore; RenderFinishedSemaphore = nullptr;
	delete RenderFinishedFence; RenderFinishedFence = nullptr;
	delete TransferSemaphore; TransferSemaphore = nullptr;
	delete CommandPool; CommandPool = nullptr;
	delete SwapChain; SwapChain = nullptr;
	delete Device; Device = nullptr;
}

void Renderer::SubmitCommands(bool present, int presentWidth, int presentHeight)
{
	if (present)
	{
		//RECT clientbox = {};
		//GetClientRect(WindowHandle, &clientbox);
		//int presentWidth = clientbox.right;
		//int presentHeight = clientbox.bottom;

		PresentImageIndex = SwapChain->acquireImage(presentWidth, presentHeight, ImageAvailableSemaphore);
		if (PresentImageIndex != 0xffffffff)
		{
			PPViewport box;
			box.x = 0;
			box.y = 0;
			box.width = presentWidth;
			box.height = presentHeight;
			Postprocess->drawPresentTexture(box);
		}
	}

	if (TransferCommands)
	{
		TransferCommands->end();

		QueueSubmit submit;
		submit.addCommandBuffer(TransferCommands);
		submit.addSignal(TransferSemaphore);
		submit.execute(Device, Device->graphicsQueue);
	}

	if (DrawCommands)
		DrawCommands->end();

	QueueSubmit submit;
	if (DrawCommands)
	{
		submit.addCommandBuffer(DrawCommands);
	}
	if (TransferCommands)
	{
		submit.addWait(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, TransferSemaphore);
	}
	if (present && PresentImageIndex != 0xffffffff)
	{
		submit.addWait(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, ImageAvailableSemaphore);
		submit.addSignal(RenderFinishedSemaphore);
	}
	submit.execute(Device, Device->graphicsQueue, RenderFinishedFence);

	if (present && PresentImageIndex != 0xffffffff)
	{
		SwapChain->queuePresent(PresentImageIndex, RenderFinishedSemaphore);
	}

	vkWaitForFences(Device->device, 1, &RenderFinishedFence->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(Device->device, 1, &RenderFinishedFence->fence);

	delete DrawCommands; DrawCommands = nullptr;
	delete TransferCommands; TransferCommands = nullptr;
	DeleteFrameObjects();
}

VulkanCommandBuffer* Renderer::GetTransferCommands()
{
	if (!TransferCommands)
	{
		TransferCommands = CommandPool->createBuffer().release();
		TransferCommands->begin();
	}
	return TransferCommands;
}

VulkanCommandBuffer* Renderer::GetDrawCommands()
{
	if (!DrawCommands)
	{
		DrawCommands = CommandPool->createBuffer().release();
		DrawCommands->begin();
	}
	return DrawCommands;
}

void Renderer::DeleteFrameObjects()
{
	delete FrameDeleteList; FrameDeleteList = nullptr;
	FrameDeleteList = new DeleteList();
}

std::unique_ptr<VulkanShader> Renderer::CreateVertexShader(VulkanDevice* device, const std::string& name, const std::string& defines)
{
	ShaderBuilder builder;
	builder.setVertexShader(LoadShaderCode(name, defines));
	return builder.create(device);
}

std::unique_ptr<VulkanShader> Renderer::CreateFragmentShader(VulkanDevice* device, const std::string& name, const std::string& defines)
{
	ShaderBuilder builder;
	builder.setFragmentShader(LoadShaderCode(name, defines));
	return builder.create(device);
}

std::unique_ptr<VulkanShader> Renderer::CreateComputeShader(VulkanDevice* device, const std::string& name, const std::string& defines)
{
	ShaderBuilder builder;
	builder.setComputeShader(LoadShaderCode(name, defines));
	return builder.create(device);
}

std::string Renderer::LoadShaderCode(const std::string& filename, const std::string& defines)
{
	const char* shaderversion = R"(
		#version 450
		#extension GL_ARB_separate_shader_objects : enable
	)";
	return shaderversion + defines + "\r\n#line 1\r\n" + FileResource::readAllText(filename);
}

void Renderer::CreateScenePipelineLayout()
{
	PipelineLayoutBuilder builder;
	builder.addSetLayout(SceneDescriptorSetLayout);
	builder.addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ScenePushConstants));
	ScenePipelineLayout = builder.create(Device).release();
}

void Renderer::CreateSceneDescriptorSetLayout()
{
	DescriptorSetLayoutBuilder builder;
	//builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	SceneDescriptorSetLayout = builder.create(Device).release();
}

void Renderer::CreateSceneVertexBuffer()
{
	size_t size = sizeof(SceneVertex) * 1'000'000;

	BufferBuilder builder;
	builder.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	builder.setMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	builder.setSize(size);

	SceneVertexBuffer = builder.create(Device).release();
	SceneVertices = (SceneVertex*)SceneVertexBuffer->Map(0, size);
	SceneVertexPos = 0;
}

void Renderer::CreateNullTexture()
{
	auto cmdbuffer = GetTransferCommands();

	ImageBuilder imgbuilder;
	imgbuilder.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	imgbuilder.setSize(1, 1);
	imgbuilder.setUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	NullTexture = imgbuilder.create(Device).release();

	ImageViewBuilder viewbuilder;
	viewbuilder.setImage(NullTexture, VK_FORMAT_R8G8B8A8_UNORM);
	NullTextureView = viewbuilder.create(Device).release();

	BufferBuilder builder;
	builder.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	builder.setSize(4);
	auto stagingbuffer = builder.create(Device);
	auto data = (uint32_t*)stagingbuffer->Map(0, 4);
	data[0] = 0xffffffff;
	stagingbuffer->Unmap();

	PipelineBarrier imageTransition0;
	imageTransition0.addImage(NullTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
	imageTransition0.execute(cmdbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { 1, 1, 1 };
	cmdbuffer->copyBufferToImage(stagingbuffer->buffer, NullTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	PipelineBarrier imageTransition1;
	imageTransition1.addImage(NullTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	imageTransition1.execute(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	FrameDeleteList->buffers.push_back(std::move(stagingbuffer));
}

void Renderer::UpdateTextureRect(FTextureInfo* texture, int x, int y, int w, int h)
{
	VulkanTexture*& tex = TextureCache[0][texture->CacheID];
	if (tex)
		tex->UpdateRect(this, *texture, x, y, w, h);
}

VulkanTexture* Renderer::GetTexture(FTextureInfo* texture, bool masked)
{
	if (!texture)
		return nullptr;

	VulkanTexture*& tex = TextureCache[(int)masked][texture->CacheID];
	if (!tex)
	{
		tex = new VulkanTexture(this, *texture, masked);
	}
	else if (texture->bRealtimeChanged)
	{
		texture->bRealtimeChanged = 0;
		tex->Update(this, *texture, masked);
	}
	return tex;
}

VulkanDescriptorSet* Renderer::GetTextureDescriptorSet(DWORD PolyFlags, VulkanTexture* tex, VulkanTexture* lightmap, VulkanTexture* macrotex, VulkanTexture* detailtex, bool clamp)
{
	uint32_t samplermode = 0;
	if (PolyFlags & PF_NoSmooth) samplermode |= 1;
	if (clamp) samplermode |= 2;

	auto& descriptorSet = TextureDescriptorSets[{ tex, lightmap, detailtex, macrotex, samplermode }];
	if (!descriptorSet)
	{
		if (SceneDescriptorPoolSetsLeft == 0)
		{
			DescriptorPoolBuilder builder;
			builder.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 * 4);
			builder.setMaxSets(1000);
			SceneDescriptorPool.push_back(builder.create(Device).release());
			SceneDescriptorPoolSetsLeft = 1000;
		}

		descriptorSet = SceneDescriptorPool.back()->allocate(SceneDescriptorSetLayout).release();
		SceneDescriptorPoolSetsLeft--;

		WriteDescriptors writes;
		int i = 0;
		for (VulkanTexture* texture : { tex, lightmap, macrotex, detailtex })
		{
			VulkanSampler* sampler = (i == 0) ? SceneSamplers->samplers[samplermode].get() : SceneSamplers->samplers[0].get();

			if (texture)
				writes.addCombinedImageSampler(descriptorSet, i++, texture->imageView.get(), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			else
				writes.addCombinedImageSampler(descriptorSet, i++, NullTextureView, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		writes.updateSets(Device);
	}
	return descriptorSet;
}

void Renderer::ClearTextureCache()
{
	for (auto it : TextureDescriptorSets)
		delete it.second;
	TextureDescriptorSets.clear();

	for (auto pool : SceneDescriptorPool)
		delete pool;
	SceneDescriptorPool.clear();
	SceneDescriptorPoolSetsLeft = 0;

	for (auto& cache : TextureCache)
	{
		for (auto it : cache)
			delete it.second;
		cache.clear();
	}
}

void Renderer::CopyScreenToBuffer(int w, int h, void* data, float gamma)
{
	// Convert from rgba16f to bgra8 using the GPU:
	ImageBuilder imgbuilder;
	imgbuilder.setFormat(VK_FORMAT_B8G8R8A8_UNORM);
	imgbuilder.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	imgbuilder.setSize(w, h);
	auto image = imgbuilder.create(Device);
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Postprocess->blitCurrentToImage(image.get(), &imageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Staging buffer for download
	BufferBuilder bufbuilder;
	bufbuilder.setSize(w * h * 4);
	bufbuilder.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
	auto staging = bufbuilder.create(Device);

	// Copy from image to buffer
	VkBufferImageCopy region = {};
	region.imageExtent.width = w;
	region.imageExtent.height = h;
	region.imageExtent.depth = 1;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	GetDrawCommands()->copyImageToBuffer(image->image, imageLayout, staging->buffer, 1, &region);

	// Submit command buffers and wait for device to finish the work
	SubmitCommands(false, 0, 0);

	uint8_t* pixels = (uint8_t*)staging->Map(0, w * h * 4);
	if (gamma != 1.0f)
	{
		float invGamma = 1.0f / gamma;

		uint8_t gammatable[256];
		for (int i = 0; i < 256; i++)
			gammatable[i] = (int)clamp(std::round(std::pow(i / 255.0f, invGamma) * 255.0f), 0.0f, 255.0f);

		uint8_t* dest = (uint8_t*)data;
		for (int i = 0; i < w * h * 4; i += 4)
		{
			dest[i] = gammatable[pixels[i]];
			dest[i + 1] = gammatable[pixels[i + 1]];
			dest[i + 2] = gammatable[pixels[i + 2]];
			dest[i + 3] = pixels[i + 3];
		}
	}
	else
	{
		memcpy(data, pixels, w * h * 4);
	}
	staging->Unmap();
}
