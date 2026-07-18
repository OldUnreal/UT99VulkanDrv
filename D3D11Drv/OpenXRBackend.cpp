
#include "Precomp.h"
#include "VRBackend.h"
#include "mat.h"

#include <cmath>
#include <vector>
#include <cstring>

// OpenXR headers are vendored (External/openxr/include). We late-bind the
// loader, so no openxr_loader.lib at link time and mono never touches it.
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

namespace
{
	// Swapchain color format. sRGB so the compositor reads the encoded pixels
	// correctly. ponytail: fixed format; if a runtime rejects it VR won't start
	// (logged) — enumerate-and-pick only if that shows up. Gamma reconciliation
	// with the present pass is a known M0 tuning item (see VR_NOTES.md).
	const int64_t kSwapchainFormat = 29; // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

	mat4 ProjFromFov(const XrFovf& fov)
	{
		// near=1, far=32768 to match SetSceneNode's frustum exactly. Tangents are
		// signed (left/down negative), which lines up with mat4::frustum(l,r,b,t).
		float l = std::tan(fov.angleLeft);
		float r = std::tan(fov.angleRight);
		float b = std::tan(fov.angleDown);
		float t = std::tan(fov.angleUp);
		return mat4::frustum(l, r, b, t, 1.0f, 32768.0f, handedness::left, clipzrange::zero_positive_w);
	}
}

class OpenXRBackend : public VRBackend
{
public:
	OpenXRBackend(HMODULE loader) : LoaderModule(loader) {}
	~OpenXRBackend() { Stop(); }

	bool QueryAdapterLuid(uint64_t& outLuid) override;
	bool Start(ID3D11Device* device, float resScale) override;
	void GetEyeResolution(uint32_t& width, uint32_t& height) override { width = EyeWidth; height = EyeHeight; }
	void GetHeadOrientation(float outQuat[4]) override
	{
		outQuat[0] = HeadOrientation.x; outQuat[1] = HeadOrientation.y;
		outQuat[2] = HeadOrientation.z; outQuat[3] = HeadOrientation.w;
	}
	bool BeginFrame(VREyePose outEyes[2]) override;
	ID3D11Texture2D* BeginEye(int eye) override;
	void EndEye(int eye) override;
	void EndFrame(bool rendered) override;
	bool Running() const override { return SessionRunning; }
	// Worn = the runtime reports us focused, and (if the runtime supports XR_EXT_user_presence)
	// the proximity sensor says the HMD is on the head. Either signal going false = doffed.
	bool IsWorn() const override
	{
		return SessionState == XR_SESSION_STATE_FOCUSED && (!UserPresenceEnabled || UserPresent);
	}
	void Stop() override;

private:
	bool LoadInstanceFunctions();
	void PollEvents();

	template<typename T> bool Load(const char* name, T& fp)
	{
		return XR_SUCCEEDED(xrGetInstanceProcAddr(Instance, name, (PFN_xrVoidFunction*)&fp));
	}

	HMODULE LoaderModule = nullptr;

	// Late-bound entry points.
	PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr = nullptr;
	PFN_xrCreateInstance xrCreateInstance = nullptr;
	PFN_xrDestroyInstance xrDestroyInstance = nullptr;
	PFN_xrGetSystem xrGetSystem = nullptr;
	PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR = nullptr;
	PFN_xrCreateSession xrCreateSession = nullptr;
	PFN_xrDestroySession xrDestroySession = nullptr;
	PFN_xrCreateReferenceSpace xrCreateReferenceSpace = nullptr;
	PFN_xrDestroySpace xrDestroySpace = nullptr;
	PFN_xrEnumerateViewConfigurationViews xrEnumerateViewConfigurationViews = nullptr;
	PFN_xrCreateSwapchain xrCreateSwapchain = nullptr;
	PFN_xrDestroySwapchain xrDestroySwapchain = nullptr;
	PFN_xrEnumerateSwapchainImages xrEnumerateSwapchainImages = nullptr;
	PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage = nullptr;
	PFN_xrWaitSwapchainImage xrWaitSwapchainImage = nullptr;
	PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage = nullptr;
	PFN_xrBeginSession xrBeginSession = nullptr;
	PFN_xrEndSession xrEndSession = nullptr;
	PFN_xrPollEvent xrPollEvent = nullptr;
	PFN_xrWaitFrame xrWaitFrame = nullptr;
	PFN_xrBeginFrame xrBeginFrame = nullptr;
	PFN_xrEndFrame xrEndFrame = nullptr;
	PFN_xrLocateViews xrLocateViews = nullptr;

	XrInstance Instance = XR_NULL_HANDLE;
	XrSystemId SystemId = XR_NULL_SYSTEM_ID;
	XrSession Session = XR_NULL_HANDLE;
	XrSpace Space = XR_NULL_HANDLE;

	XrSwapchain Swapchains[2] = { XR_NULL_HANDLE, XR_NULL_HANDLE };
	std::vector<XrSwapchainImageD3D11KHR> SwapchainImages[2];

	uint32_t EyeWidth = 0;
	uint32_t EyeHeight = 0;

	XrSessionState SessionState = XR_SESSION_STATE_UNKNOWN;
	bool SessionRunning = false;
	bool FrameBegun = false; // xrBeginFrame issued this frame → EndFrame must close it
	bool UserPresenceEnabled = false; // XR_EXT_user_presence available & enabled
	bool UserPresent = true;          // proximity sensor: HMD on head (only meaningful if enabled)
	int FrameLog = 0;        // diagnostics: log the first handful of frames

	XrTime PredictedDisplayTime = 0;
	XrView Views[2] = {};
	XrCompositionLayerProjectionView ProjViews[2] = {};
	XrQuaternionf HeadOrientation = { 0.0f, 0.0f, 0.0f, 1.0f };
};

bool OpenXRBackend::QueryAdapterLuid(uint64_t& outLuid)
{
	guard(OpenXRBackend::QueryAdapterLuid);

	xrGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)GetProcAddress(LoaderModule, "xrGetInstanceProcAddr");
	if (!xrGetInstanceProcAddr)
	{
		debugf(TEXT("D3D11Drv VR: openxr_loader.dll has no xrGetInstanceProcAddr"));
		return false;
	}

	// Null-instance entry points.
	PFN_xrCreateInstance createInstance = nullptr;
	if (XR_FAILED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrCreateInstance", (PFN_xrVoidFunction*)&createInstance)) || !createInstance)
	{
		debugf(TEXT("D3D11Drv VR: could not resolve xrCreateInstance"));
		return false;
	}

	const char* extensions[2] = { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
	uint32_t extCount = 1;

#ifdef XR_EXT_USER_PRESENCE_EXTENSION_NAME
	// Enable XR_EXT_user_presence if the runtime lists it — gives an explicit "HMD on head"
	// signal (proximity sensor). Enumerate first; enabling an unsupported extension would fail
	// xrCreateInstance. Absent -> fall back to session-focus detection.
	{
		PFN_xrEnumerateInstanceExtensionProperties enumExt = nullptr;
		xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&enumExt);
		uint32_t n = 0;
		if (enumExt && XR_SUCCEEDED(enumExt(nullptr, 0, &n, nullptr)) && n)
		{
			std::vector<XrExtensionProperties> props(n, { XR_TYPE_EXTENSION_PROPERTIES });
			if (XR_SUCCEEDED(enumExt(nullptr, n, &n, props.data())))
			{
				for (uint32_t i = 0; i < n; i++)
					if (strcmp(props[i].extensionName, XR_EXT_USER_PRESENCE_EXTENSION_NAME) == 0)
						UserPresenceEnabled = true;
			}
		}
		if (UserPresenceEnabled)
			extensions[extCount++] = XR_EXT_USER_PRESENCE_EXTENSION_NAME;
	}
#endif

	XrInstanceCreateInfo ci = { XR_TYPE_INSTANCE_CREATE_INFO };
	ci.enabledExtensionCount = extCount;
	ci.enabledExtensionNames = extensions;
	strcpy_s(ci.applicationInfo.applicationName, "UT469 D3D11Drv");
	ci.applicationInfo.applicationVersion = 1;
	strcpy_s(ci.applicationInfo.engineName, "Unreal");
	ci.applicationInfo.engineVersion = 1;
	ci.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

	XrResult r = createInstance(&ci, &Instance);
	if (XR_FAILED(r))
	{
		debugf(TEXT("D3D11Drv VR: xrCreateInstance failed (%d) — no runtime / D3D11 unsupported"), (int)r);
		return false;
	}

	if (!LoadInstanceFunctions())
		return false;

	XrSystemGetInfo sysInfo = { XR_TYPE_SYSTEM_GET_INFO };
	sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	r = xrGetSystem(Instance, &sysInfo, &SystemId);
	if (XR_FAILED(r))
	{
		debugf(TEXT("D3D11Drv VR: xrGetSystem failed (%d) — headset not connected/ready"), (int)r);
		return false;
	}

	XrGraphicsRequirementsD3D11KHR gr = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	r = xrGetD3D11GraphicsRequirementsKHR(Instance, SystemId, &gr);
	if (XR_FAILED(r))
	{
		debugf(TEXT("D3D11Drv VR: xrGetD3D11GraphicsRequirementsKHR failed (%d)"), (int)r);
		return false;
	}

	// LUID packs as a signed HighPart + unsigned LowPart; a raw 64-bit copy
	// matches DXGI_ADAPTER_DESC::AdapterLuid for comparison.
	uint64_t luid = 0;
	static_assert(sizeof(gr.adapterLuid) == sizeof(luid), "LUID size");
	memcpy(&luid, &gr.adapterLuid, sizeof(luid));
	outLuid = luid;
	return true;

	unguard;
}

bool OpenXRBackend::LoadInstanceFunctions()
{
	bool ok = true;
	ok &= Load("xrDestroyInstance", xrDestroyInstance);
	ok &= Load("xrGetSystem", xrGetSystem);
	ok &= Load("xrGetD3D11GraphicsRequirementsKHR", xrGetD3D11GraphicsRequirementsKHR);
	ok &= Load("xrCreateSession", xrCreateSession);
	ok &= Load("xrDestroySession", xrDestroySession);
	ok &= Load("xrCreateReferenceSpace", xrCreateReferenceSpace);
	ok &= Load("xrDestroySpace", xrDestroySpace);
	ok &= Load("xrEnumerateViewConfigurationViews", xrEnumerateViewConfigurationViews);
	ok &= Load("xrCreateSwapchain", xrCreateSwapchain);
	ok &= Load("xrDestroySwapchain", xrDestroySwapchain);
	ok &= Load("xrEnumerateSwapchainImages", xrEnumerateSwapchainImages);
	ok &= Load("xrAcquireSwapchainImage", xrAcquireSwapchainImage);
	ok &= Load("xrWaitSwapchainImage", xrWaitSwapchainImage);
	ok &= Load("xrReleaseSwapchainImage", xrReleaseSwapchainImage);
	ok &= Load("xrBeginSession", xrBeginSession);
	ok &= Load("xrEndSession", xrEndSession);
	ok &= Load("xrPollEvent", xrPollEvent);
	ok &= Load("xrWaitFrame", xrWaitFrame);
	ok &= Load("xrBeginFrame", xrBeginFrame);
	ok &= Load("xrEndFrame", xrEndFrame);
	ok &= Load("xrLocateViews", xrLocateViews);
	if (!ok)
		debugf(TEXT("D3D11Drv VR: failed to resolve one or more OpenXR entry points"));
	return ok;
}

bool OpenXRBackend::Start(ID3D11Device* device, float resScale)
{
	guard(OpenXRBackend::Start);

	if (Instance == XR_NULL_HANDLE || SystemId == XR_NULL_SYSTEM_ID)
		return false;

	// View configuration → recommended eye resolution.
	uint32_t viewCount = 0;
	XrResult r = xrEnumerateViewConfigurationViews(Instance, SystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
	if (XR_FAILED(r) || viewCount != 2)
	{
		debugf(TEXT("D3D11Drv VR: unexpected view config (count=%d, r=%d)"), (int)viewCount, (int)r);
		return false;
	}
	XrViewConfigurationView configViews[2] = { { XR_TYPE_VIEW_CONFIGURATION_VIEW }, { XR_TYPE_VIEW_CONFIGURATION_VIEW } };
	xrEnumerateViewConfigurationViews(Instance, SystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, configViews);

	// Supersample the recommended resolution. The real ceiling is the runtime's maxImageRect
	// (clamped below) — the scale range is only a sanity guard against garbage config values.
	if (resScale < 1.0f / 16.0f) resScale = 1.0f / 16.0f;
	if (resScale > 16.0f) resScale = 16.0f;
	EyeWidth = (uint32_t)(configViews[0].recommendedImageRectWidth * resScale);
	EyeHeight = (uint32_t)(configViews[0].recommendedImageRectHeight * resScale);
	if (EyeWidth < 64) EyeWidth = 64;
	if (EyeHeight < 64) EyeHeight = 64;
	if (configViews[0].maxImageRectWidth && EyeWidth > configViews[0].maxImageRectWidth) EyeWidth = configViews[0].maxImageRectWidth;
	if (configViews[0].maxImageRectHeight && EyeHeight > configViews[0].maxImageRectHeight) EyeHeight = configViews[0].maxImageRectHeight;

	XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	binding.device = device;

	XrSessionCreateInfo sci = { XR_TYPE_SESSION_CREATE_INFO };
	sci.next = &binding;
	sci.systemId = SystemId;
	r = xrCreateSession(Instance, &sci, &Session);
	if (XR_FAILED(r))
	{
		debugf(TEXT("D3D11Drv VR: xrCreateSession failed (%d) — adapter LUID mismatch?"), (int)r);
		return false;
	}

	XrReferenceSpaceCreateInfo spci = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	spci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spci.poseInReferenceSpace.orientation.w = 1.0f; // identity
	r = xrCreateReferenceSpace(Session, &spci, &Space);
	if (XR_FAILED(r))
	{
		debugf(TEXT("D3D11Drv VR: xrCreateReferenceSpace failed (%d)"), (int)r);
		return false;
	}

	for (int eye = 0; eye < 2; eye++)
	{
		XrSwapchainCreateInfo scci = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		scci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
		scci.format = kSwapchainFormat;
		scci.sampleCount = 1;
		scci.width = EyeWidth;
		scci.height = EyeHeight;
		scci.faceCount = 1;
		scci.arraySize = 1;
		scci.mipCount = 1;
		r = xrCreateSwapchain(Session, &scci, &Swapchains[eye]);
		if (XR_FAILED(r))
		{
			debugf(TEXT("D3D11Drv VR: xrCreateSwapchain failed (%d) — format unsupported?"), (int)r);
			return false;
		}

		uint32_t imageCount = 0;
		xrEnumerateSwapchainImages(Swapchains[eye], 0, &imageCount, nullptr);
		SwapchainImages[eye].resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
		xrEnumerateSwapchainImages(Swapchains[eye], imageCount, &imageCount, (XrSwapchainImageBaseHeader*)SwapchainImages[eye].data());
	}

	debugf(TEXT("D3D11Drv VR: OpenXR session up, eye %dx%d"), (int)EyeWidth, (int)EyeHeight);
	return true;

	unguard;
}

void OpenXRBackend::PollEvents()
{
	for (;;)
	{
		XrEventDataBuffer ev = { XR_TYPE_EVENT_DATA_BUFFER };
		XrResult r = xrPollEvent(Instance, &ev);
		if (r != XR_SUCCESS)
			break;

		if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		{
			XrEventDataSessionStateChanged& s = *(XrEventDataSessionStateChanged*)&ev;
			SessionState = s.state;

			if (SessionState == XR_SESSION_STATE_READY)
			{
				XrSessionBeginInfo bi = { XR_TYPE_SESSION_BEGIN_INFO };
				bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				if (XR_SUCCEEDED(xrBeginSession(Session, &bi)))
					SessionRunning = true;
			}
			else if (SessionState == XR_SESSION_STATE_STOPPING)
			{
				SessionRunning = false;
				xrEndSession(Session);
			}
			else if (SessionState == XR_SESSION_STATE_EXITING || SessionState == XR_SESSION_STATE_LOSS_PENDING)
			{
				SessionRunning = false;
			}
		}
		else if (ev.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
		{
			SessionRunning = false;
		}
#ifdef XR_EXT_USER_PRESENCE_EXTENSION_NAME
		else if (ev.type == XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT)
		{
			XrEventDataUserPresenceChangedEXT& u = *(XrEventDataUserPresenceChangedEXT*)&ev;
			UserPresent = (u.isUserPresent == XR_TRUE);
		}
#endif
	}
}

bool OpenXRBackend::BeginFrame(VREyePose outEyes[2])
{
	guard(OpenXRBackend::BeginFrame);

	PollEvents();
	if (!SessionRunning)
		return false;

	XrFrameWaitInfo wi = { XR_TYPE_FRAME_WAIT_INFO };
	XrFrameState fs = { XR_TYPE_FRAME_STATE };
	if (XR_FAILED(xrWaitFrame(Session, &wi, &fs)))
		return false;
	PredictedDisplayTime = fs.predictedDisplayTime;

	XrFrameBeginInfo bi = { XR_TYPE_FRAME_BEGIN_INFO };
	xrBeginFrame(Session, &bi);
	FrameBegun = true;

	bool logNow = FrameLog < 12;
	if (logNow)
	{
		FrameLog++;
		debugf(TEXT("D3D11Drv VR: frame state=%d shouldRender=%d"), (int)SessionState, (int)fs.shouldRender);
	}

	if (!fs.shouldRender)
		return false; // frame still needs EndFrame(false)

	XrViewLocateInfo vli = { XR_TYPE_VIEW_LOCATE_INFO };
	vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	vli.displayTime = PredictedDisplayTime;
	vli.space = Space;

	XrViewState vs = { XR_TYPE_VIEW_STATE };
	uint32_t viewCount = 0;
	Views[0] = { XR_TYPE_VIEW };
	Views[1] = { XR_TYPE_VIEW };
	XrResult lr = xrLocateViews(Session, &vli, &vs, 2, &viewCount, Views);

	if (logNow)
	{
		debugf(TEXT("D3D11Drv VR: locate=%d flags=0x%x count=%d fovL=%.3f pos0=(%.3f,%.3f,%.3f)"),
			(int)lr, (int)vs.viewStateFlags, (int)viewCount,
			Views[0].fov.angleLeft, Views[0].pose.position.x, Views[0].pose.position.y, Views[0].pose.position.z);
	}

	if (XR_FAILED(lr) || viewCount != 2)
		return false;
	if ((vs.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
		(vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
		return false;

	// IPD from the runtime; M0 ignores head orientation and lays the two eyes on
	// the game camera's right axis (±IPD/2). Clean horizontal stereo regardless
	// of how the head is turned. Head-lock is the accepted M0 limitation.
	float dx = Views[1].pose.position.x - Views[0].pose.position.x;
	float dy = Views[1].pose.position.y - Views[0].pose.position.y;
	float dz = Views[1].pose.position.z - Views[0].pose.position.z;
	float ipd = std::sqrt(dx * dx + dy * dy + dz * dz);
	float half = ipd * 0.5f;

	outEyes[0].Projection = ProjFromFov(Views[0].fov);
	outEyes[0].ViewOffset = vec3(-half, 0.0f, 0.0f);
	outEyes[1].Projection = ProjFromFov(Views[1].fov);
	outEyes[1].ViewOffset = vec3(+half, 0.0f, 0.0f);

	// Head orientation (both eyes share it closely) for the look/roll modes.
	HeadOrientation = Views[0].pose.orientation;
	return true;

	unguard;
}

ID3D11Texture2D* OpenXRBackend::BeginEye(int eye)
{
	uint32_t index = 0;
	XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	XrResult ar = xrAcquireSwapchainImage(Swapchains[eye], &ai, &index);
	if (XR_FAILED(ar))
	{
		debugf(TEXT("D3D11Drv VR: xrAcquireSwapchainImage eye=%d failed %d"), eye, (int)ar);
		return nullptr;
	}

	XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	wi.timeout = XR_INFINITE_DURATION;
	XrResult wr = xrWaitSwapchainImage(Swapchains[eye], &wi);
	if (XR_FAILED(wr))
	{
		debugf(TEXT("D3D11Drv VR: xrWaitSwapchainImage eye=%d failed %d"), eye, (int)wr);
		return nullptr;
	}

	return SwapchainImages[eye][index].texture;
}

void OpenXRBackend::EndEye(int eye)
{
	XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
	xrReleaseSwapchainImage(Swapchains[eye], &ri);
}

void OpenXRBackend::EndFrame(bool rendered)
{
	guard(OpenXRBackend::EndFrame);

	if (!FrameBegun) // session not rendering — nothing was begun
		return;
	FrameBegun = false;

	XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	if (rendered)
	{
		for (int eye = 0; eye < 2; eye++)
		{
			ProjViews[eye] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
			ProjViews[eye].pose = Views[eye].pose;
			ProjViews[eye].fov = Views[eye].fov;
			ProjViews[eye].subImage.swapchain = Swapchains[eye];
			ProjViews[eye].subImage.imageRect.offset = { 0, 0 };
			ProjViews[eye].subImage.imageRect.extent = { (int32_t)EyeWidth, (int32_t)EyeHeight };
			ProjViews[eye].subImage.imageArrayIndex = 0;
		}
		layer.space = Space;
		layer.viewCount = 2;
		layer.views = ProjViews;
	}

	const XrCompositionLayerBaseHeader* layers[1] = { (const XrCompositionLayerBaseHeader*)&layer };
	XrFrameEndInfo ei = { XR_TYPE_FRAME_END_INFO };
	ei.displayTime = PredictedDisplayTime;
	ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	ei.layerCount = rendered ? 1 : 0;
	ei.layers = rendered ? layers : nullptr;
	XrResult er = xrEndFrame(Session, &ei);
	if (XR_FAILED(er) || FrameLog < 12)
		debugf(TEXT("D3D11Drv VR: xrEndFrame rendered=%d layers=%d result=%d"), (int)rendered, (int)ei.layerCount, (int)er);

	unguard;
}

void OpenXRBackend::Stop()
{
	SessionRunning = false;
	for (int eye = 0; eye < 2; eye++)
	{
		if (Swapchains[eye] != XR_NULL_HANDLE && xrDestroySwapchain)
			xrDestroySwapchain(Swapchains[eye]);
		Swapchains[eye] = XR_NULL_HANDLE;
		SwapchainImages[eye].clear();
	}
	if (Space != XR_NULL_HANDLE && xrDestroySpace) { xrDestroySpace(Space); Space = XR_NULL_HANDLE; }
	if (Session != XR_NULL_HANDLE && xrDestroySession) { xrDestroySession(Session); Session = XR_NULL_HANDLE; }
	if (Instance != XR_NULL_HANDLE && xrDestroyInstance) { xrDestroyInstance(Instance); Instance = XR_NULL_HANDLE; }
	if (LoaderModule) { FreeLibrary(LoaderModule); LoaderModule = nullptr; }
}

VRBackend* CreateOpenXRBackend()
{
	HMODULE loader = LoadLibraryA("openxr_loader.dll");
	if (!loader)
	{
		debugf(TEXT("D3D11Drv VR: openxr_loader.dll not found — VR unavailable, using mono"));
		return nullptr;
	}
	return new OpenXRBackend(loader);
}
