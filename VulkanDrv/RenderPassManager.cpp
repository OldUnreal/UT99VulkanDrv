
#include "Precomp.h"
#include "RenderPassManager.h"
#include "UVulkanRenderDevice.h"
#include "VulkanBuilders.h"
#include "VulkanSwapChain.h"

RenderPassManager::RenderPassManager(UVulkanRenderDevice* renderer) : renderer(renderer)
{
	CreateScenePipelineLayout();
	CreatePresentPipelineLayout();
}

RenderPassManager::~RenderPassManager()
{
}

void RenderPassManager::CreateScenePipelineLayout()
{
	PipelineLayoutBuilder builder;
	builder.addSetLayout(renderer->DescriptorSets->SceneDescriptorSetLayout.get());
	builder.addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ScenePushConstants));
	ScenePipelineLayout = builder.create(renderer->Device);
}

void RenderPassManager::CreatePresentPipelineLayout()
{
	PipelineLayoutBuilder builder;
	builder.addSetLayout(renderer->DescriptorSets->PresentDescriptorSetLayout.get());
	builder.addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PresentPushConstants));
	PresentPipelineLayout = builder.create(renderer->Device);
}

void RenderPassManager::BeginScene(VulkanCommandBuffer* cmdbuffer)
{
	RenderPassBegin renderPassInfo;
	renderPassInfo.setRenderPass(SceneRenderPass.get());
	renderPassInfo.setFramebuffer(renderer->Framebuffers->sceneFramebuffer.get());
	renderPassInfo.setRenderArea(0, 0, renderer->Textures->Scene->width, renderer->Textures->Scene->height);
	renderPassInfo.addClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	renderPassInfo.addClearDepthStencil(1.0f, 0);
	cmdbuffer->beginRenderPass(renderPassInfo);
}

void RenderPassManager::EndScene(VulkanCommandBuffer* cmdbuffer)
{
	cmdbuffer->endRenderPass();
}

void RenderPassManager::BeginPresent(VulkanCommandBuffer* cmdbuffer)
{
	RenderPassBegin renderPassInfo;
	renderPassInfo.setRenderPass(PresentRenderPass.get());
	renderPassInfo.setFramebuffer(renderer->Framebuffers->GetSwapChainFramebuffer());
	renderPassInfo.setRenderArea(0, 0, renderer->Textures->Scene->width, renderer->Textures->Scene->height);
	renderPassInfo.addClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	cmdbuffer->beginRenderPass(renderPassInfo);
}

void RenderPassManager::EndPresent(VulkanCommandBuffer* cmdbuffer)
{
	cmdbuffer->endRenderPass();
}

VulkanPipeline* RenderPassManager::getPipeline(DWORD PolyFlags)
{
	// Adjust PolyFlags according to Unreal's precedence rules.
	if (!(PolyFlags & (PF_Translucent | PF_Modulated)))
		PolyFlags |= PF_Occlude;
	else if (PolyFlags & PF_Translucent)
		PolyFlags &= ~PF_Masked;

	int index;
	if (PolyFlags & PF_Translucent)
	{
		index = 0;
	}
	else if (PolyFlags & PF_Modulated)
	{
		index = 1;
	}
	else if (PolyFlags & PF_Highlighted)
	{
		index = 2;
	}
	else
	{
		index = 3;
	}

	if (PolyFlags & PF_Invisible)
	{
		index |= 4;
	}
	if (PolyFlags & PF_Occlude)
	{
		index |= 8;
	}
	if (PolyFlags & PF_Masked)
	{
		index |= 16;
	}

	return pipeline[index].get();
}

VulkanPipeline* RenderPassManager::getEndFlashPipeline()
{
	return pipeline[2].get();
}

void RenderPassManager::CreatePipelines()
{
	for (int i = 0; i < 32; i++)
	{
		GraphicsPipelineBuilder builder;
		builder.addVertexShader(renderer->Shaders->vertexShader.get());
		builder.setViewport(0.0f, 0.0f, (float)renderer->Textures->Scene->width, (float)renderer->Textures->Scene->height);
		builder.setScissor(0, 0, renderer->Textures->Scene->width, renderer->Textures->Scene->height);
		builder.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
		builder.setCull(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		builder.addVertexBufferBinding(0, sizeof(SceneVertex));
		builder.addVertexAttribute(0, 0, VK_FORMAT_R32_UINT, offsetof(SceneVertex, flags));
		builder.addVertexAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, x));
		builder.addVertexAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SceneVertex, u));
		builder.addVertexAttribute(3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SceneVertex, u2));
		builder.addVertexAttribute(4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SceneVertex, u3));
		builder.addVertexAttribute(5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SceneVertex, u4));
		builder.addVertexAttribute(6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SceneVertex, r));
		builder.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		builder.setLayout(ScenePipelineLayout.get());
		builder.setRenderPass(SceneRenderPass.get());

		// Avoid clipping the weapon. The UE1 engine clips the geometry anyway.
		if (renderer->Device->enabledDeviceFeatures.depthClamp)
			builder.setDepthClampEnable(true);

		switch (i & 3)
		{
		case 0: // PF_Translucent
			builder.setBlendMode(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
			break;
		case 1: // PF_Modulated
			builder.setBlendMode(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_SRC_COLOR);
			break;
		case 2: // PF_Highlighted
			builder.setBlendMode(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
			break;
		case 3:
			builder.setBlendMode(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO); // Hmm, is it faster to keep the blend mode enabled or to toggle it?
			break;
		}

		if (i & 4) // PF_Invisible
		{
			builder.setColorWriteMask(0);
		}

		if (i & 8) // PF_Occlude
		{
			builder.setDepthStencilEnable(true, true, false);
		}
		else
		{
			builder.setDepthStencilEnable(true, false, false);
		}

		if (i & 16) // PF_Masked
			builder.addFragmentShader(renderer->Shaders->fragmentShaderAlphaTest.get());
		else
			builder.addFragmentShader(renderer->Shaders->fragmentShader.get());

		builder.setSubpassColorAttachmentCount(1);
		builder.setRasterizationSamples(renderer->Textures->Scene->sceneSamples);

		pipeline[i] = builder.create(renderer->Device);
	}
}

void RenderPassManager::CreateRenderPass()
{
	RenderPassBuilder builder;

	builder.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, renderer->Textures->Scene->sceneSamples, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	builder.addDepthStencilAttachment(VK_FORMAT_D32_SFLOAT, renderer->Textures->Scene->sceneSamples, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	builder.addExternalSubpassDependency(
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	builder.addSubpass();
	builder.addSubpassColorAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	builder.addSubpassDepthStencilAttachmentRef(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	SceneRenderPass = builder.create(renderer->Device);
}

void RenderPassManager::CreatePresentRenderPass()
{
	RenderPassBuilder builder;

	builder.addAttachment(renderer->Commands->SwapChain->swapChainFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	builder.addExternalSubpassDependency(
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	builder.addSubpass();
	builder.addSubpassColorAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	PresentRenderPass = builder.create(renderer->Device);
}

void RenderPassManager::CreatePresentPipeline()
{
	GraphicsPipelineBuilder builder;
	builder.addVertexShader(renderer->Shaders->ppVertexShader.get());
	builder.addFragmentShader(renderer->Shaders->ppFragmentPresentShader.get());

	builder.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	builder.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	// Note: the actual values are ignored since we use dynamic viewport+scissor states
	builder.setViewport(0.0f, 0.0f, 320.0f, 200.0f);
	builder.setScissor(0.0f, 0.0f, 320.0f, 200.0f);
	builder.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	builder.setRasterizationSamples(VK_SAMPLE_COUNT_1_BIT);
	builder.setLayout(PresentPipelineLayout.get());
	builder.setRenderPass(PresentRenderPass.get());
	PresentPipeline = builder.create(renderer->Device);
}
