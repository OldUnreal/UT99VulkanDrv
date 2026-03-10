
#include "Precomp.h"
#include "CommandBufferManager.h"
#include "UVulkanRenderDevice.h"

CommandBufferManager::CommandBufferManager(UVulkanRenderDevice* renderer) : renderer(renderer)
{
	SwapChain = VulkanSwapChainBuilder()
		.Create(renderer->Device.get());

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		ImageAvailableSemaphores[i] = SemaphoreBuilder()
			.DebugName("ImageAvailableSemaphore")
			.Create(renderer->Device.get());

		RenderFinishedSemaphores[i] = SemaphoreBuilder()
			.DebugName("RenderFinishedSemaphore")
			.Create(renderer->Device.get());

		DrawFinishedSemaphores[i] = SemaphoreBuilder()
			.DebugName("DrawFinishedSemaphores")
			.Create(renderer->Device.get());

		TransferSemaphores[i] = SemaphoreBuilder()
			.DebugName("TransferSemaphore")
			.Create(renderer->Device.get());

		RenderFinishedFences[i] = FenceBuilder()
			.DebugName("RenderFinishedFence")
			.Flags(VK_FENCE_CREATE_SIGNALED_BIT)
			.Create(renderer->Device.get());

		FrameDeleteLists[i] = std::make_unique<DeleteList>();
	}

	CommandPool = CommandPoolBuilder()
		.QueueFamily(renderer->Device.get()->GraphicsFamily)
		.DebugName("CommandPool")
		.Create(renderer->Device.get());
}

CommandBufferManager::~CommandBufferManager()
{
	DeleteFrameObjects();
}

void CommandBufferManager::BeginFrame()
{
	VkFence currentFence = RenderFinishedFences[CurrentFrameIndex]->fence;
	vkWaitForFences(renderer->Device.get()->device, 1, &currentFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(renderer->Device.get()->device, 1, &currentFence);

	// Safely clear old Vulkan objects now that the GPU is 100% done with this frame index
	FrameDeleteLists[CurrentFrameIndex] = std::make_unique<DeleteList>();

	// Reset per-frame CPU write positions now that this frame index is safe to reuse
	renderer->Buffers->UploadBufferPositions[CurrentFrameIndex] = 0;

}

void CommandBufferManager::WaitForTransfer()
{
	renderer->Uploads->SubmitUploads();

	auto& TransferCommands = TransferCommandsArray[CurrentFrameIndex];
	auto& RenderFinishedFence = RenderFinishedFences[CurrentFrameIndex];

	if (TransferCommands)
	{
		TransferCommands->end();

		QueueSubmit()
			.AddCommandBuffer(TransferCommands.get())
			.Execute(renderer->Device.get(), renderer->Device.get()->GraphicsQueue, RenderFinishedFence.get());
		vkWaitForFences(renderer->Device.get()->device, 1, &RenderFinishedFence->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(renderer->Device.get()->device, 1, &RenderFinishedFence->fence);

		TransferCommands->begin();
	}
}

void CommandBufferManager::SubmitCommands(bool present, int presentWidth, int presentHeight, bool presentFullscreen)
{
	renderer->Uploads->SubmitUploads();

	auto& ImageAvailableSemaphore = ImageAvailableSemaphores[CurrentFrameIndex];
	auto& RenderFinishedSemaphore = RenderFinishedSemaphores[CurrentFrameIndex];
	auto& TransferSemaphore = TransferSemaphores[CurrentFrameIndex];
	auto& RenderFinishedFence = RenderFinishedFences[CurrentFrameIndex];
	auto& DrawCommands = DrawCommandsArray[CurrentFrameIndex];
	auto& TransferCommands = TransferCommandsArray[CurrentFrameIndex];

	if (present)
	{
		if (SwapChain->Lost() || SwapChain->Width() != presentWidth || SwapChain->Height() != presentHeight || UsingVsync != renderer->UseVSync || UsingHdr != renderer->Hdr)
		{
			UsingVsync = renderer->UseVSync;
			UsingHdr = renderer->Hdr;
			renderer->Framebuffers->DestroySwapChainFramebuffers();
			SwapChain->Create(presentWidth, presentHeight, renderer->UseVSync ? 2 : 3, renderer->UseVSync, renderer->Hdr, renderer->VkExclusiveFullscreen && presentFullscreen);
			renderer->Framebuffers->CreateSwapChainFramebuffers();
		}

		PresentImageIndex = SwapChain->AcquireImage(ImageAvailableSemaphore.get());
		if (PresentImageIndex != -1)
		{
			renderer->DrawPresentTexture(presentWidth, presentHeight);
		}
	}

	if (TransferCommands)
	{
		TransferCommands->end();

		auto SubmitTransfer = QueueSubmit();
		SubmitTransfer.AddCommandBuffer(TransferCommands.get());
		SubmitTransfer.AddSignal(TransferSemaphore.get());

		if (!IsFirstFrame)
		{
			uint32_t PrevFrame = (CurrentFrameIndex + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
			SubmitTransfer.AddWait(VK_PIPELINE_STAGE_TRANSFER_BIT, DrawFinishedSemaphores[PrevFrame].get());
		}

		SubmitTransfer.Execute(renderer->Device.get(), renderer->Device.get()->GraphicsQueue);
	}

	if (DrawCommands)
		DrawCommands->end();

	QueueSubmit submit;
	if (DrawCommands)
	{
		submit.AddCommandBuffer(DrawCommands.get());
	}
	if (TransferCommands)
	{
		submit.AddWait(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, TransferSemaphore.get());
	}
	if (present && PresentImageIndex != -1)
	{
		submit.AddWait(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, ImageAvailableSemaphore.get());
		submit.AddSignal(RenderFinishedSemaphore.get());
	}
	submit.AddSignal(DrawFinishedSemaphores[CurrentFrameIndex].get());
	submit.Execute(renderer->Device.get(), renderer->Device.get()->GraphicsQueue, RenderFinishedFence.get());

	FrameBegun = false;
	IsFirstFrame = false;

	if (present && PresentImageIndex != -1)
	{
		SwapChain->QueuePresent(PresentImageIndex, RenderFinishedSemaphore.get());
	}

	// Advance frame index. NO vkWaitForFences here!
	CurrentFrameIndex = (CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

VulkanCommandBuffer* CommandBufferManager::GetTransferCommands()
{
	if (!FrameBegun)
	{
		BeginFrame();
		DrawCommandsBegun[CurrentFrameIndex] = false;
		TransferCommandsBegun[CurrentFrameIndex] = false;
		FrameBegun = true;
	}

	auto& TransferCommands = TransferCommandsArray[CurrentFrameIndex];
	if (!TransferCommands)
		TransferCommands = CommandPool->createBuffer();

	if (!TransferCommandsBegun[CurrentFrameIndex])
	{
		TransferCommands->begin();
		TransferCommandsBegun[CurrentFrameIndex] = true;
	}
	return TransferCommands.get();
}

VulkanCommandBuffer* CommandBufferManager::GetDrawCommands()
{
	if (!FrameBegun)
	{
		BeginFrame();
		DrawCommandsBegun[CurrentFrameIndex] = false;
		TransferCommandsBegun[CurrentFrameIndex] = false;
		FrameBegun = true;
	}

	auto& DrawCommands = DrawCommandsArray[CurrentFrameIndex];
	if (!DrawCommands)
		DrawCommands = CommandPool->createBuffer();

	if (!DrawCommandsBegun[CurrentFrameIndex])
	{
		DrawCommands->begin();
		DrawCommandsBegun[CurrentFrameIndex] = true;
	}
	return DrawCommands.get();
}

void CommandBufferManager::DeleteFrameObjects()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		FrameDeleteLists[i] = std::make_unique<DeleteList>();
}
