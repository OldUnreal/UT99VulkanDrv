
#include "Precomp.h"
#include "UD3D11RenderDevice.h"
#include "CachedTexture.h"
#include "UTF16.h"
#include "FileResource.h"
#include "halffloat.h"

#ifdef USE_SSE2
// Unfortunately this code is slower than what the compiler generates on its own ;(
//#undef USE_SSE2
#endif

IMPLEMENT_CLASS(UD3D11RenderDevice);

#include <initguid.h>
DEFINE_GUID(WKPDID_D3DDebugObjectName, 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00);

UD3D11RenderDevice::UD3D11RenderDevice()
{
}

void UD3D11RenderDevice::StaticConstructor()
{
	guard(UD3D11RenderDevice::StaticConstructor);

	SpanBased = 0;
	FullscreenOnly = 0;
	SupportsFogMaps = 1;
	SupportsDistanceFog = 0;
	SupportsTC = 1;
	SupportsLazyTextures = 0;
	PrefersDeferredLoad = 0;
	UseVSync = 1;
	AntialiasMode = 0;
	UsePrecache = 1;
	Coronas = 1;
	ShinySurfaces = 1;
#if !defined(UNREALGOLD)
	DetailTextures = 1;
#endif
	HighDetailActors = 1;
	VolumetricLighting = 1;

#if defined(OLDUNREAL469SDK)
	UseLightmapAtlas = 0; // Note: do not turn this on. It does not work and generates broken fogmaps.
	SupportsUpdateTextureRect = 1;
	SupportsDrawTileList = 1;
	MaxTextureSize = 4096;
	NeedsMaskedFonts = 0;
	DescFlags |= RDDESCF_Certified;
#endif

	GammaMode = 0;
	GammaOffset = 0.0f;
	GammaOffsetRed = 0.0f;
	GammaOffsetGreen = 0.0f;
	GammaOffsetBlue = 0.0f;

	LinearBrightness = 128; // 0.0f;
	Contrast = 128; // 1.0f;
	Saturation = 255; // 1.0f;
	GrayFormula = 1;

	Hdr = 0;
	HdrScale = 128;
#if !defined(OLDUNREAL469SDK)
	OccludeLines = 0;
#endif
	Bloom = 0;
	BloomAmount = 128;

	LODBias = 0.0f;
	LightMode = 0;
	RefreshRate = 0;

	GammaCorrectScreenshots = 1;
	UseDebugLayer = 0;

	UseVR = 0; // opt-in (VR users are rare). Once on, the Init probe auto-detects the headset (else mono).
	VRWorldScale = 52.5f;
	VRHudDepth = 150.0f;
	VRHudDepthBottom = 30.0f;
	VRHudBottomY = 0.7f;
	VRUIDepth = 150.0f;
	VRCrosshairDepth = 150.0f;
	VRResScale = 1.5f;
	VRHeightOffset = 0.0f;
	VRClearZBeforeHud = 1;
	VRLookMode = 0;
	VRHudScaleX = 1.0f;
	VRHudScaleY = 1.0f;
	VRWeaponIPDScale = 1.0f;
	VRBrightnessScale = 1.0f;
	VRBrightnessOffset = 0.0f;
	VRMirrorMode = 1; // 0=off, 1=when headset removed, 2=in menu, 3=always, 4=SBS record (full-res side-by-side backbuffer for game-capture recorders)
	VRScaleHudMeshes = 0;
	VRSBSHalf = 0;

#if defined(OLDUNREAL469SDK)
	new(GetClass(), TEXT("UseLightmapAtlas"), RF_Public) UBoolProperty(CPP_PROPERTY(UseLightmapAtlas), TEXT("Display"), CPF_Config);
#endif

	new(GetClass(), TEXT("UseVSync"), RF_Public) UBoolProperty(CPP_PROPERTY(UseVSync), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("UsePrecache"), RF_Public) UBoolProperty(CPP_PROPERTY(UsePrecache), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GammaCorrectScreenshots"), RF_Public) UBoolProperty(CPP_PROPERTY(GammaCorrectScreenshots), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("UseDebugLayer"), RF_Public) UBoolProperty(CPP_PROPERTY(UseDebugLayer), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("UseVR"), RF_Public) UBoolProperty(CPP_PROPERTY(UseVR), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRWorldScale"), RF_Public) UFloatProperty(CPP_PROPERTY(VRWorldScale), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHudDepth"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHudDepth), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHudDepthBottom"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHudDepthBottom), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHudBottomY"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHudBottomY), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRUIDepth"), RF_Public) UFloatProperty(CPP_PROPERTY(VRUIDepth), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRCrosshairDepth"), RF_Public) UFloatProperty(CPP_PROPERTY(VRCrosshairDepth), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRResScale"), RF_Public) UFloatProperty(CPP_PROPERTY(VRResScale), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHeightOffset"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHeightOffset), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRClearZBeforeHud"), RF_Public) UBoolProperty(CPP_PROPERTY(VRClearZBeforeHud), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRLookMode"), RF_Public) UByteProperty(CPP_PROPERTY(VRLookMode), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHudScaleX"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHudScaleX), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRHudScaleY"), RF_Public) UFloatProperty(CPP_PROPERTY(VRHudScaleY), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRWeaponIPDScale"), RF_Public) UFloatProperty(CPP_PROPERTY(VRWeaponIPDScale), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRBrightnessScale"), RF_Public) UFloatProperty(CPP_PROPERTY(VRBrightnessScale), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRBrightnessOffset"), RF_Public) UFloatProperty(CPP_PROPERTY(VRBrightnessOffset), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRMirrorMode"), RF_Public) UByteProperty(CPP_PROPERTY(VRMirrorMode), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRScaleHudMeshes"), RF_Public) UBoolProperty(CPP_PROPERTY(VRScaleHudMeshes), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("VRSBSHalf"), RF_Public) UBoolProperty(CPP_PROPERTY(VRSBSHalf), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GammaOffset"), RF_Public) UFloatProperty(CPP_PROPERTY(GammaOffset), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GammaOffsetRed"), RF_Public) UFloatProperty(CPP_PROPERTY(GammaOffsetRed), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GammaOffsetGreen"), RF_Public) UFloatProperty(CPP_PROPERTY(GammaOffsetGreen), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GammaOffsetBlue"), RF_Public) UFloatProperty(CPP_PROPERTY(GammaOffsetBlue), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("LinearBrightness"), RF_Public) UByteProperty(CPP_PROPERTY(LinearBrightness), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("Contrast"), RF_Public) UByteProperty(CPP_PROPERTY(Contrast), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("Saturation"), RF_Public) UByteProperty(CPP_PROPERTY(Saturation), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("GrayFormula"), RF_Public) UIntProperty(CPP_PROPERTY(GrayFormula), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("Hdr"), RF_Public) UBoolProperty(CPP_PROPERTY(Hdr), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("HdrScale"), RF_Public) UByteProperty(CPP_PROPERTY(HdrScale), TEXT("Display"), CPF_Config);
#if !defined(OLDUNREAL469SDK)
	new(GetClass(), TEXT("OccludeLines"), RF_Public) UBoolProperty(CPP_PROPERTY(OccludeLines), TEXT("Display"), CPF_Config);
#endif
	new(GetClass(), TEXT("Bloom"), RF_Public) UBoolProperty(CPP_PROPERTY(Bloom), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("BloomAmount"), RF_Public) UByteProperty(CPP_PROPERTY(BloomAmount), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("LODBias"), RF_Public) UFloatProperty(CPP_PROPERTY(LODBias), TEXT("Display"), CPF_Config);
	new(GetClass(), TEXT("RefreshRate"), RF_Public) UIntProperty(CPP_PROPERTY(RefreshRate), TEXT("Display"), CPF_Config);

	UEnum* AntialiasModes = new(GetClass(), TEXT("AntialiasModes"))UEnum(nullptr);
	new(AntialiasModes->Names)FName(TEXT("Off"));
	new(AntialiasModes->Names)FName(TEXT("MSAA_2x"));
	new(AntialiasModes->Names)FName(TEXT("MSAA_4x"));
	new(GetClass(), TEXT("AntialiasMode"), RF_Public) UByteProperty(CPP_PROPERTY(AntialiasMode), TEXT("Display"), CPF_Config, AntialiasModes);

	UEnum* GammaModes = new(GetClass(), TEXT("GammaModes"))UEnum(nullptr);
	new(GammaModes->Names)FName(TEXT("D3D9"));
	new(GammaModes->Names)FName(TEXT("XOpenGL"));
	new(GetClass(), TEXT("GammaMode"), RF_Public) UByteProperty(CPP_PROPERTY(GammaMode), TEXT("Display"), CPF_Config, GammaModes);

	UEnum* LightModes = new(GetClass(), TEXT("LightModes"))UEnum(nullptr);
	new(LightModes->Names)FName(TEXT("Normal"));
	new(LightModes->Names)FName(TEXT("OneXBlending"));
	new(LightModes->Names)FName(TEXT("BrighterActors"));
	new(GetClass(), TEXT("LightMode"), RF_Public) UByteProperty(CPP_PROPERTY(LightMode), TEXT("Display"), CPF_Config, LightModes);

	unguard;
}

int UD3D11RenderDevice::GetSettingsMultisample()
{
	switch (AntialiasMode)
	{
	default:
	case 0: return 0;
	case 1: return 2;
	case 2: return 4;
	}
}

UBOOL UD3D11RenderDevice::Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	guard(UD3D11RenderDevice::Init);

	Viewport = InViewport;
	ActiveHdr = Hdr;
	BufferCount = UseVSync ? 2 : 3;

	if (UseVR && !GIsEditor)
	{
		// Auto-detect: probe the OpenXR runtime now. Headset present -> run in VR;
		// otherwise clean up and run normally (mono). No config toggling needed.
		VR = CreateOpenXRBackend();
		uint64_t xrLuid = 0;
		if (VR && VR->QueryAdapterLuid(xrLuid))
		{
			// VR mode: size the scene buffers to hold a whole frame (mono streams
			// through tiny ones for FPS; the two-eye replay needs the whole frame).
			SceneVertexBufferCap = 1024 * 1024;
			SceneIndexBufferCap = 3 * 1024 * 1024;
		}
		else if (VR)
		{
			delete VR; // no headset -> stay mono, normal buffers/performance
			VR = nullptr;
		}
	}

	HDC screenDC = GetDC(0);
	DesktopResolution.Width = GetDeviceCaps(screenDC, HORZRES);
	DesktopResolution.Height = GetDeviceCaps(screenDC, VERTRES);
	ReleaseDC(0, screenDC);

	try
	{
		std::vector<D3D_FEATURE_LEVEL> featurelevels =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
		};

		UINT deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		if (UseDebugLayer)
			deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

		// First try use a more recent way of creating the device and swap chain
		HRESULT result = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			deviceFlags,
			featurelevels.data(), (UINT)featurelevels.size(),
			D3D11_SDK_VERSION,
			Device.TypedInitPtr(),
			&FeatureLevel,
			Context.TypedInitPtr());
		if (FAILED(result))
			debugf(TEXT("D3D11Drv: Could not create a modern D3D11 device"));

		// Wonderful API you got here, Microsoft. Good job.
		ComPtr<IDXGIDevice2> dxgiDevice;
		ComPtr<IDXGIAdapter> dxgiAdapter;
		ComPtr<IDXGIFactory2> dxgiFactory;

		if (SUCCEEDED(result))
			result = Device->QueryInterface(dxgiDevice.GetIID(), dxgiDevice.InitPtr());
		else
			debugf(TEXT("D3D11Drv: Could not get IDXGIDevice2 interface for the D3D11 device"));

		if (SUCCEEDED(result))
			result = dxgiDevice->GetParent(dxgiAdapter.GetIID(), dxgiAdapter.InitPtr());
		else
			debugf(TEXT("D3D11Drv: Could not get IDXGIAdapter interface for the D3D11 device"));

		if (SUCCEEDED(result))
			result = dxgiAdapter->GetParent(dxgiFactory.GetIID(), dxgiFactory.InitPtr());
		else
			debugf(TEXT("D3D11Drv: Could not get IDXGIFactory2 interface for the D3D11 device"));

		if (SUCCEEDED(result))
		{
			ComPtr<IDXGIFactory5> dxgiFactory5;
			result = dxgiFactory->QueryInterface(dxgiFactory5.GetIID(), dxgiFactory5.InitPtr());
			if (SUCCEEDED(result))
			{
				INT support = 0;
				result = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &support, sizeof(INT));
				if (SUCCEEDED(result))
				{
					DxgiSwapChainAllowTearing = support != 0;
				}
				else
				{
					debugf(TEXT("D3D11Drv: Device does not support DXGI_FEATURE_PRESENT_ALLOW_TEARING"));
				}
			}
			else
			{
				debugf(TEXT("D3D11Drv: Could not get IDXGIFactory5 interface for the D3D11 device"));
			}

			UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			if (DxgiSwapChainAllowTearing)
				swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

			DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
			swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapDesc.Width = NewX;
			swapDesc.Height = NewY;
			swapDesc.Format = ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
			swapDesc.BufferCount = BufferCount;
			swapDesc.SampleDesc.Count = 1;
			swapDesc.Scaling = DXGI_SCALING_STRETCH;
			swapDesc.SwapEffect = GIsEditor ? DXGI_SWAP_EFFECT_DISCARD : DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapDesc.Flags = swapChainFlags;
			swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			result = dxgiFactory->CreateSwapChainForHwnd(Device, (HWND)Viewport->GetWindow(), &swapDesc, nullptr, nullptr, SwapChain1.TypedInitPtr());
			if (SUCCEEDED(result))
			{
				dxgiFactory->MakeWindowAssociation((HWND)Viewport->GetWindow(), DXGI_MWA_NO_ALT_ENTER);
			}
			else
			{
				debugf(TEXT("D3D11Drv: CreateSwapChainForHwnd failed"));
				DxgiSwapChainAllowTearing = false;
			}
		}
		if (SUCCEEDED(result))
		{
			result = SwapChain1->QueryInterface(SwapChain.GetIID(), SwapChain.InitPtr());
			if (FAILED(result))
				SwapChain1.reset();
		}
		else
		{
			Context.reset();
			Device.reset();
		}
		dxgiFactory.reset();
		dxgiAdapter.reset();
		dxgiDevice.reset();

		// We still don't have a swap chain. Let's try the older Windows 7 API
		if (!SwapChain)
		{
			debugf(TEXT("D3D11Drv: Modern D3D11 device creation failed. Falling back to Windows 7"));

			DXGI_SWAP_CHAIN_DESC swapDesc = {};
			swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapDesc.BufferDesc.Width = NewX;
			swapDesc.BufferDesc.Height = NewY;
			swapDesc.BufferDesc.Format = ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
			swapDesc.BufferCount = BufferCount;
			swapDesc.SampleDesc.Count = 1;
			swapDesc.OutputWindow = (HWND)Viewport->GetWindow();
			swapDesc.Windowed = TRUE;
			swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			if (RefreshRate != 0)
			{
				swapDesc.BufferDesc.RefreshRate.Numerator = RefreshRate;
				swapDesc.BufferDesc.RefreshRate.Denominator = 1;
			}
			else
			{
				DEVMODE devmode = {};
				devmode.dmSize = sizeof(DEVMODE);
				if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devmode) && devmode.dmDisplayFrequency > 1)
				{
					swapDesc.BufferDesc.RefreshRate.Numerator = devmode.dmDisplayFrequency;
					swapDesc.BufferDesc.RefreshRate.Denominator = 1;
				}
			}

			// First try create a swap chain for Windows 8 and newer. If that fails, try the old for Windows 7
			HRESULT result = E_FAIL;
			for (DXGI_SWAP_EFFECT swapeffect : { GIsEditor ? DXGI_SWAP_EFFECT_DISCARD : DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_SWAP_EFFECT_DISCARD })
			{
				swapDesc.SwapEffect = swapeffect;

				result = D3D11CreateDeviceAndSwapChain(
					nullptr,
					D3D_DRIVER_TYPE_HARDWARE,
					0,
					deviceFlags,
					featurelevels.data(), (UINT)featurelevels.size(),
					D3D11_SDK_VERSION,
					&swapDesc,
					SwapChain.TypedInitPtr(),
					Device.TypedInitPtr(),
					&FeatureLevel,
					Context.TypedInitPtr());
				if (SUCCEEDED(result))
					break;

				debugf(TEXT("D3D11Drv: Could not use DXGI_SWAP_EFFECT_FLIP_DISCARD. Falling back to DXGI_SWAP_EFFECT_DISCARD"));
			}
			ThrowIfFailed(result, "D3D11CreateDeviceAndSwapChain failed");
		}

		if (UseDebugLayer)
		{
			result = Device->QueryInterface(DebugLayer.GetIID(), DebugLayer.InitPtr());
			if (SUCCEEDED(result))
			{
				result = DebugLayer->QueryInterface(InfoQueue.GetIID(), InfoQueue.InitPtr());
				if (SUCCEEDED(result))
				{
					std::initializer_list<D3D11_MESSAGE_ID> denyList =
					{
						D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
					};
					D3D11_INFO_QUEUE_FILTER filter = {};
					filter.DenyList.NumIDs = (UINT)denyList.size();
					filter.DenyList.pIDList = const_cast<D3D11_MESSAGE_ID*>(denyList.begin());
					result = InfoQueue->AddStorageFilterEntries(&filter);

					// result = InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
					// result = InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
				}
			}
			else
			{
				debugf(TEXT("D3D11Drv: Could not get ID3D11Debug interface"));
			}
		}

		SetDebugName(Device, "D3D11Drv.Device");
		SetDebugName(Context, "D3D11Drv.Context");

		SetColorSpace();

		CreateScenePass();
		CreatePresentPass();
		CreateBloomPass();

		Textures.reset(new TextureManager(this));
		Uploads.reset(new UploadManager(this));
	}
	catch (_com_error error)
	{
		debugf(TEXT("Could not create d3d11 renderer: [_com_error] %s"), error.ErrorMessage());
		Exit();
		return 0;
	}
	catch (const std::exception& e)
	{
		debugf(TEXT("Could not create d3d11 renderer: %s"), to_utf16(e.what()).c_str());
		Exit();
		return 0;
	}

	if (!SetRes(NewX, NewY, NewColorBytes, Fullscreen))
	{
		Exit();
		return 0;
	}

	StartVR(); // no-op unless the early probe found a headset

	return 1;
	unguard;
}

void UD3D11RenderDevice::StartVR()
{
	guard(UD3D11RenderDevice::StartVR);

	if (!VR) // no headset detected in the Init probe -> mono
		return;

	// ponytail: M0 assumes the default adapter is the one XR wants (true on single-GPU).
	// Multi-GPU: create the device on the probed LUID instead.
	if (!VR->Start(Device, VRResScale))
	{
		debugf(TEXT("D3D11Drv VR: session start failed, falling back to mono"));
		VR->Stop();
		delete VR;
		VR = nullptr;
		return;
	}

	// Depth test disabled, for the HUD range (painter's order, no z-fight).
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = FALSE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;
	Device->CreateDepthStencilState(&dsd, VRNoDepthState.TypedInitPtr());

	debugf(TEXT("D3D11Drv VR: active"));

	unguard;
}

void UD3D11RenderDevice::PrintDebugLayerMessages()
{
	if (InfoQueue)
	{
		UINT64 count = InfoQueue->GetNumStoredMessages();
		for (UINT64 i = 0; i < count; i++)
		{
			SIZE_T msgLength = 0;
			HRESULT result = InfoQueue->GetMessage(i, nullptr, &msgLength);
			if (msgLength >= sizeof(D3D11_MESSAGE))
			{
				std::vector<uint8_t> buffer(msgLength);
				D3D11_MESSAGE* msg = (D3D11_MESSAGE*)buffer.data();
				result = InfoQueue->GetMessage(i, msg, &msgLength);
				if (SUCCEEDED(result))
				{
					std::string description = msg->pDescription;
					bool found = SeenDebugMessages.find(description) != SeenDebugMessages.end();
					if (!found)
					{
						if (TotalSeenDebugMessages < 20)
						{
							TotalSeenDebugMessages++;
							SeenDebugMessages.insert(description);

							const char* severitystr = "unknown";
							switch (msg->Severity)
							{
							case D3D11_MESSAGE_SEVERITY_CORRUPTION: severitystr = "corruption"; break;
							case D3D11_MESSAGE_SEVERITY_ERROR: severitystr = "error"; break;
							case D3D11_MESSAGE_SEVERITY_WARNING: severitystr = "warning"; break;
							case D3D11_MESSAGE_SEVERITY_INFO: severitystr = "info"; break;
							case D3D11_MESSAGE_SEVERITY_MESSAGE: severitystr = "message"; break;
							}
							debugf(TEXT("D3D11Drv: [%s] %s"), to_utf16(severitystr).c_str(), to_utf16(msg->pDescription).c_str());
						}
					}
				}
			}
		}
		InfoQueue->ClearStoredMessages();
	}
}

void UD3D11RenderDevice::SetColorSpace()
{
	if (ActiveHdr)
	{
		ComPtr<IDXGISwapChain3> swapChain3;
		HRESULT result = SwapChain->QueryInterface(swapChain3.GetIID(), swapChain3.InitPtr());
		if (SUCCEEDED(result))
		{
			UINT support = 0;
			result = swapChain3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &support);
			if (SUCCEEDED(result) && (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
			{
				result = swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
				if (FAILED(result))
				{
					debugf(TEXT("D3D11Drv: IDXGISwapChain3.SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) failed"));
				}
			}
			else
			{
				debugf(TEXT("D3D11Drv: Swap chain does not support DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020"));
			}
		}
	}
}

class SetResCallLock
{
public:
	SetResCallLock(bool &value) : value(value)
	{
		value = true;
	}
	~SetResCallLock()
	{
		value = false;
	}
	bool& value;
};

UBOOL UD3D11RenderDevice::SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	guard(UD3D11RenderDevice::SetRes);

	debugf(TEXT("D3D11Drv: SetRes(%d, %d, %d, %d) called"), NewX, NewY, NewColorBytes, Fullscreen);

	if (InSetResCall)
	{
		debugf(TEXT("D3D11Drv: Ignoring recursive SetRes(%d, %d, %d, %d) call"), NewX, NewY, NewColorBytes, Fullscreen);
		return TRUE;
	}
	SetResCallLock lock(InSetResCall);

	ReleaseSwapChainResources();

	// Resize the swap chain buffers before doing the mode switch. Shouldn't really make any difference but you never know!
	if (Fullscreen)
	{
		UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		if (DxgiSwapChainAllowTearing)
			flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		SwapChain->ResizeBuffers(UseVSync ? 2 : 3, NewX, NewY, ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, flags);
	}

	HRESULT result;

	DXGI_MODE_DESC modeDesc = {};
	modeDesc.Width = NewX;
	modeDesc.Height = NewY;
	modeDesc.Format = ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
	if (RefreshRate != 0)
	{
		modeDesc.RefreshRate.Numerator = RefreshRate;
		modeDesc.RefreshRate.Denominator = 1;
	}
	else
	{
		DEVMODE devmode = {};
		devmode.dmSize = sizeof(DEVMODE);
		if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devmode) && devmode.dmDisplayFrequency > 1)
		{
			modeDesc.RefreshRate.Numerator = devmode.dmDisplayFrequency;
			modeDesc.RefreshRate.Denominator = 1;
		}
	}

	if (Fullscreen)
	{
		IDXGIOutput* output = nullptr;
		result = SwapChain->GetContainingOutput(&output);
		if (SUCCEEDED(result))
		{
			DXGI_MODE_DESC modeToMatch = modeDesc;
			DXGI_MODE_DESC modeFound = {};
			result = output->FindClosestMatchingMode(&modeToMatch, &modeFound, Device);
			if (SUCCEEDED(result))
			{
				if (modeToMatch.Width == modeFound.Width && modeToMatch.Height == modeFound.Height)
				{
					modeDesc = modeFound;
				}
				else
				{
					debugf(TEXT("FindClosestMatchingMode could not find a mode with the specified resolution (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
				}
			}
			else
			{
				debugf(TEXT("FindClosestMatchingMode failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
			}
			NewX = modeDesc.Width;
			NewY = modeDesc.Height;
			output->Release();
		}
		else
		{
			debugf(TEXT("GetContainingOutput failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
		}

		debugf(TEXT("D3D11Drv: Calling Viewport->ResizeViewport(BLIT_Fullscreen | BLIT_Direct3D, %d, %d, %d)"), NewX, NewY, NewColorBytes);

		// If we are going fullscreen we want to resize the window *prior* to entering fullscreen.
		if (!Viewport->ResizeViewport(BLIT_Fullscreen | BLIT_Direct3D, NewX, NewY, NewColorBytes))
		{
			debugf(TEXT("Viewport.ResizeViewport failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
			return FALSE;
		}

		result = SwapChain->SetFullscreenState(TRUE, nullptr);
		if (FAILED(result))
		{
			debugf(TEXT("SwapChain.SetFullscreenState failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
			// Don't fail this as it can happen if the application isn't the foreground process
		}

		result = SwapChain->ResizeTarget(&modeDesc);
		if (FAILED(result))
		{
			debugf(TEXT("SwapChain.ResizeTarget failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
		}
	}
	else
	{
		if (CurrentFullscreen)
		{
			result = SwapChain->SetFullscreenState(FALSE, nullptr);
			if (FAILED(result))
			{
				debugf(TEXT("SwapChain.SetFullscreenState failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
				// Don't fail this as it can happen if the application isn't the foreground process
			}

			result = SwapChain->ResizeTarget(&modeDesc);
			if (FAILED(result))
			{
				debugf(TEXT("SwapChain.ResizeTarget failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
			}
		}

		debugf(TEXT("D3D11Drv: Calling Viewport->ResizeViewport(BLIT_HardwarePaint | BLIT_Direct3D, %d, %d, %d)"), NewX, NewY, NewColorBytes);

		// If we exiting fullscreen, we want to resize/reposition the window *after* exiting fullscreen
		if (!Viewport->ResizeViewport(BLIT_HardwarePaint | BLIT_Direct3D, NewX, NewY, NewColorBytes))
		{
			debugf(TEXT("Viewport.ResizeViewport failed (%d, %d, %d, %d)"), NewX, NewY, NewColorBytes, (INT)Fullscreen);
			return FALSE;
		}
	}

	CurrentSizeX = NewX;
	CurrentSizeY = NewY;
	CurrentFullscreen = Fullscreen;

	BufferCount = UseVSync ? 2 : 3;
	if (!UpdateSwapChain())
		return FALSE;

	SaveConfig();

#if defined(UNREALGOLD)
	Flush();
#else
	Flush(1);
#endif

	return 1;
	unguard;
}

void UD3D11RenderDevice::ReleaseSwapChainResources()
{
	BackBuffer.reset();
	BackBufferView.reset();
}

bool UD3D11RenderDevice::UpdateSwapChain(bool resizeSceneBuffers)
{
	UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	if (DxgiSwapChainAllowTearing)
		flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	// SBS record mode (VRMirrorMode 4): size the backbuffer to both eye screen-crops side by
	// side at full eye resolution, independent of the window — DXGI_SCALING_STRETCH shrinks it
	// into the window for preview, while game recorders that hook Present (Bandicam game mode,
	// OBS game capture) grab the backbuffer at its native size, giving a full-res side-by-side
	// 3D video. The wanted size comes from RenderVREyes (the crop depends on the eye projection;
	// zero until the first VR frame). Not with HDR: the SBS blit is a raw copy from the RGBA8
	// eye images, incompatible with an RGBA16F backbuffer (mode 4 then falls back to a plain
	// always-on mirror).
	BackBufferSizeX = CurrentSizeX;
	BackBufferSizeY = CurrentSizeY;
	if (VR && VRMirrorMode == 4 && !ActiveHdr && VRSBSSizeX > 0 && VRSBSSizeY > 0)
	{
		BackBufferSizeX = VRSBSSizeX;
		BackBufferSizeY = VRSBSSizeY;
	}

	debugf(TEXT("D3D11Drv: Updating SwapChain size to %d x %d"), BackBufferSizeX, BackBufferSizeY);

	HRESULT result = SwapChain->ResizeBuffers(BufferCount, BackBufferSizeX, BackBufferSizeY, ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, flags);
	if (FAILED(result))
	{
		return false;
	}

	SetColorSpace();

	if (resizeSceneBuffers && CurrentSizeX && CurrentSizeY)
	{
		try
		{
			debugf(TEXT("D3D11Drv: Resizing scene buffers to %d x %d"), CurrentSizeX, CurrentSizeY);

			ResizeSceneBuffers(CurrentSizeX, CurrentSizeY, GetSettingsMultisample());
		}
		catch (const std::exception& e)
		{
			debugf(TEXT("Could not resize scene buffers: %s"), to_utf16(e.what()).c_str());
			return false;
		}
	}

	result = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (FAILED(result))
		return false;
	SetDebugName(BackBuffer, "BackBuffer");

	result = Device->CreateRenderTargetView(BackBuffer, nullptr, BackBufferView.TypedInitPtr());
	if (FAILED(result))
		return false;
	SetDebugName(BackBufferView, "BackBufferView");

	return true;
}

void UD3D11RenderDevice::Exit()
{
	guard(UD3D11RenderDevice::Exit);

	debugf(TEXT("D3D11Drv: exit called"));

	if (VR)
	{
		VR->Stop();
		delete VR;
		VR = nullptr;
	}

	UnmapVertices();

	ReleaseSwapChainResources();
	if (CurrentFullscreen && SwapChain)
		SwapChain->SetFullscreenState(FALSE, nullptr);

	PrintDebugLayerMessages();

	if (Context)
		Context->ClearState();

	Uploads.reset();
	Textures.reset();
	ReleasePresentPass();
	ReleaseBloomPass();
	ReleaseScenePass();
	ReleaseSceneBuffers();
	BackBufferView.reset();
	BackBuffer.reset();
	SwapChain.reset();
	SwapChain1.reset();
	Context.reset();

	if (DebugLayer)
	{
		DebugLayer->ReportLiveDeviceObjects(/*D3D11_RLDO_SUMMARY |*/ D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		PrintDebugLayerMessages();
	}

	InfoQueue.reset();
	DebugLayer.reset();

	if (Device)
	{
		Device->AddRef();
		int count = Device->Release();
		Device.reset();
		debugf(TEXT("D3D11Drv: D3D11Drv.Device refcount is now %d"), count - 1);
	}

	unguard;
}

void UD3D11RenderDevice::ResizeSceneBuffers(int width, int height, int multisample)
{
	multisample = std::max(multisample, 1);

	if (SceneBuffers.Width == width && SceneBuffers.Height == height && multisample == SceneBuffers.Multisample && SceneBuffers.ColorBuffer && SceneBuffers.HitBuffer && SceneBuffers.PPHitBuffer && SceneBuffers.StagingHitBuffer && SceneBuffers.DepthBuffer && SceneBuffers.PPImage[0] && SceneBuffers.PPImage[1])
		return;

	SceneBuffers.ColorBufferView.reset();
	SceneBuffers.HitBufferView.reset();
	SceneBuffers.HitBufferShaderView.reset();
	SceneBuffers.PPHitBufferView.reset();
	SceneBuffers.DepthBufferView.reset();
	SceneBuffers.MirrorDepthBufferView.reset();
	for (int i = 0; i < 2; i++)
	{
		SceneBuffers.PPImageShaderView[i].reset();
		SceneBuffers.PPImageView[i].reset();
		SceneBuffers.PPImage[i].reset();
	}
	SceneBuffers.ColorBuffer.reset();
	SceneBuffers.StagingHitBuffer.reset();
	SceneBuffers.PPHitBuffer.reset();
	SceneBuffers.HitBuffer.reset();
	SceneBuffers.DepthBuffer.reset();
	SceneBuffers.MirrorDepthBuffer.reset();

	for (PPBlurLevel& level : SceneBuffers.BlurLevels)
	{
		level.VTexture.reset();
		level.VTextureRTV.reset();
		level.VTextureSRV.reset();
		level.HTexture.reset();
		level.HTextureRTV.reset();
		level.HTextureSRV.reset();
	}

	SceneBuffers.Width = width;
	SceneBuffers.Height = height;
	SceneBuffers.Multisample = multisample;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = SceneBuffers.Multisample;
	texDesc.SampleDesc.Quality = SceneBuffers.Multisample > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	HRESULT result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.ColorBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(ColorBuffer) failed");
	SetDebugName(SceneBuffers.ColorBuffer, "SceneBuffers.ColorBuffer");

	texDesc = {};
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_UINT;
	texDesc.SampleDesc.Count = SceneBuffers.Multisample;
	texDesc.SampleDesc.Quality = SceneBuffers.Multisample > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.HitBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(HitBuffer) failed");
	SetDebugName(SceneBuffers.HitBuffer, "SceneBuffers.HitBuffer");

	texDesc = {};
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_UINT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.PPHitBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(PPHitBuffer) failed");
	SetDebugName(SceneBuffers.PPHitBuffer, "SceneBuffers.PPHitBuffer");

	texDesc = {};
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.BindFlags = 0;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_UINT;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.StagingHitBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(StagingHitBuffer) failed");
	SetDebugName(SceneBuffers.StagingHitBuffer, "SceneBuffers.StagingHitBuffer");

	texDesc = {};
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_D32_FLOAT;
	texDesc.SampleDesc.Count = SceneBuffers.Multisample;
	texDesc.SampleDesc.Quality = SceneBuffers.Multisample > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.DepthBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(DepthBuffer) failed");
	SetDebugName(SceneBuffers.DepthBuffer, "SceneBuffers.DepthBuffer");

	// 1-sample depth for the VR mirror (rendered without MSAA straight into PPImage[0]).
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.MirrorDepthBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(MirrorDepthBuffer) failed");
	SetDebugName(SceneBuffers.MirrorDepthBuffer, "SceneBuffers.MirrorDepthBuffer");

	for (int i = 0; i < 2; i++)
	{
		texDesc = {};
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.Width = SceneBuffers.Width;
		texDesc.Height = SceneBuffers.Height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		result = Device->CreateTexture2D(&texDesc, nullptr, SceneBuffers.PPImage[i].TypedInitPtr());
		ThrowIfFailed(result, "CreateTexture2D(PPImage) failed");
		SetDebugName(SceneBuffers.PPImage[i], "SceneBuffers.PPImage");
	}

	result = Device->CreateRenderTargetView(SceneBuffers.ColorBuffer, nullptr, SceneBuffers.ColorBufferView.TypedInitPtr());
	ThrowIfFailed(result, "CreateRenderTargetView(ColorBuffer) failed");
	SetDebugName(SceneBuffers.ColorBufferView, "SceneBuffers.ColorBufferView");

	result = Device->CreateRenderTargetView(SceneBuffers.HitBuffer, nullptr, SceneBuffers.HitBufferView.TypedInitPtr());
	ThrowIfFailed(result, "CreateRenderTargetView(HitBuffer) failed");
	SetDebugName(SceneBuffers.HitBufferView, "SceneBuffers.HitBufferView");

	result = Device->CreateShaderResourceView(SceneBuffers.HitBuffer, nullptr, SceneBuffers.HitBufferShaderView.TypedInitPtr());
	ThrowIfFailed(result, "CreateShaderResourceView(HitBuffer) failed");
	SetDebugName(SceneBuffers.HitBufferShaderView, "SceneBuffers.HitBufferShaderView");

	result = Device->CreateRenderTargetView(SceneBuffers.PPHitBuffer, nullptr, SceneBuffers.PPHitBufferView.TypedInitPtr());
	ThrowIfFailed(result, "CreateRenderTargetView(PPHitBuffer) failed");
	SetDebugName(SceneBuffers.PPHitBufferView, "SceneBuffers.PPHitBufferView");

	result = Device->CreateDepthStencilView(SceneBuffers.DepthBuffer, nullptr, SceneBuffers.DepthBufferView.TypedInitPtr());
	ThrowIfFailed(result, "CreateDepthStencilView(DepthBuffer) failed");
	SetDebugName(SceneBuffers.DepthBufferView, "SceneBuffers.DepthBufferView");

	result = Device->CreateDepthStencilView(SceneBuffers.MirrorDepthBuffer, nullptr, SceneBuffers.MirrorDepthBufferView.TypedInitPtr());
	ThrowIfFailed(result, "CreateDepthStencilView(MirrorDepthBuffer) failed");
	SetDebugName(SceneBuffers.MirrorDepthBufferView, "SceneBuffers.MirrorDepthBufferView");

	for (int i = 0; i < 2; i++)
	{
		result = Device->CreateRenderTargetView(SceneBuffers.PPImage[i], nullptr, SceneBuffers.PPImageView[i].TypedInitPtr());
		ThrowIfFailed(result, "CreateRenderTargetView(PPImage) failed");
		SetDebugName(SceneBuffers.PPImageView[i], "SceneBuffers.PPImageView");

		result = Device->CreateShaderResourceView(SceneBuffers.PPImage[i], nullptr, SceneBuffers.PPImageShaderView[i].TypedInitPtr());
		ThrowIfFailed(result, "CreateShaderResourceView(PPImage) failed");
		SetDebugName(SceneBuffers.PPImageShaderView[i], "SceneBuffers.PPImageShaderView");
	}

	int bloomWidth = width;
	int bloomHeight = height;
	for (PPBlurLevel& level : SceneBuffers.BlurLevels)
	{
		bloomWidth = (bloomWidth + 1) / 2;
		bloomHeight = (bloomHeight + 1) / 2;

		texDesc = {};
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.Width = bloomWidth;
		texDesc.Height = bloomHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		result = Device->CreateTexture2D(&texDesc, nullptr, level.VTexture.TypedInitPtr());
		ThrowIfFailed(result, "CreateTexture2D(SceneBuffers.BlurLevels.VTexture) failed");
		SetDebugName(level.VTexture, "SceneBuffers.BlurLevels.VTexture");

		result = Device->CreateTexture2D(&texDesc, nullptr, level.HTexture.TypedInitPtr());
		ThrowIfFailed(result, "CreateTexture2D(SceneBuffers.BlurLevels.HTexture) failed");
		SetDebugName(level.HTexture, "SceneBuffers.BlurLevels.HTexture");

		result = Device->CreateRenderTargetView(level.VTexture, nullptr, level.VTextureRTV.TypedInitPtr());
		ThrowIfFailed(result, "CreateRenderTargetView(SceneBuffers.BlurLevels.VTextureRTV) failed");
		SetDebugName(level.VTextureRTV, "SceneBuffers.BlurLevels.VTextureRTV");

		result = Device->CreateRenderTargetView(level.HTexture, nullptr, level.HTextureRTV.TypedInitPtr());
		ThrowIfFailed(result, "CreateRenderTargetView(SceneBuffers.BlurLevels.HTextureRTV) failed");
		SetDebugName(level.HTextureRTV, "SceneBuffers.BlurLevels.HTextureRTV");

		result = Device->CreateShaderResourceView(level.VTexture, nullptr, level.VTextureSRV.TypedInitPtr());
		ThrowIfFailed(result, "CreateRenderTargetView(SceneBuffers.BlurLevels.VTextureSRV) failed");
		SetDebugName(level.VTextureSRV, "SceneBuffers.BlurLevels.VTextureSRV");

		result = Device->CreateShaderResourceView(level.HTexture, nullptr, level.HTextureSRV.TypedInitPtr());
		ThrowIfFailed(result, "CreateRenderTargetView(SceneBuffers.BlurLevels.HTextureSRV) failed");
		SetDebugName(level.HTextureSRV, "SceneBuffers.BlurLevels.HTextureSRV");

		level.Width = bloomWidth;
		level.Height = bloomHeight;
	}
}

void UD3D11RenderDevice::CreateScenePass()
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> elements =
	{
		{ "AttrFlags", 0, DXGI_FORMAT_R32_UINT, 0, offsetof(SceneVertex, Flags), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrPos", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(SceneVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrTexCoordOne", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SceneVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrTexCoordTwo", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SceneVertex, TexCoord2), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrTexCoordThree", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SceneVertex, TexCoord3), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrTexCoordFour", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SceneVertex, TexCoord4), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AttrColor", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SceneVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	CreateVertexShader(ScenePass.VertexShader, "ScenePass.VertexShader", ScenePass.InputLayout, "ScenePass.InputLayout", elements, "shaders/Scene.vert");
	CreatePixelShader(ScenePass.PixelShader, "ScenePass.PixelShader", "shaders/Scene.frag");
	CreatePixelShader(ScenePass.PixelShaderAlphaTest, "ScenePass.PixelShaderAlphaTest", "shaders/Scene.frag", { "ALPHATEST" });

	CreateSceneSamplers();

	for (int i = 0; i < 2; i++)
	{
		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthClipEnable = FALSE; // Avoid clipping the weapon. The UE1 engine clips the geometry anyway.
		rasterizerDesc.MultisampleEnable = i == 1 ? TRUE : FALSE;
		HRESULT result = Device->CreateRasterizerState(&rasterizerDesc, ScenePass.RasterizerState[i].TypedInitPtr());
		ThrowIfFailed(result, "CreateRasterizerState(ScenePass.Pipelines.RasterizerState) failed");
		SetDebugName(ScenePass.RasterizerState[i], "ScenePass.RasterizerState");
	}

	for (int i = 0; i < 32; i++)
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		switch (i & 3)
		{
		case 0: // PF_Translucent
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case 1: // PF_Modulated
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			break;
		case 2: // PF_Highlighted
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case 3: // Hmm, is it faster to keep the blend mode enabled or to toggle it?
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			break;
		}
		if (i & 4) // PF_Invisible
			blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
		else
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[1].BlendEnable = FALSE;
		blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HRESULT result = Device->CreateBlendState(&blendDesc, ScenePass.Pipelines[i].BlendState.TypedInitPtr());
		ThrowIfFailed(result, "CreateBlendState(ScenePass.Pipelines.BlendState) failed");
		SetDebugName(ScenePass.Pipelines[i].BlendState, "ScenePass.Pipelines.BlendState");

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		if (i & 8) // PF_Occlude
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		else
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		result = Device->CreateDepthStencilState(&depthStencilDesc, ScenePass.Pipelines[i].DepthStencilState.TypedInitPtr());
		ThrowIfFailed(result, "CreateDepthStencilState(ScenePass.Pipelines.DepthStencilState) failed");
		SetDebugName(ScenePass.Pipelines[i].DepthStencilState, "ScenePass.Pipelines.DepthStencilState");

		if (i & 16) // PF_Masked
			ScenePass.Pipelines[i].PixelShader = ScenePass.PixelShaderAlphaTest;
		else
			ScenePass.Pipelines[i].PixelShader = ScenePass.PixelShader;

		ScenePass.Pipelines[i].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	// Line pipeline
	for (int i = 0; i < 2; i++)
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[1].BlendEnable = FALSE;
		blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HRESULT result = Device->CreateBlendState(&blendDesc, ScenePass.LinePipeline[i].BlendState.TypedInitPtr());
		ThrowIfFailed(result, "CreateBlendState(ScenePass.LinePipeline.BlendState) failed");
		SetDebugName(ScenePass.LinePipeline[i].BlendState, "ScenePass.LinePipeline.BlendState");

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		result = Device->CreateDepthStencilState(&depthStencilDesc, ScenePass.LinePipeline[i].DepthStencilState.TypedInitPtr());
		ThrowIfFailed(result, "CreateDepthStencilState(ScenePass.LinePipeline.DepthStencilState) failed");
		SetDebugName(ScenePass.LinePipeline[i].DepthStencilState, "ScenePass.LinePipeline.DepthStencilState");

		ScenePass.LinePipeline[i].PixelShader = ScenePass.PixelShader;
		ScenePass.LinePipeline[i].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

		if (i == 0)
		{
			ScenePass.LinePipeline[i].MinDepth = 0.0f;
			ScenePass.LinePipeline[i].MaxDepth = 0.1f;
		}
	}

	// Point pipeline
	for (int i = 0; i < 2; i++)
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[1].BlendEnable = FALSE;
		blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HRESULT result = Device->CreateBlendState(&blendDesc, ScenePass.PointPipeline[i].BlendState.TypedInitPtr());
		ThrowIfFailed(result, "CreateBlendState(ScenePass.LinePipeline.BlendState) failed");
		SetDebugName(ScenePass.PointPipeline[i].BlendState, "ScenePass.PointPipeline.BlendState");

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		result = Device->CreateDepthStencilState(&depthStencilDesc, ScenePass.PointPipeline[i].DepthStencilState.TypedInitPtr());
		ThrowIfFailed(result, "CreateDepthStencilState(ScenePass.PointPipeline.DepthStencilState) failed");
		SetDebugName(ScenePass.PointPipeline[i].DepthStencilState, "ScenePass.PointPipeline.DepthStencilState");

		ScenePass.PointPipeline[i].PixelShader = ScenePass.PixelShader;
		ScenePass.PointPipeline[i].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		if (i == 0)
		{
			ScenePass.PointPipeline[i].MinDepth = 0.0f;
			ScenePass.PointPipeline[i].MaxDepth = 0.1f;
		}
	}

	// Allocate the actual buffers at the runtime cap (mono = the tuned const, VR =
	// the larger cap). Must match what ReserveVertices bounds against, or writes
	// run past the mapped buffer.
	D3D11_BUFFER_DESC bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.ByteWidth = SceneVertexBufferCap * sizeof(SceneVertex);
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT result = Device->CreateBuffer(&bufDesc, nullptr, ScenePass.VertexBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(ScenePass.VertexBuffer) failed");
	SetDebugName(ScenePass.VertexBuffer, "ScenePass.VertexBuffer");

	bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.ByteWidth = SceneIndexBufferCap * sizeof(uint32_t);
	bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	result = Device->CreateBuffer(&bufDesc, nullptr, ScenePass.IndexBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(ScenePass.IndexBuffer) failed");
	SetDebugName(ScenePass.IndexBuffer, "ScenePass.IndexBuffer");

	bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.ByteWidth = sizeof(ScenePushConstants);
	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	result = Device->CreateBuffer(&bufDesc, nullptr, ScenePass.ConstantBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(ScenePass.ConstantBuffer) failed");
	SetDebugName(ScenePass.ConstantBuffer, "ScenePass.ConstantBuffer");
}

void UD3D11RenderDevice::CreateSceneSamplers()
{
	for (int i = 0; i < 16; i++)
	{
		int dummyMipmapCount = (i >> 2) & 3;
		D3D11_FILTER filter = (i & 1) ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_ANISOTROPIC;
		D3D11_TEXTURE_ADDRESS_MODE addressmode = (i & 2) ? D3D11_TEXTURE_ADDRESS_MIRROR_ONCE : D3D11_TEXTURE_ADDRESS_WRAP;
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.MinLOD = dummyMipmapCount;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 1.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.MaxAnisotropy = 8.0f;
		samplerDesc.MipLODBias = (float)dummyMipmapCount + LODBias;
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = addressmode;
		samplerDesc.AddressV = addressmode;
		samplerDesc.AddressW = addressmode;
		HRESULT result = Device->CreateSamplerState(&samplerDesc, ScenePass.Samplers[i].TypedInitPtr());
		ThrowIfFailed(result, "CreateSamplerState(ScenePass.Samplers) failed");
		SetDebugName(ScenePass.Samplers[i], "ScenePass.Samplers");
	}

	ScenePass.LODBias = LODBias;
}

void UD3D11RenderDevice::ReleaseSceneSamplers()
{
	for (auto& sampler : ScenePass.Samplers)
	{
		sampler.reset();
	}
	ScenePass.LODBias = 0.0f;
}

void UD3D11RenderDevice::UpdateLODBias()
{
	if (ScenePass.LODBias != LODBias)
	{
		ReleaseSceneSamplers();
		CreateSceneSamplers();
	}
}

void UD3D11RenderDevice::ReleaseScenePass()
{
	ScenePass.VertexShader.reset();
	ScenePass.InputLayout.reset();
	ScenePass.VertexBuffer.reset();
	ScenePass.IndexBuffer.reset();
	ScenePass.ConstantBuffer.reset();
	ScenePass.RasterizerState[0].reset();
	ScenePass.RasterizerState[1].reset();
	ScenePass.PixelShader.reset();
	ScenePass.PixelShaderAlphaTest.reset();
	ReleaseSceneSamplers();
	for (auto& pipeline : ScenePass.Pipelines)
	{
		pipeline.BlendState.reset();
		pipeline.DepthStencilState.reset();
	}
	for (int i = 0; i < 2; i++)
	{
		ScenePass.LinePipeline[i].BlendState.reset();
		ScenePass.LinePipeline[i].DepthStencilState.reset();
		ScenePass.PointPipeline[i].BlendState.reset();
		ScenePass.PointPipeline[i].DepthStencilState.reset();
	}
}

void UD3D11RenderDevice::ReleaseBloomPass()
{
	BloomPass.Extract.reset();
	BloomPass.Combine.reset();
	BloomPass.BlurVertical.reset();
	BloomPass.BlurHorizontal.reset();
	BloomPass.ConstantBuffer.reset();
	BloomPass.AdditiveBlendState.reset();
}

void UD3D11RenderDevice::ReleasePresentPass()
{
	PresentPass.PPStepLayout.reset();
	PresentPass.PPStep.reset();
	PresentPass.PPStepVertexBuffer.reset();
	PresentPass.HitResolve.reset();
	for (auto& shader : PresentPass.Present) shader.reset();
	PresentPass.PresentConstantBuffer.reset();
	PresentPass.DitherTextureView.reset();
	PresentPass.DitherTexture.reset();
	PresentPass.BlendState.reset();
	PresentPass.DepthStencilState.reset();
	PresentPass.RasterizerState.reset();
}

void UD3D11RenderDevice::ReleaseSceneBuffers()
{
	SceneBuffers.ColorBufferView.reset();
	SceneBuffers.HitBufferView.reset();
	SceneBuffers.HitBufferShaderView.reset();
	SceneBuffers.PPHitBufferView.reset();
	SceneBuffers.DepthBufferView.reset();
	SceneBuffers.MirrorDepthBufferView.reset();
	for (int i = 0; i < 2; i++)
	{
		SceneBuffers.PPImageShaderView[i].reset();
		SceneBuffers.PPImageView[i].reset();
		SceneBuffers.PPImage[i].reset();
	}
	SceneBuffers.ColorBuffer.reset();
	SceneBuffers.StagingHitBuffer.reset();
	SceneBuffers.PPHitBuffer.reset();
	SceneBuffers.HitBuffer.reset();
	SceneBuffers.DepthBuffer.reset();
	SceneBuffers.MirrorDepthBuffer.reset();
	for (PPBlurLevel& level : SceneBuffers.BlurLevels)
	{
		level.VTexture.reset();
		level.VTextureRTV.reset();
		level.VTextureSRV.reset();
		level.HTexture.reset();
		level.HTextureRTV.reset();
		level.HTextureSRV.reset();
	}
}

UD3D11RenderDevice::ScenePipelineState* UD3D11RenderDevice::GetPipeline(DWORD PolyFlags)
{
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

	return &ScenePass.Pipelines[index];
}

void UD3D11RenderDevice::RunBloomPass()
{
	ID3D11RenderTargetView* rtvs[1] = {};
	ID3D11ShaderResourceView* srvs[1] = {};

	float blurAmount = 0.6f + BloomAmount * (1.9f / 255.0f);
	BloomPushConstants pushconstants;
	ComputeBlurSamples(7, blurAmount, pushconstants.SampleWeights);

	ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get() };
	ID3D11Buffer* cbs[1] = { BloomPass.ConstantBuffer.get() };
	UINT stride = sizeof(vec2);
	UINT offset = 0;
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetInputLayout(PresentPass.PPStepLayout);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
	Context->RSSetState(PresentPass.RasterizerState);
	Context->PSSetConstantBuffers(0, 1, cbs);
	Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
	Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);
	Context->UpdateSubresource(BloomPass.ConstantBuffer, 0, nullptr, &pushconstants, 0, 0);

	D3D11_VIEWPORT viewport = {};
	viewport.MaxDepth = 1.0f;

	// Extract overbright pixels that we want to bloom:
	viewport.Width = SceneBuffers.BlurLevels[0].Width;
	viewport.Height = SceneBuffers.BlurLevels[0].Height;
	rtvs[0] = SceneBuffers.BlurLevels[0].VTextureRTV.get();
	srvs[0] = SceneBuffers.PPImageShaderView[0].get();
	Context->OMSetRenderTargets(1, rtvs, nullptr);
	Context->RSSetViewports(1, &viewport);
	Context->PSSetShader(BloomPass.Extract, nullptr, 0);
	Context->PSSetShaderResources(0, 1, srvs);
	Context->Draw(6, 0);

	// Blur and downscale:
	for (int i = 0; i < SceneBuffers.NumBloomLevels - 1; i++)
	{
		auto& blevel = SceneBuffers.BlurLevels[i];
		auto& next = SceneBuffers.BlurLevels[i + 1];

		viewport.Width = blevel.Width;
		viewport.Height = blevel.Height;
		Context->RSSetViewports(1, &viewport);
		BlurStep(blevel.VTextureSRV, blevel.HTextureRTV, false);
		BlurStep(blevel.HTextureSRV, blevel.VTextureRTV, true);

		// Linear downscale:
		viewport.Width = next.Width;
		viewport.Height = next.Height;
		rtvs[0] = next.VTextureRTV.get();
		srvs[0] = blevel.VTextureSRV.get();
		Context->OMSetRenderTargets(1, rtvs, nullptr);
		Context->RSSetViewports(1, &viewport);
		Context->PSSetShader(BloomPass.Combine, nullptr, 0);
		Context->PSSetShaderResources(0, 1, srvs);
		Context->Draw(6, 0);
	}

	// Blur and upscale:
	for (int i = SceneBuffers.NumBloomLevels - 1; i > 0; i--)
	{
		auto& blevel = SceneBuffers.BlurLevels[i];
		auto& next = SceneBuffers.BlurLevels[i - 1];

		viewport.Width = blevel.Width;
		viewport.Height = blevel.Height;
		Context->RSSetViewports(1, &viewport);
		BlurStep(blevel.VTextureSRV, blevel.HTextureRTV, false);
		BlurStep(blevel.HTextureSRV, blevel.VTextureRTV, true);

		// Linear upscale:
		viewport.Width = next.Width;
		viewport.Height = next.Height;
		rtvs[0] = next.VTextureRTV.get();
		srvs[0] = blevel.VTextureSRV.get();
		Context->OMSetRenderTargets(1, rtvs, nullptr);
		Context->RSSetViewports(1, &viewport);
		Context->PSSetShader(BloomPass.Combine, nullptr, 0);
		Context->PSSetShaderResources(0, 1, srvs);
		Context->Draw(6, 0);
	}

	viewport.Width = SceneBuffers.BlurLevels[0].Width;
	viewport.Height = SceneBuffers.BlurLevels[0].Height;
	Context->RSSetViewports(1, &viewport);
	BlurStep(SceneBuffers.BlurLevels[0].VTextureSRV, SceneBuffers.BlurLevels[0].HTextureRTV, false);
	BlurStep(SceneBuffers.BlurLevels[0].HTextureSRV, SceneBuffers.BlurLevels[0].VTextureRTV, true);

	// Add bloom back to scene post process texture:
	viewport.Width = SceneBuffers.Width;
	viewport.Height = SceneBuffers.Height;
	rtvs[0] = SceneBuffers.PPImageView[0].get();
	srvs[0] = SceneBuffers.BlurLevels[0].VTextureSRV.get();
	Context->OMSetRenderTargets(1, rtvs, nullptr);
	Context->OMSetBlendState(BloomPass.AdditiveBlendState, nullptr, 0xffffffff);
	Context->RSSetViewports(1, &viewport);
	Context->PSSetShader(BloomPass.Combine, nullptr, 0);
	Context->PSSetShaderResources(0, 1, srvs);
	Context->Draw(6, 0);
}

void UD3D11RenderDevice::BlurStep(ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, bool vertical)
{
	Context->OMSetRenderTargets(1, &output, nullptr);
	Context->PSSetShader(vertical ? BloomPass.BlurVertical : BloomPass.BlurHorizontal, nullptr, 0);
	Context->PSSetShaderResources(0, 1, &input);
	Context->Draw(6, 0);
}

float UD3D11RenderDevice::ComputeBlurGaussian(float n, float theta) // theta = Blur Amount
{
	return (float)((1.0f / std::sqrtf(2 * 3.14159265359f * theta)) * std::expf(-(n * n) / (2.0f * theta * theta)));
}

void UD3D11RenderDevice::ComputeBlurSamples(int sampleCount, float blurAmount, float* sampleWeights)
{
	sampleWeights[0] = ComputeBlurGaussian(0, blurAmount);

	float totalWeights = sampleWeights[0];

	for (int i = 0; i < sampleCount / 2; i++)
	{
		float weight = ComputeBlurGaussian(i + 1.0f, blurAmount);

		sampleWeights[i * 2 + 1] = weight;
		sampleWeights[i * 2 + 2] = weight;

		totalWeights += weight * 2;
	}

	for (int i = 0; i < sampleCount; i++)
	{
		sampleWeights[i] /= totalWeights;
	}
}

void UD3D11RenderDevice::CreateBloomPass()
{
	CreatePixelShader(BloomPass.Extract, "BloomPass.Extract", "shaders/BloomExtract.frag");
	CreatePixelShader(BloomPass.Combine, "BloomPass.Combine", "shaders/BloomCombine.frag");
	CreatePixelShader(BloomPass.BlurVertical, "BloomPass.BlurVertical", "shaders/Blur.frag", { "BLUR_VERTICAL" });
	CreatePixelShader(BloomPass.BlurHorizontal, "BloomPass.BlurHorizontal", "shaders/Blur.frag", { "BLUR_HORIZONTAL" });

	D3D11_BUFFER_DESC bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.ByteWidth = sizeof(BloomPushConstants);
	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HRESULT result = Device->CreateBuffer(&bufDesc, nullptr, BloomPass.ConstantBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(BloomPass.ConstantBuffer) failed");
	SetDebugName(BloomPass.ConstantBuffer, "BloomPass.ConstantBuffer");

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	result = Device->CreateBlendState(&blendDesc, BloomPass.AdditiveBlendState.TypedInitPtr());
	ThrowIfFailed(result, "CreateBlendState(BloomPass.AdditiveBlendState) failed");
	SetDebugName(BloomPass.AdditiveBlendState, "BloomPass.AdditiveBlendState");
}

void UD3D11RenderDevice::CreatePresentPass()
{
	std::vector<vec2> positions =
	{
		vec2(-1.0, -1.0),
		vec2( 1.0, -1.0),
		vec2(-1.0,  1.0),
		vec2(-1.0,  1.0),
		vec2( 1.0, -1.0),
		vec2( 1.0,  1.0)
	};

	D3D11_BUFFER_DESC bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufDesc.ByteWidth = positions.size() * sizeof(vec2);
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = positions.data();

	HRESULT result = Device->CreateBuffer(&bufDesc, &initData, PresentPass.PPStepVertexBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(PresentPass.PPStepVertexBuffer) failed");
	SetDebugName(PresentPass.PPStepVertexBuffer, "PresentPass.PPStepVertexBuffer");

	bufDesc = {};
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.ByteWidth = sizeof(PresentPushConstants);
	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	result = Device->CreateBuffer(&bufDesc, nullptr, PresentPass.PresentConstantBuffer.TypedInitPtr());
	ThrowIfFailed(result, "CreateBuffer(PresentPass.PresentConstantBuffer) failed");
	SetDebugName(PresentPass.PresentConstantBuffer, "PresentPass.PresentConstantBuffer");

	std::vector<D3D11_INPUT_ELEMENT_DESC> elements =
	{
		{ "AttrPos", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	CreateVertexShader(PresentPass.PPStep, "PresentPass.PPStep", PresentPass.PPStepLayout, "PresentPass.PPStepLayout", elements, "shaders/PPStep.vert");

	static const char* transferFunctions[2] = { nullptr, "HDR_MODE" };
	static const char* gammaModes[2] = { "GAMMA_MODE_D3D9", "GAMMA_MODE_XOPENGL" };
	static const char* colorModes[4] = { nullptr, "COLOR_CORRECT_MODE0", "COLOR_CORRECT_MODE1", "COLOR_CORRECT_MODE2" };
	for (int i = 0; i < 16; i++)
	{
		std::vector<std::string> defines;
		if (transferFunctions[i & 1]) defines.push_back(transferFunctions[i & 1]);
		if (gammaModes[(i >> 1) & 1]) defines.push_back(gammaModes[(i >> 1) & 1]);
		if (colorModes[(i >> 2) & 3]) defines.push_back(colorModes[(i >> 2) & 3]);

		CreatePixelShader(PresentPass.Present[i], "PresentPass.Present", "shaders/Present.frag", defines);
	}

	CreatePixelShader(PresentPass.HitResolve, "PresentPass.HitResolve", "shaders/HitResolve.frag");

	static const float ditherdata[64] =
	{
		.0078125, .2578125, .1328125, .3828125, .0234375, .2734375, .1484375, .3984375,
		.7578125, .5078125, .8828125, .6328125, .7734375, .5234375, .8984375, .6484375,
		.0703125, .3203125, .1953125, .4453125, .0859375, .3359375, .2109375, .4609375,
		.8203125, .5703125, .9453125, .6953125, .8359375, .5859375, .9609375, .7109375,
		.0390625, .2890625, .1640625, .4140625, .0546875, .3046875, .1796875, .4296875,
		.7890625, .5390625, .9140625, .6640625, .8046875, .5546875, .9296875, .6796875,
		.1015625, .3515625, .2265625, .4765625, .1171875, .3671875, .2421875, .4921875,
		.8515625, .6015625, .9765625, .7265625, .8671875, .6171875, .9921875, .7421875
	};

	initData = {};
	initData.pSysMem = ditherdata;
	initData.SysMemPitch = sizeof(float) * 8;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.Width = 8;
	texDesc.Height = 8;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	result = Device->CreateTexture2D(&texDesc, &initData, PresentPass.DitherTexture.TypedInitPtr());
	ThrowIfFailed(result, "CreateTexture2D(DitherTexture) failed");
	SetDebugName(PresentPass.DitherTexture, "PresentPass.DitherTexture");

	result = Device->CreateShaderResourceView(PresentPass.DitherTexture, nullptr, PresentPass.DitherTextureView.TypedInitPtr());
	ThrowIfFailed(result, "CreateShaderResourceView(DitherTexture) failed");
	SetDebugName(PresentPass.DitherTextureView, "PresentPass.DitherTextureView");

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = Device->CreateBlendState(&blendDesc, PresentPass.BlendState.TypedInitPtr());
	ThrowIfFailed(result, "CreateBlendState(PresentPass.BlendState) failed");
	SetDebugName(PresentPass.BlendState, "PresentPass.BlendState");

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	result = Device->CreateDepthStencilState(&depthStencilDesc, PresentPass.DepthStencilState.TypedInitPtr());
	ThrowIfFailed(result, "CreateDepthStencilState(PresentPass.DepthStencilState) failed");
	SetDebugName(PresentPass.DepthStencilState, "PresentPass.DepthStencilState");

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	result = Device->CreateRasterizerState(&rasterizerDesc, PresentPass.RasterizerState.TypedInitPtr());
	ThrowIfFailed(result, "CreateRasterizerState(PresentPass.RasterizerState) failed");
	SetDebugName(PresentPass.RasterizerState, "PresentPass.RasterizerState");
}

#if defined(UNREALGOLD)

void UD3D11RenderDevice::Flush()
{
	guard(UD3D11RenderDevice::Flush);

	DrawBatches();
	ClearTextureCache();

	if (UsePrecache && !GIsEditor)
		PrecacheOnFlip = 1;

	unguard;
}

#else

void UD3D11RenderDevice::Flush(UBOOL AllowPrecache)
{
	guard(UD3D11RenderDevice::Flush);

	DrawBatches();
	ClearTextureCache();

	if (AllowPrecache && UsePrecache && !GIsEditor)
		PrecacheOnFlip = 1;

	unguard;
}

#endif

UBOOL UD3D11RenderDevice::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	guard(UD3D11RenderDevice::Exec);

	if (ParseCommand(&Cmd, TEXT("VRRECENTER")))
	{
		// Bindable recenter: reset the head-look reference to the current head pose.
		VRRecenterPending = true;
		return 1;
	}

	// Mod-facing VR query interface. A mod reads these via PlayerPawn.ConsoleCommand("VR ..."),
	// which returns whatever we Log here — so the mod stays renderer-agnostic (any VR driver that
	// implements this exec answers; the mod never needs to know which one is live).
	if (ParseCommand(&Cmd, TEXT("VR")))
	{
		if (ParseCommand(&Cmd, TEXT("GETPARAM")))
		{
			// VR GETPARAM <PropName> -> current value of a config knob (e.g. VRHudDepth), so the mod
			// can render actors at the same depth the user dialled in.
			while (*Cmd == ' ') Cmd++;
			TCHAR Name[128] = TEXT("");
			INT n = 0;
			while (*Cmd && *Cmd != ' ' && n < 127) Name[n++] = *Cmd++;
			Name[n] = 0;
			UProperty* Prop = FindField<UProperty>(GetClass(), Name);
			if (Prop)
			{
				BYTE* V = (BYTE*)this + Prop->Offset;
				if (Cast<UFloatProperty>(Prop))
					Ar.Logf(TEXT("%f"), *(FLOAT*)V);
				else if (Cast<UIntProperty>(Prop))
					Ar.Logf(TEXT("%i"), *(INT*)V);
				else if (UBoolProperty* BP = Cast<UBoolProperty>(Prop))
					Ar.Logf(TEXT("%i"), (*(DWORD*)V & BP->BitMask) ? 1 : 0);
				else if (Cast<UByteProperty>(Prop))
					Ar.Logf(TEXT("%i"), *(BYTE*)V);
			}
			return 1;
		}
		if (ParseCommand(&Cmd, TEXT("INGAME")))
		{
			// 1 = the game owns the mouse (playing) -> draw actors at gameplay HUD depth; 0 = a
			// menu/console is up (mouse shown) -> UI depth.
			Ar.Logf(TEXT("%i"), (Viewport && !Viewport->bShowWindowsMouse) ? 1 : 0);
			return 1;
		}
		return 1;
	}

	if (ParseCommand(&Cmd, TEXT("DGL")))
	{
		if (ParseCommand(&Cmd, TEXT("BUFFERTRIS")))
		{
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("BUILD")))
		{
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("AA")))
		{
			return 1;
		}
		return 0;
	}
	else if (ParseCommand(&Cmd, TEXT("GetRes")))
	{
		struct Resolution
		{
			int X;
			int Y;

			// For sorting highest resolution first
			bool operator<(const Resolution& other) const { if (X != other.X) return X > other.X; else return Y > other.Y; }
		};

		std::set<Resolution> resolutions;

		// Always include what the monitor is currently using
		resolutions.insert({ DesktopResolution.Width, DesktopResolution.Height });

		IDXGIOutput* output = nullptr;
		HRESULT result = SwapChain->GetContainingOutput(&output);
		if (SUCCEEDED(result))
		{
			UINT numModes = 0;
			result = output->GetDisplayModeList(ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, nullptr);
			if (SUCCEEDED(result))
			{
				std::vector<DXGI_MODE_DESC> descs(numModes);
				result = output->GetDisplayModeList(ActiveHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, descs.data());
				if (SUCCEEDED(result))
				{
					for (const DXGI_MODE_DESC& desc : descs)
					{
						resolutions.insert({ (int)desc.Width, (int)desc.Height });
					}
				}
			}
			output->Release();
		}

		FString Str;
		for (const Resolution& resolution : resolutions)
		{
			Str += FString::Printf(TEXT("%ix%i "), (INT)resolution.X, (INT)resolution.Y);
		}
		Ar.Log(*Str.LeftChop(1));
		return 1;
	}
	else
	{
#if !defined(UNREALGOLD)
		return URenderDevice::Exec(Cmd, Ar);
#else
		return 0;
#endif
	}

	unguard;
}

void UD3D11RenderDevice::MapVertices(bool nextBuffer)
{
	if (!SceneVertices)
	{
		D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer = {};
		HRESULT result = Context->Map(ScenePass.VertexBuffer, 0, nextBuffer ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVertexBuffer);
		if (SUCCEEDED(result))
		{
			SceneVertices = (SceneVertex*)mappedVertexBuffer.pData;
		}
	}

	if (!SceneIndexes)
	{
		D3D11_MAPPED_SUBRESOURCE mappedIndexBuffer = {};
		HRESULT result = Context->Map(ScenePass.IndexBuffer, 0, nextBuffer ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedIndexBuffer);
		if (SUCCEEDED(result))
		{
			SceneIndexes = (uint32_t*)mappedIndexBuffer.pData;
		}
	}
}

void UD3D11RenderDevice::UnmapVertices()
{
	if (SceneVertices)
	{
		Context->Unmap(ScenePass.VertexBuffer, 0);
		SceneVertices = nullptr;
	}

	if (SceneIndexes)
	{
		Context->Unmap(ScenePass.IndexBuffer, 0);
		SceneIndexes = nullptr;
	}
}

void UD3D11RenderDevice::Lock(FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize)
{
	guard(UD3D11RenderDevice::Lock);

	VRMainCaptured = false; // recapture the main scene node's FOV this frame
	VRSubRProjZ = 0.0f;     // no sub-view FOV seen yet this frame
	VRSubX = 0;

	Timers.DrawBatches.Reset();
	Timers.DrawComplexSurface.Reset();
	Timers.DrawGouraudPolygon.Reset();
	Timers.DrawTile.Reset();
	Timers.DrawGouraudTriangles.Reset();
	Timers.TextureCache.Reset();
	Timers.TextureUpload.Reset();

	nulltex = Textures->GetNullTexture();

	int wantedBufferCount = UseVSync ? 2 : 3;
	if (BufferCount != wantedBufferCount)
	{
		BufferCount = wantedBufferCount;
		ReleaseSwapChainResources();
		UpdateSwapChain();
	}

	// VR frame start: pump the runtime, get eye poses, render the scene at eye
	// resolution. BeginFrame is called unconditionally (it bootstraps the session
	// via its event pump); it returns false until the session is rendering, in
	// which case we just render mono to the desktop this frame.
	VRShouldRender = false;
	VRRetaining = false;
	int sceneSizeX = CurrentSizeX;
	int sceneSizeY = CurrentSizeY;
	if (VR)
	{
		VRShouldRender = VR->BeginFrame(VREyes);
		VRRetaining = VRShouldRender;
		if (VRShouldRender)
		{
			uint32_t ew = 0, eh = 0;
			VR->GetEyeResolution(ew, eh);
			sceneSizeX = (int)ew;
			sceneSizeY = (int)eh;
		}
	}

	if (sceneSizeX && sceneSizeY)
	{
		try
		{
			ResizeSceneBuffers(sceneSizeX, sceneSizeY, GetSettingsMultisample());
		}
		catch (const std::exception& e)
		{
			debugf(TEXT("Could not resize scene buffers: %s"), to_utf16(e.what()).c_str());
			return;
		}
	}

	HitData = InHitData;
	HitSize = InHitSize;

	FlashScale = InFlashScale;
	FlashFog = InFlashFog;

	FLOAT color[4] = { ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W };
	FLOAT zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ID3D11RenderTargetView* views[2] = { SceneBuffers.ColorBufferView, SceneBuffers.HitBufferView };
	Context->ClearRenderTargetView(SceneBuffers.ColorBufferView, color);
	Context->ClearRenderTargetView(SceneBuffers.HitBufferView, zero);
	Context->ClearDepthStencilView(SceneBuffers.DepthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	Context->OMSetRenderTargets(2, views, SceneBuffers.DepthBufferView);

	UINT stride = sizeof(SceneVertex);
	UINT offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { ScenePass.VertexBuffer.get() };
	ID3D11Buffer* cbs[1] = { ScenePass.ConstantBuffer.get() };
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetIndexBuffer(ScenePass.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetInputLayout(ScenePass.InputLayout);
	Context->VSSetShader(ScenePass.VertexShader, nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, cbs);
	Context->RSSetState(ScenePass.RasterizerState[SceneBuffers.Multisample > 1]);

	D3D11_RECT box = {};
	box.right = CurrentSizeX;
	box.bottom = CurrentSizeY;
	Context->RSSetScissorRects(1, &box);

	MapVertices(true);

	SceneConstants.HitIndex = 0;
	ForceHitIndex = -1;

	IsLocked = true;

	unguard;
}

void UD3D11RenderDevice::DrawStats(FSceneNode* Frame)
{
	Super::DrawStats(Frame);

	CycleTimer::SetActive(true);

#if defined(OLDUNREAL469SDK)
	GRender->ShowStat(
		CurrentFrame,
		TEXT("D3D11: Draw calls: %d, Complex surfaces: %d, Gouraud polygons: %d, Tiles: %d; Uploads: %d, Rect Uploads: %d, Buffers Used: %d\r\n"),
		Stats.DrawCalls,
		Stats.ComplexSurfaces,
		Stats.GouraudPolygons,
		Stats.Tiles,
		Stats.Uploads,
		Stats.RectUploads,
		Stats.BuffersUsed);

	GRender->ShowStat(
		CurrentFrame,
		TEXT("D3D11: DrawBatches: %f ms, Complex surfaces: %f ms, Polygons: %f ms, Tiles: %f ms, Cache %f ms, Upload %f ms\r\n"),
		Timers.DrawBatches.TimeMS(),
		Timers.DrawComplexSurface.TimeMS(),
		Timers.DrawGouraudPolygon.TimeMS() + Timers.DrawGouraudTriangles.TimeMS(),
		Timers.DrawTile.TimeMS(),
		Timers.TextureCache.TimeMS(),
		Timers.TextureUpload.TimeMS());
#endif

	Stats.DrawCalls = 0;
	Stats.ComplexSurfaces = 0;
	Stats.GouraudPolygons = 0;
	Stats.Tiles = 0;
	Stats.Uploads = 0;
	Stats.RectUploads = 0;
	Stats.BuffersUsed = 1;
}

PresentPushConstants UD3D11RenderDevice::GetPresentPushConstants(float brightnessScale, float brightnessOffset)
{
	PresentPushConstants pushconstants;
	pushconstants.HdrScale = 0.8f + HdrScale * (3.0f / 255.0f);
	if (Viewport->IsOrtho())
	{
		pushconstants.GammaCorrection = { 1.0f };
		pushconstants.Contrast = 1.0f;
		pushconstants.Saturation = 1.0f;
		pushconstants.Brightness = 0.0f;
	}
	else
	{
		float brightness = Clamp(Viewport->GetOuterUClient()->Brightness * 2.0, 0.05, 2.99);
		brightness = Clamp(brightness * brightnessScale + brightnessOffset, 0.05f, 2.99f); // VR eye override (1,0 = unchanged)

		if (GammaMode == 0)
		{
			float invGammaRed = 1.0f / Max(brightness + GammaOffset + GammaOffsetRed, 0.001f);
			float invGammaGreen = 1.0f / Max(brightness + GammaOffset + GammaOffsetGreen, 0.001f);
			float invGammaBlue = 1.0f / Max(brightness + GammaOffset + GammaOffsetBlue, 0.001f);
			pushconstants.GammaCorrection = vec4(invGammaRed, invGammaGreen, invGammaBlue, 0.0f);
		}
		else
		{
			float invGammaRed = (GammaOffset + GammaOffsetRed + 2.0f) > 0.0f ? 1.0f / (GammaOffset + GammaOffsetRed + 1.0f) : 1.0f;
			float invGammaGreen = (GammaOffset + GammaOffsetGreen + 2.0f) > 0.0f ? 1.0f / (GammaOffset + GammaOffsetGreen + 1.0f) : 1.0f;
			float invGammaBlue = (GammaOffset + GammaOffsetBlue + 2.0f) > 0.0f ? 1.0f / (GammaOffset + GammaOffsetBlue + 1.0f) : 1.0f;
			pushconstants.GammaCorrection = vec4(invGammaRed, invGammaGreen, invGammaBlue, brightness);
		}

		// pushconstants.Contrast = clamp(Contrast, 0.1f, 3.f);
		if (Contrast >= 128)
		{
			pushconstants.Contrast = 1.0f + (Contrast - 128) / 127.0f * 3.0f;
		}
		else
		{
			pushconstants.Contrast = Max(Contrast / 128.0f, 0.1f);
		}

		// pushconstants.Saturation = clamp(Saturation, -1.0f, 1.0f);
		pushconstants.Saturation = 1.0f - 2.0f * (255 - Saturation) / 255.0f;

		// pushconstants.Brightness = clamp(LinearBrightness, -1.8f, 1.8f);
		if (LinearBrightness >= 128)
		{
			pushconstants.Brightness = (LinearBrightness - 128) / 127.0f * 1.8f;
		}
		else
		{
			pushconstants.Brightness = (128 - LinearBrightness) / 128.0f * -1.8f;
		}
	}
	return pushconstants;
}

void UD3D11RenderDevice::Unlock(UBOOL Blit)
{
	guard(UD3D11RenderDevice::Unlock);

	if (!IsLocked) // Don't trust the engine.
		return;

	if (VRRetaining)
	{
		// Replay the accumulated frame into both eyes, submit, mirror to desktop.
		RenderVREyes();
		VRRetaining = false;
		IsLocked = false;
		if (VR)
			VR->EndFrame(true);
		return;
	}

	DrawBatches();
	UnmapVertices();

	Batch.SceneIndexStart = 0;
	SceneVertexPos = 0;
	SceneIndexPos = 0;

	// This path also runs for VR when the headset is off (shouldRender=false -> mono frame).
	// Close the begun OpenXR frame (EndFrame no-ops if none was begun), and only present to
	// the desktop per VRMirrorMode. Non-VR (VR==null) always presents.
	if (VR)
		VR->EndFrame(false);

	if (Blit && (!VR || VRWantMirror()))
	{
		if (SceneBuffers.Multisample > 1)
		{
			Context->ResolveSubresource(SceneBuffers.PPImage[0], 0, SceneBuffers.ColorBuffer, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
		}
		else
		{
			Context->CopyResource(SceneBuffers.PPImage[0], SceneBuffers.ColorBuffer);
		}

		if (Bloom)
		{
			RunBloomPass();
		}

		ID3D11RenderTargetView* rtvs[1] = { BackBufferView.get() };
		Context->OMSetRenderTargets(1, rtvs, nullptr);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = CurrentSizeX;
		viewport.Height = CurrentSizeY;
		viewport.MaxDepth = 1.0f;
		// VR SBS-record mode leaves the backbuffer larger than the window (see UpdateSwapChain);
		// when this fallback present runs (VR session not rendering), it must fill the whole
		// backbuffer or the picture lands squeezed in its top-left corner.
		if (VR && BackBufferSizeX > 0 && (BackBufferSizeX != CurrentSizeX || BackBufferSizeY != CurrentSizeY))
		{
			viewport.Width = (FLOAT)BackBufferSizeX;
			viewport.Height = (FLOAT)BackBufferSizeY;
		}
		Context->RSSetViewports(1, &viewport);

		PresentPushConstants pushconstants = GetPresentPushConstants();

		// Select present shader based on what the user is actually using
		int presentShader = 0;
		if (ActiveHdr) presentShader |= 1;
		if (GammaMode == 1) presentShader |= 2;
		if (pushconstants.Brightness != 0.0f || pushconstants.Contrast != 1.0f || pushconstants.Saturation != 1.0f) presentShader |= (Clamp(GrayFormula, 0, 2) + 1) << 2;

		UINT stride = sizeof(vec2);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get()};
		ID3D11ShaderResourceView* psResources[] = { SceneBuffers.PPImageShaderView[0], PresentPass.DitherTextureView};
		ID3D11Buffer* cbs[1] = { PresentPass.PresentConstantBuffer.get() };
		Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
		Context->IASetInputLayout(PresentPass.PPStepLayout);
		Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
		Context->RSSetState(PresentPass.RasterizerState);
		Context->PSSetShader(PresentPass.Present[presentShader], nullptr, 0);
		Context->PSSetConstantBuffers(0, 1, cbs);
		Context->PSSetShaderResources(0, 2, psResources);
		Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
		Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);
		Context->UpdateSubresource(PresentPass.PresentConstantBuffer, 0, nullptr, &pushconstants, 0, 0);
		Context->Draw(6, 0);

		if (SwapChain1)
		{
			UINT flags = 0;
			if (!UseVSync && !CurrentFullscreen && DxgiSwapChainAllowTearing)
				flags |= DXGI_PRESENT_ALLOW_TEARING;

			DXGI_PRESENT_PARAMETERS presentParams = {};
			SwapChain1->Present1(UseVSync ? 1 : 0, flags, &presentParams);
		}
		else
		{
			SwapChain->Present(UseVSync ? 1 : 0, 0);
		}

		Batch.Pipeline = nullptr;
		Batch.Tex = nullptr;
		Batch.Lightmap = nullptr;
		Batch.Detailtex = nullptr;
		Batch.Macrotex = nullptr;
		Batch.SceneIndexStart = 0;

		UpdateLODBias();
	}

	if (HitData)
	{
		D3D11_BOX box = {};
		box.left = Viewport->HitX;
		box.right = Viewport->HitX + Viewport->HitXL;
		box.top = SceneBuffers.Height - Viewport->HitY - Viewport->HitYL;
		box.bottom = SceneBuffers.Height - Viewport->HitY;
		box.front = 0;
		box.back = 1;

		// Resolve multisampling
		if (SceneBuffers.Multisample > 1)
		{
			ID3D11RenderTargetView* rtvs[1] = { SceneBuffers.PPHitBufferView.get() };
			Context->OMSetRenderTargets(1, rtvs, nullptr);

			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = box.left;
			viewport.TopLeftY = box.top;
			viewport.Width = box.right - box.left;
			viewport.Height = box.bottom - box.top;
			viewport.MaxDepth = 1.0f;
			Context->RSSetViewports(1, &viewport);

			UINT stride = sizeof(vec2);
			UINT offset = 0;
			ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get() };
			ID3D11ShaderResourceView* srvs[1] = { SceneBuffers.HitBufferShaderView.get() };
			Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
			Context->IASetInputLayout(PresentPass.PPStepLayout);
			Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
			Context->RSSetState(PresentPass.RasterizerState);
			Context->PSSetShader(PresentPass.HitResolve, nullptr, 0);
			Context->PSSetShaderResources(0, 1, srvs);
			Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
			Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);

			Context->Draw(6, 0);
		}
		else
		{
			Context->CopySubresourceRegion(SceneBuffers.PPHitBuffer, 0, box.left, box.top, 0, SceneBuffers.HitBuffer, 0, &box);
		}

		// Copy the hit buffer to a mappable texture, but only the part we want to examine
		Context->CopySubresourceRegion(SceneBuffers.StagingHitBuffer, 0, 0, 0, 0, SceneBuffers.PPHitBuffer, 0, &box);

		// Lock the buffer and look for the last hit
		int hit = 0;
		D3D11_MAPPED_SUBRESOURCE mapping = {};
		HRESULT result = Context->Map(SceneBuffers.StagingHitBuffer, 0, D3D11_MAP_READ, 0, &mapping);
		if (SUCCEEDED(result))
		{
			int width = Viewport->HitXL;
			int height = Viewport->HitYL;
			for (int y = 0; y < height; y++)
			{
				const INT* line = (const INT*)(((const char*)mapping.pData) + y * mapping.RowPitch);
				for (int x = 0; x < width; x++)
				{
					hit = std::max(hit, line[x]);
				}
			}
			Context->Unmap(SceneBuffers.StagingHitBuffer, 0);
		}
		hit--;

		hit = std::max(hit, ForceHitIndex);

		if (hit >= 0 && hit < (int)HitQueries.size())
		{
			const HitQuery& query = HitQueries[hit];
			memcpy(HitData, HitBuffer.data() + query.Start, query.Count);
			*HitSize = query.Count;
		}
		else
		{
			*HitSize = 0;
		}
	}

	Context->OMSetRenderTargets(0, nullptr, nullptr);

	// VR frame that the compositor told us to skip: still must close the frame.
	// No-op when VR is off or no frame was begun.
	if (VR)
		VR->EndFrame(VRShouldRender);

	HitQueryStack.clear();
	HitQueries.clear();
	HitBuffer.clear();
	HitData = nullptr;
	HitSize = nullptr;

	IsLocked = false;

	PrintDebugLayerMessages();

	unguard;
}

void UD3D11RenderDevice::PushHit(const BYTE* Data, INT Count)
{
	guard(UD3D11RenderDevice::PushHit);

	if (Count <= 0) return;
	HitQueryStack.insert(HitQueryStack.end(), Data, Data + Count);

	SetHitLocation();

	unguard;
}

void UD3D11RenderDevice::PopHit(INT Count, UBOOL bForce)
{
	guard(UD3D11RenderDevice::PopHit);

	if (bForce) // Force hit what we are popping
		ForceHitIndex = HitQueries.size() - 1;

	HitQueryStack.resize(HitQueryStack.size() - Count);

	SetHitLocation();

	unguard;
}

void UD3D11RenderDevice::SetHitLocation()
{
	DrawBatches();

	if (!HitQueryStack.empty())
	{
		INT index = HitQueries.size();

		HitQuery query;
		query.Start = HitBuffer.size();
		query.Count = HitQueryStack.size();
		HitQueries.push_back(query);

		HitBuffer.insert(HitBuffer.end(), HitQueryStack.begin(), HitQueryStack.end());

		SceneConstants.HitIndex = index + 1;
	}
	else
	{
		SceneConstants.HitIndex = 0;
	}

	Context->UpdateSubresource(ScenePass.ConstantBuffer, 0, nullptr, &SceneConstants, 0, 0);
}

#if defined(OLDUNREAL469SDK)

UBOOL UD3D11RenderDevice::SupportsTextureFormat(ETextureFormat Format)
{
	guard(UD3D11RenderDevice::SupportsTextureFormat);

	return Uploads->SupportsTextureFormat(Format) ? TRUE : FALSE;

	unguard;
}

void UD3D11RenderDevice::UpdateTextureRect(FTextureInfo& Info, INT U, INT V, INT UL, INT VL)
{
	guardSlow(UD3D11RenderDevice::UpdateTextureRect);

	Textures->UpdateTextureRect(&Info, U, V, UL, VL);

	unguardSlow;
}

#endif

void UD3D11RenderDevice::DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
	guardSlow(UD3D11RenderDevice::DrawComplexSurface);

	if (VRRetaining)
	{
		VRSetProj(VRIsSkyFrame(Frame) ? VRPROJ_SKY : VRPROJ_WORLD);
		VRBeginPostRender();
	}

	Timers.DrawComplexSurface.Clock();
	ActiveTimer = &Timers.DrawComplexSurface;

	DWORD PolyFlags = ApplyPrecedenceRules(Surface.PolyFlags);

	ComplexSurfaceInfo info;
	info.facet = &Facet;
	info.tex = Textures->GetTexture(Surface.Texture, (PolyFlags & PF_Masked) || 
		(Surface.Texture->Texture && (Surface.Texture->Texture->PolyFlags & PF_Masked)));
	info.lightmap = Textures->GetTexture(Surface.LightMap, false);
	info.macrotex = Textures->GetTexture(Surface.MacroTexture, false);
	info.detailtex = Textures->GetTexture(Surface.DetailTexture, false);
	info.fogmap = (Surface.FogMap && Surface.FogMap->Mips[0] && Surface.FogMap->Mips[0]->DataPtr) ?
		Textures->GetTexture(Surface.FogMap, false) : nulltex;
	info.editorcolor = nullptr;

#if defined(UNREALGOLD)
	if (Surface.DetailTexture && Surface.FogMap) info.detailtex = nulltex;
#else
	if ((Surface.DetailTexture && Surface.FogMap) || (!DetailTextures)) info.detailtex = nulltex;
#endif

	if (info.fogmap != nulltex)
		info.detailtex = info.fogmap;

	SetPipeline(PolyFlags);
	SetDescriptorSet(PolyFlags, info);

	DrawComplexSurfaceFaces(info);

	Stats.ComplexSurfaces++;
	Timers.DrawComplexSurface.Unclock();
	ActiveTimer = nullptr;

	if (!GIsEditor || (PolyFlags & (PF_Selected | PF_FlatShaded)) == 0)
		return;

	// Editor highlight surface (so stupid this is delegated to the renderdev as the engine could just issue a second call):

	SetPipeline(PF_Highlighted);
	SetDescriptorSet(PF_Highlighted);

	vec4 editorcolor;
	if (PolyFlags & PF_FlatShaded)
	{
		editorcolor.x = Surface.FlatColor.R / 255.0f;
		editorcolor.y = Surface.FlatColor.G / 255.0f;
		editorcolor.z = Surface.FlatColor.B / 255.0f;
		editorcolor.w = 0.85f;
		if (PolyFlags & PF_Selected)
		{
			editorcolor.x *= 1.5f;
			editorcolor.y *= 1.5f;
			editorcolor.z *= 1.5f;
			editorcolor.w = 1.0f;
		}
	}
	else
	{
		editorcolor = vec4(0.0f, 0.0f, 0.05f, 0.20f);
	}
	info.editorcolor = &editorcolor;

	DrawComplexSurfaceFaces(info);

	unguardSlow;
}

#ifdef USE_SSE2

// Calculates dot(vec4, vec4). All elements will hold the result.
inline __m128 sse_dot4(__m128 v0, __m128 v1)
{
	v0 = _mm_mul_ps(v0, v1);

	v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(2, 3, 0, 1));
	v0 = _mm_add_ps(v0, v1);
	v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(0, 1, 2, 3));
	v0 = _mm_add_ps(v0, v1);

	return v0;
}

void UD3D11RenderDevice::DrawComplexSurfaceFaces(const ComplexSurfaceInfo& info)
{
	uint32_t flags = 0;
	if (info.lightmap != nulltex) flags |= 1;
	if (info.macrotex != nulltex) flags |= 2;
	if (info.detailtex != nulltex && info.fogmap == nulltex) flags |= 4;
	if (info.fogmap != nulltex) flags |= 8;
	if (LightMode == 1) flags |= 64;

	__m128 mflags = _mm_castsi128_ps(_mm_cvtsi32_si128(flags));
	__m128 maskClearW = _mm_castsi128_ps(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0));
	__m128 xaxis = _mm_and_ps(_mm_loadu_ps((float*)&info.facet->MapCoords.XAxis), maskClearW);
	__m128 yaxis = _mm_and_ps(_mm_loadu_ps((float*)&info.facet->MapCoords.YAxis), maskClearW);
	__m128 origin = _mm_and_ps(_mm_loadu_ps((float*)&info.facet->MapCoords.Origin), maskClearW);

	__m128 UDot = sse_dot4(xaxis, origin);
	__m128 VDot = sse_dot4(yaxis, origin);
	__m128 UVDot = _mm_shuffle_ps(UDot, VDot, _MM_SHUFFLE(0, 0, 0, 0));
	UVDot = _mm_shuffle_ps(UVDot, UVDot, _MM_SHUFFLE(2, 0, 2, 0));

	float UPan = info.tex->PanX;
	float VPan = info.tex->PanY;
	float LMUPan = info.lightmap->PanX - 0.5f * info.lightmap->UScale;
	float LMVPan = info.lightmap->PanY - 0.5f * info.lightmap->VScale;
	__m128 pan0 = _mm_add_ps(UVDot, _mm_setr_ps(UPan, VPan, LMUPan, LMVPan));

	float MacroUPan = info.macrotex->PanX;
	float MacroVPan = info.macrotex->PanY;
	float DetailUPan = info.fogmap == nulltex ? info.detailtex->PanX : info.fogmap->PanX - 0.5f * info.fogmap->UScale;
	float DetailVPan = info.fogmap == nulltex ? info.detailtex->PanY : info.fogmap->PanY - 0.5f * info.fogmap->VScale;
	__m128 pan1 = _mm_add_ps(UVDot, _mm_setr_ps(MacroUPan, MacroVPan, DetailUPan, DetailVPan));

	float UMult = info.tex->UMult;
	float VMult = info.tex->VMult;
	float LMUMult = info.lightmap->UMult;
	float LMVMult = info.lightmap->VMult;
	__m128 mult0 = _mm_setr_ps(UMult, VMult, LMUMult, LMVMult);

	float MacroUMult = info.macrotex->UMult;
	float MacroVMult = info.macrotex->VMult;
	float DetailUMult = info.fogmap == nulltex ? info.detailtex->UMult : info.fogmap->UMult;
	float DetailVMult = info.fogmap == nulltex ? info.detailtex->VMult : info.fogmap->VMult;
	__m128 mult1 = _mm_setr_ps(MacroUMult, MacroVMult, DetailUMult, DetailVMult);

	__m128 color = info.editorcolor ? _mm_loadu_ps(&info.editorcolor->x) : _mm_set_ps1(1.0f);

	for (FSavedPoly* Poly = info.facet->Polys; Poly; Poly = Poly->Next)
	{
		auto pts = Poly->Pts;
		uint32_t vcount = Poly->NumPts;
		if (vcount < 3) continue;

		uint32_t icount = (vcount - 2) * 3;
		auto alloc = ReserveVertices(vcount, icount);
		if (alloc.vptr)
		{
			SceneVertex* vptr = alloc.vptr;
			uint32_t* iptr = alloc.iptr;
			uint32_t vpos = alloc.vpos;

			for (uint32_t i = 0; i < vcount; i++)
			{
				__m128 point = _mm_and_ps(_mm_loadu_ps((float*)&pts[i]->Point), maskClearW);
				__m128 u = sse_dot4(xaxis, point);
				__m128 v = sse_dot4(yaxis, point);
				__m128 uv = _mm_shuffle_ps(u, v, _MM_SHUFFLE(0, 0, 0, 0));
				uv = _mm_shuffle_ps(uv, uv, _MM_SHUFFLE(2, 0, 2, 0));

				__m128 pos = _mm_or_ps(_mm_shuffle_ps(point, point, _MM_SHUFFLE(2, 1, 0, 3)), mflags);
				__m128 uv0 = _mm_mul_ps(_mm_sub_ps(uv, pan0), mult0);
				__m128 uv1 = _mm_mul_ps(_mm_sub_ps(uv, pan1), mult1);

				_mm_store_ps((float*)vptr, pos);
				_mm_store_ps((float*)vptr + 4, uv0);
				_mm_store_ps((float*)vptr + 8, uv1);
				_mm_store_ps((float*)vptr + 12, color);
				vptr++;
			}

			for (uint32_t i = vpos + 2; i < vpos + vcount; i++)
			{
				*(iptr++) = vpos;
				*(iptr++) = i - 1;
				*(iptr++) = i;
			}

			UseVertices(vcount, icount);
		}
	}
}

#else

void UD3D11RenderDevice::DrawComplexSurfaceFaces(const ComplexSurfaceInfo& info)
{
	uint32_t flags = 0;
	if (info.lightmap != nulltex) flags |= 1;
	if (info.macrotex != nulltex) flags |= 2;
	if (info.detailtex != nulltex && info.fogmap == nulltex) flags |= 4;
	if (info.fogmap != nulltex) flags |= 8;
	if (LightMode == 1) flags |= 64;

	FVector xaxis = info.facet->MapCoords.XAxis;
	FVector yaxis = info.facet->MapCoords.YAxis;
	float UDot = xaxis | info.facet->MapCoords.Origin;
	float VDot = yaxis | info.facet->MapCoords.Origin;

	float UPan = UDot + info.tex->PanX;
	float VPan = VDot + info.tex->PanY;
	float LMUPan = UDot + info.lightmap->PanX - 0.5f * info.lightmap->UScale;
	float LMVPan = VDot + info.lightmap->PanY - 0.5f * info.lightmap->VScale;
	float MacroUPan = UDot + info.macrotex->PanX;
	float MacroVPan = VDot + info.macrotex->PanY;
	float DetailUPan = UDot + (info.fogmap == nulltex ? info.detailtex->PanX : info.fogmap->PanX - 0.5f * info.fogmap->UScale);
	float DetailVPan = VDot + (info.fogmap == nulltex ? info.detailtex->PanY : info.fogmap->PanY - 0.5f * info.fogmap->VScale);

	float UMult = info.tex->UMult;
	float VMult = info.tex->VMult;
	float LMUMult = info.lightmap->UMult;
	float LMVMult = info.lightmap->VMult;
	float MacroUMult = info.macrotex->UMult;
	float MacroVMult = info.macrotex->VMult;
	float DetailUMult = info.fogmap == nulltex ? info.detailtex->UMult : info.fogmap->UMult;
	float DetailVMult = info.fogmap == nulltex ? info.detailtex->VMult : info.fogmap->VMult;

	vec4 color = info.editorcolor ? *info.editorcolor : vec4(1.0f);

	for (FSavedPoly* Poly = info.facet->Polys; Poly; Poly = Poly->Next)
	{
		auto pts = Poly->Pts;
		uint32_t vcount = Poly->NumPts;
		if (vcount < 3) continue;

		uint32_t icount = (vcount - 2) * 3;
		auto alloc = ReserveVertices(vcount, icount);
		if (alloc.vptr)
		{
			SceneVertex* vptr = alloc.vptr;
			uint32_t* iptr = alloc.iptr;
			uint32_t vpos = alloc.vpos;

			for (uint32_t i = 0; i < vcount; i++)
			{
				FVector point = pts[i]->Point;
				FLOAT u = xaxis | point;
				FLOAT v = yaxis | point;

				vptr->Flags = flags;
				vptr->Position.x = point.X;
				vptr->Position.y = point.Y;
				vptr->Position.z = point.Z;
				vptr->TexCoord.s = (u - UPan) * UMult;
				vptr->TexCoord.t = (v - VPan) * VMult;
				vptr->TexCoord2.s = (u - LMUPan) * LMUMult;
				vptr->TexCoord2.t = (v - LMVPan) * LMVMult;
				vptr->TexCoord3.s = (u - MacroUPan) * MacroUMult;
				vptr->TexCoord3.t = (v - MacroVPan) * MacroVMult;
				vptr->TexCoord4.s = (u - DetailUPan) * DetailUMult;
				vptr->TexCoord4.t = (v - DetailVPan) * DetailVMult;
				vptr->Color = color;
				vptr++;
			}

			for (uint32_t i = vpos + 2; i < vpos + vcount; i++)
			{
				*(iptr++) = vpos;
				*(iptr++) = i - 1;
				*(iptr++) = i;
			}

			UseVertices(vcount, icount);
		}
	}
}

#endif

void UD3D11RenderDevice::DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span)
{
	guardSlow(UD3D11RenderDevice::DrawGouraudPolygon);

	if (NumPts < 3) return; // This can apparently happen!!

	if (VRRetaining)
	{
		// HUD mesh. Two very different cases must not be confused:
		//  - Menu player-mesh preview (UMenuPlayerMeshClient): a narrow-FOV sub-view, NOT a render
		//    overlay. Magnify to fill the virtual screen like the 2D render.
		//  - Mod radar icon: a render overlay (RenderOverlays -> HACKFLAGS_NoNearZ) at the game FOV,
		//    scaled by VRScaleHudMeshes via the ClearZ arm (VRHudMeshArm). MUST stay on that path.
		// So the preview is "sub-view AND not an overlay AND narrow FOV"; a NoNearZ overlay is never a
		// preview even if DrawClippedActor made it a sub-view.
		bool noNearZ = (GUglyHackFlags & HACKFLAGS_NoNearZ) != 0;
		bool preview = VRInSubView && !noNearZ && VRSubRProjZ > 0.0f && VRSubRProjZ < VRMainRProjZ * 0.9f;
		if (preview || VRHudMeshArm)
		{
			float sx, sy;
			if (preview) // menu player-mesh preview: magnify to fill like the 2D render
			{
				float s = (VRMainX > 0) ? (VRMainRProjZ / VRSubRProjZ) * ((float)VRSubX / (float)VRMainX) : 1.0f;
				sx = s; sy = s;
			}
			else // radar HUD mesh (VRHudMeshArm implies VRScaleHudMeshes): scale IN-GAME only, same as
			{    // the HUD tile scaling — a menu up (mouse shown) leaves the icons at their real size.
				bool inGame = Viewport && !Viewport->bShowWindowsMouse;
				sx = inGame ? VRHudScaleX : 1.0f;
				sy = inGame ? VRHudScaleY : 1.0f;
			}
			VRSetMeshProj(sx, sy);
		}
		else
			VRSetProj(noNearZ ? VRPROJ_WEAPON : VRIsSkyFrame(Frame) ? VRPROJ_SKY : VRPROJ_WORLD);
		VRBeginPostRender();
	}

	Timers.DrawGouraudPolygon.Clock();
	ActiveTimer = &Timers.DrawGouraudPolygon;

	PolyFlags = ApplyPrecedenceRules(PolyFlags);

	CachedTexture* tex = Textures->GetTexture(&Info, !!(PolyFlags & PF_Masked));

	SetPipeline(PolyFlags);
	SetDescriptorSet(PolyFlags, tex);

	float UMult = tex->UMult;
	float VMult = tex->VMult;

	int flags = (PolyFlags & (PF_RenderFog | PF_Translucent | PF_Modulated)) == PF_RenderFog ? 16 : 0;
	if ((PolyFlags & (PF_Translucent | PF_Modulated)) == 0 && LightMode == 2) flags |= 32;

#ifdef USE_SSE2
	__m128 mflags = _mm_castsi128_ps(_mm_cvtsi32_si128(flags));
	__m128 maskClearW = _mm_castsi128_ps(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0));
#endif

	auto alloc = ReserveVertices(NumPts, (NumPts - 2) * 3);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

#ifdef USE_SSE2
		if (PolyFlags & PF_Modulated)
		{
			SceneVertex* vertex = vptr;
			__m128 color = _mm_set_ps1(1.0f);
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = Pts[i];

				__m128 point = _mm_and_ps(_mm_loadu_ps((float*)&P->Point), maskClearW);
				__m128 fog = _mm_loadu_ps((float*)&P->Fog);
				__m128 pos = _mm_or_ps(_mm_shuffle_ps(point, point, _MM_SHUFFLE(2, 1, 0, 3)), mflags);
				__m128 uvzero = _mm_setr_ps(P->U * UMult, P->V * VMult, 0.0f, 0.0f);
				__m128 uv0 = _mm_shuffle_ps(uvzero, fog, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 uv1 = _mm_shuffle_ps(fog, uvzero, _MM_SHUFFLE(1, 0, 1, 0));

				_mm_store_ps((float*)vertex, pos);
				_mm_store_ps((float*)vertex + 4, uv0);
				_mm_store_ps((float*)vertex + 8, uv1);
				_mm_store_ps((float*)vertex + 12, color);
				vertex++;
			}
		}
		else
		{
			SceneVertex* vertex = vptr;
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = Pts[i];

				__m128 point = _mm_and_ps(_mm_loadu_ps((float*)&P->Point), maskClearW);
				__m128 fog = _mm_loadu_ps((float*)&P->Fog);
				__m128 pos = _mm_or_ps(_mm_shuffle_ps(point, point, _MM_SHUFFLE(2, 1, 0, 3)), mflags);
				__m128 uvzero = _mm_setr_ps(P->U * UMult, P->V * VMult, 0.0f, 0.0f);
				__m128 uv0 = _mm_shuffle_ps(uvzero, fog, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 uv1 = _mm_shuffle_ps(fog, uvzero, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 color = _mm_and_ps(_mm_loadu_ps((float*)&P->Light), maskClearW);
				color = _mm_or_ps(color, _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

				_mm_store_ps((float*)vertex, pos);
				_mm_store_ps((float*)vertex + 4, uv0);
				_mm_store_ps((float*)vertex + 8, uv1);
				_mm_store_ps((float*)vertex + 12, color);
				vertex++;
			}
		}
#else
		if (PolyFlags & PF_Modulated)
		{
			SceneVertex* vertex = vptr;

			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = Pts[i];
				vertex->Flags = flags;
				vertex->Position.x = P->Point.X;
				vertex->Position.y = P->Point.Y;
				vertex->Position.z = P->Point.Z;
				vertex->TexCoord.s = P->U * UMult;
				vertex->TexCoord.t = P->V * VMult;
				vertex->TexCoord2.s = P->Fog.X;
				vertex->TexCoord2.t = P->Fog.Y;
				vertex->TexCoord3.s = P->Fog.Z;
				vertex->TexCoord3.t = P->Fog.W;
				vertex->TexCoord4.s = 0.0f;
				vertex->TexCoord4.t = 0.0f;
				vertex->Color.r = 1.0f;
				vertex->Color.g = 1.0f;
				vertex->Color.b = 1.0f;
				vertex->Color.a = 1.0f;
				vertex++;
			}
		}
		else
		{
			SceneVertex* vertex = vptr;
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = Pts[i];
				vertex->Flags = flags;
				vertex->Position.x = P->Point.X;
				vertex->Position.y = P->Point.Y;
				vertex->Position.z = P->Point.Z;
				vertex->TexCoord.s = P->U * UMult;
				vertex->TexCoord.t = P->V * VMult;
				vertex->TexCoord2.s = P->Fog.X;
				vertex->TexCoord2.t = P->Fog.Y;
				vertex->TexCoord3.s = P->Fog.Z;
				vertex->TexCoord3.t = P->Fog.W;
				vertex->TexCoord4.s = 0.0f;
				vertex->TexCoord4.t = 0.0f;
				vertex->Color.r = P->Light.X;
				vertex->Color.g = P->Light.Y;
				vertex->Color.b = P->Light.Z;
				vertex->Color.a = 1.0f;
				vertex++;
			}
		}
#endif

		uint32_t vstart = vpos;
		uint32_t vcount = NumPts;
		for (uint32_t i = vstart + 2; i < vstart + vcount; i++)
		{
			*(iptr++) = vstart;
			*(iptr++) = i - 1;
			*(iptr++) = i;
		}

		UseVertices(NumPts, (NumPts - 2) * 3);
	}

	Stats.GouraudPolygons++;
	Timers.DrawGouraudPolygon.Unclock();
	ActiveTimer = nullptr;

	unguardSlow;
}

#if defined(OLDUNREAL469SDK)

static void EnviroMap(const FSceneNode* Frame, FTransTexture& P, FLOAT UScale, FLOAT VScale)
{
	FVector T = P.Point.UnsafeNormal().MirrorByVector(P.Normal).TransformVectorBy(Frame->Uncoords);
	P.U = (T.X + 1.0f) * 0.5f * 256.0f * UScale;
	P.V = (T.Y + 1.0f) * 0.5f * 256.0f * VScale;
}

void UD3D11RenderDevice::DrawGouraudTriangles(const FSceneNode* Frame, const FTextureInfo& Info, FTransTexture* const InPts, INT NumPts, DWORD PolyFlags, DWORD DataFlags, FSpanBuffer* Span)
{
	guardSlow(UD3D11RenderDevice::DrawGouraudTriangles);

	if (VRRetaining)
	{
		// HUD mesh. Two very different cases must not be confused:
		//  - Menu player-mesh preview (UMenuPlayerMeshClient): a narrow-FOV sub-view, NOT a render
		//    overlay. Magnify to fill the virtual screen like the 2D render.
		//  - Mod radar icon: a render overlay (RenderOverlays -> HACKFLAGS_NoNearZ) at the game FOV,
		//    scaled by VRScaleHudMeshes via the ClearZ arm (VRHudMeshArm). MUST stay on that path.
		// So the preview is "sub-view AND not an overlay AND narrow FOV"; a NoNearZ overlay is never a
		// preview even if DrawClippedActor made it a sub-view.
		bool noNearZ = (GUglyHackFlags & HACKFLAGS_NoNearZ) != 0;
		bool preview = VRInSubView && !noNearZ && VRSubRProjZ > 0.0f && VRSubRProjZ < VRMainRProjZ * 0.9f;
		if (preview || VRHudMeshArm)
		{
			float sx, sy;
			if (preview) // menu player-mesh preview: magnify to fill like the 2D render
			{
				float s = (VRMainX > 0) ? (VRMainRProjZ / VRSubRProjZ) * ((float)VRSubX / (float)VRMainX) : 1.0f;
				sx = s; sy = s;
			}
			else // radar HUD mesh (VRHudMeshArm implies VRScaleHudMeshes): scale IN-GAME only, same as
			{    // the HUD tile scaling — a menu up (mouse shown) leaves the icons at their real size.
				bool inGame = Viewport && !Viewport->bShowWindowsMouse;
				sx = inGame ? VRHudScaleX : 1.0f;
				sy = inGame ? VRHudScaleY : 1.0f;
			}
			VRSetMeshProj(sx, sy);
		}
		else
			VRSetProj(noNearZ ? VRPROJ_WEAPON : VRIsSkyFrame(Frame) ? VRPROJ_SKY : VRPROJ_WORLD);
		VRBeginPostRender();
	}

	FTransTexture* Pts = InPts;
	constexpr INT SceneLimit = (SceneIndexBufferSize/3)*3 + 2;
	constexpr INT PtsMax = SceneVertexBufferSize < SceneLimit ? SceneVertexBufferSize : SceneLimit;
	constexpr INT PtsLimit = (PtsMax/3)*3; // Ensure we not split triangle by partial draw!
	while (NumPts > PtsLimit)
	{
		DrawGouraudTriangles(Frame, Info, Pts, PtsLimit, PolyFlags, DataFlags, Span);
		NumPts -= PtsLimit;
		Pts += PtsLimit;
	}

	if (NumPts < 3) return; // This can apparently happen!!

	Timers.DrawGouraudTriangles.Clock();
	ActiveTimer = &Timers.DrawGouraudTriangles;

	PolyFlags = ApplyPrecedenceRules(PolyFlags);

	if (PolyFlags & PF_Environment)
	{
		FLOAT UScale = Info.UScale * Info.USize * (1.0f / 256.0f);
		FLOAT VScale = Info.VScale * Info.VSize * (1.0f / 256.0f);

		for (INT i = 0; i < NumPts; i++)
			::EnviroMap(Frame, Pts[i], UScale, VScale);
	}

	CachedTexture* tex = Textures->GetTexture(const_cast<FTextureInfo*>(&Info), !!(PolyFlags & PF_Masked));

	SetPipeline(PolyFlags);
	SetDescriptorSet(PolyFlags, tex);

	float UMult = tex->UMult;
	float VMult = tex->VMult;
	int flags = (PolyFlags & (PF_RenderFog | PF_Translucent | PF_Modulated)) == PF_RenderFog ? 16 : 0;
	if ((PolyFlags & (PF_Translucent | PF_Modulated)) == 0 && LightMode == 2) flags |= 32;

#ifdef USE_SSE2
	__m128 mflags = _mm_castsi128_ps(_mm_cvtsi32_si128(flags));
	__m128 maskClearW = _mm_castsi128_ps(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0));
#endif

	auto alloc = ReserveVertices(NumPts, ((NumPts - 2)/3)*3);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

#ifdef USE_SSE2
		if (PolyFlags & PF_Modulated)
		{
			SceneVertex* vertex = vptr;
			__m128 color = _mm_set_ps1(1.0f);
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = &Pts[i];

				__m128 point = _mm_and_ps(_mm_loadu_ps((float*)&P->Point), maskClearW);
				__m128 fog = _mm_loadu_ps((float*)&P->Fog);
				__m128 pos = _mm_or_ps(_mm_shuffle_ps(point, point, _MM_SHUFFLE(2, 1, 0, 3)), mflags);
				__m128 uvzero = _mm_setr_ps(P->U * UMult, P->V * VMult, 0.0f, 0.0f);
				__m128 uv0 = _mm_shuffle_ps(uvzero, fog, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 uv1 = _mm_shuffle_ps(fog, uvzero, _MM_SHUFFLE(1, 0, 1, 0));

				_mm_store_ps((float*)vertex, pos);
				_mm_store_ps((float*)vertex + 4, uv0);
				_mm_store_ps((float*)vertex + 8, uv1);
				_mm_store_ps((float*)vertex + 12, color);
				vertex++;
			}
		}
		else
		{
			SceneVertex* vertex = vptr;
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = &Pts[i];

				__m128 point = _mm_and_ps(_mm_loadu_ps((float*)&P->Point), maskClearW);
				__m128 fog = _mm_loadu_ps((float*)&P->Fog);
				__m128 pos = _mm_or_ps(_mm_shuffle_ps(point, point, _MM_SHUFFLE(2, 1, 0, 3)), mflags);
				__m128 uvzero = _mm_setr_ps(P->U * UMult, P->V * VMult, 0.0f, 0.0f);
				__m128 uv0 = _mm_shuffle_ps(uvzero, fog, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 uv1 = _mm_shuffle_ps(fog, uvzero, _MM_SHUFFLE(1, 0, 1, 0));
				__m128 color = _mm_and_ps(_mm_loadu_ps((float*)&P->Light), maskClearW);
				color = _mm_or_ps(color, _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

				_mm_store_ps((float*)vertex, pos);
				_mm_store_ps((float*)vertex + 4, uv0);
				_mm_store_ps((float*)vertex + 8, uv1);
				_mm_store_ps((float*)vertex + 12, color);
				vertex++;
			}
		}
#else
		if (PolyFlags & PF_Modulated)
		{
			SceneVertex* vertex = vptr;
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = &Pts[i];
				vertex->Flags = flags;
				vertex->Position.x = P->Point.X;
				vertex->Position.y = P->Point.Y;
				vertex->Position.z = P->Point.Z;
				vertex->TexCoord.s = P->U * UMult;
				vertex->TexCoord.t = P->V * VMult;
				vertex->TexCoord2.s = P->Fog.X;
				vertex->TexCoord2.t = P->Fog.Y;
				vertex->TexCoord3.s = P->Fog.Z;
				vertex->TexCoord3.t = P->Fog.W;
				vertex->TexCoord4.s = 0.0f;
				vertex->TexCoord4.t = 0.0f;
				vertex->Color.r = 1.0f;
				vertex->Color.g = 1.0f;
				vertex->Color.b = 1.0f;
				vertex->Color.a = 1.0f;
				vertex++;
			}
		}
		else
		{
			SceneVertex* vertex = vptr;
			for (INT i = 0; i < NumPts; i++)
			{
				FTransTexture* P = &Pts[i];
				vertex->Flags = flags;
				vertex->Position.x = P->Point.X;
				vertex->Position.y = P->Point.Y;
				vertex->Position.z = P->Point.Z;
				vertex->TexCoord.s = P->U * UMult;
				vertex->TexCoord.t = P->V * VMult;
				vertex->TexCoord2.s = P->Fog.X;
				vertex->TexCoord2.t = P->Fog.Y;
				vertex->TexCoord3.s = P->Fog.Z;
				vertex->TexCoord3.t = P->Fog.W;
				vertex->TexCoord4.s = 0.0f;
				vertex->TexCoord4.t = 0.0f;
				vertex->Color.r = P->Light.X;
				vertex->Color.g = P->Light.Y;
				vertex->Color.b = P->Light.Z;
				vertex->Color.a = 1.0f;
				vertex++;
			}
		}
#endif

		bool mirror = (Frame->Mirror == -1.0);

		size_t vstart = vpos;
		size_t vcount = NumPts;
		size_t icount = 0;

		if (PolyFlags & PF_TwoSided)
		{
			for (uint32_t i = 2; i < vcount; i += 3)
			{
				// If outcoded, skip it.
				if (Pts[i - 2].Flags & Pts[i - 1].Flags & Pts[i].Flags)
					continue;

				*(iptr++) = vstart + i;
				*(iptr++) = vstart + i - 1;
				*(iptr++) = vstart + i - 2;
				icount += 3;
			}
		}
		else
		{
			for (uint32_t i = 2; i < vcount; i += 3)
			{
				// If outcoded, skip it.
				if (Pts[i - 2].Flags & Pts[i - 1].Flags & Pts[i].Flags)
					continue;

				bool backface = FTriple(Pts[i - 2].Point, Pts[i - 1].Point, Pts[i].Point) <= 0.0;
				if (mirror == backface)
				{
					*(iptr++) = vstart + i - 2;
					*(iptr++) = vstart + i - 1;
					*(iptr++) = vstart + i;
					icount += 3;
				}
			}
		}

		UseVertices(vcount, icount);
	}

	Stats.GouraudPolygons++;
	Timers.DrawGouraudTriangles.Unclock();
	ActiveTimer = nullptr;

	unguardSlow;
}

#endif

#if defined(OLDUNREAL469SDK)

void UD3D11RenderDevice::DrawTileList(const FSceneNode* Frame, const FTextureInfo& Info, const FTileRect* Tiles, INT NumTiles, FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags)
{
	guardSlow(UD3D11RenderDevice::DrawTileList);

	Timers.DrawTile.Clock();
	ActiveTimer = &Timers.DrawTile;

	// Tile lists are HUD text/bars, never the crosshair; NoNearZ -> HUD overlay, else world.
	if (VRRetaining)
	{
		VRHudMeshArm = false; // a tile ends the post-ClearZ mesh run
		VRSetProj((GUglyHackFlags & HACKFLAGS_NoNearZ) ? VRPROJ_HUDOVERLAY : VRPROJ_WORLD);
		VRBeginPostRender();
	}

	// stijn: fix for invisible actor icons in ortho viewports
	if (GIsEditor && Frame->Viewport->Actor && (Frame->Viewport->IsOrtho() || Abs(Z) <= SMALL_NUMBER))
	{
		Z = 1.f;
	}

	PolyFlags = ApplyPrecedenceRules(PolyFlags);

	CachedTexture* tex = Textures->GetTexture(const_cast<FTextureInfo*>(&Info), (PolyFlags & PF_Masked) || 
		(Info.Texture && (Info.Texture->PolyFlags & PF_Masked)));
	float UMult = tex->UMult;
	float VMult = tex->VMult;
	int curclampmode = -1;

	SetPipeline(PolyFlags);

	float r, g, b, a;
	if (PolyFlags & PF_Modulated)
	{
		r = 1.0f;
		g = 1.0f;
		b = 1.0f;
	}
	else
	{
		r = Color.X;
		g = Color.Y;
		b = Color.Z;
	}
	a = 1.0f;

	// VR: push only true HUD tiles (drawn at the near Z~1) to the convergence
	// depth. Sprites/effects come through DrawTile too but at their real world Z —
	// leave those alone or they'd sit at HUD distance and misalign with the world.
	// Flat HUD (Z~1) -> convergence depth (PostRender start, depth clear and the screen
	// frame are handled by VRBeginPostRender above). Gameplay HUD vs free-mouse UI depth.
	bool vrHud = false;
	if (VRRetaining && (GUglyHackFlags & HACKFLAGS_PostRender) && Abs(1.0f - Z) <= SMALL_NUMBER)
	{
		Z = (Viewport && !Viewport->bShowWindowsMouse) ? VRHudDepth : VRUIDepth;
		vrHud = true;
	}

	float rfx2z = RFX2 * Z;
	float rfy2z = RFY2 * Z;
	// HUD scale: gameplay only (mouse captured) so menu/UI is untouched. Baked into the
	// tile verts, so it currently affects the desktop mirror too (see VR_NOTES).
	if (vrHud && Viewport && !Viewport->bShowWindowsMouse)
	{
		rfx2z *= VRHudScaleX;
		rfy2z *= VRHudScaleY;
	}

	for (INT i = 0; i < NumTiles; i++)
	{
		auto alloc = ReserveVertices(4, 6);
		if (!alloc.vptr)
			break;

		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

		FLOAT X = Tiles[i].X;
		FLOAT Y = Tiles[i].Y;
		FLOAT XL = Tiles[i].XL;
		FLOAT YL = Tiles[i].YL;
		FLOAT U = Tiles[i].U;
		FLOAT V = Tiles[i].V;
		FLOAT UL = Tiles[i].UL;
		FLOAT VL = Tiles[i].VL;

		float u0 = U * UMult;
		float v0 = V * VMult;
		float u1 = (U + UL) * UMult;
		float v1 = (V + VL) * VMult;
		int clamp = (u0 >= 0.0f && u1 <= 1.00001f && v0 >= 0.0f && v1 <= 1.00001f);
		if (clamp != curclampmode)
		{
			SetDescriptorSet(PolyFlags, tex, clamp);
			curclampmode = clamp;
		}

		if (SceneBuffers.Multisample > 1)
		{
			XL = std::floor(X + XL + 0.5f);
			YL = std::floor(Y + YL + 0.5f);
			X = std::floor(X + 0.5f);
			Y = std::floor(Y + 0.5f);
			XL = XL - X;
			YL = YL - Y;
		}

		X -= Frame->FX2;
		Y -= Frame->FY2;
		XL += X;
		YL += Y;
		U *= UMult;
		UL = U + UL * UMult;
		V *= VMult;
		VL = V + VL * VMult;

		vptr[0].Flags = 0;
		vptr[0].Position.x = rfx2z * X;
		vptr[0].Position.y = rfy2z * Y;
		vptr[0].Position.z = Z;
		vptr[0].TexCoord.s = U;
		vptr[0].TexCoord.t = V;
		vptr[0].TexCoord2.s = 0.0f;
		vptr[0].TexCoord2.t = 0.0f;
		vptr[0].TexCoord3.s = 0.0f;
		vptr[0].TexCoord3.t = 0.0f;
		vptr[0].TexCoord4.s = 0.0f;
		vptr[0].TexCoord4.t = 0.0f;
		vptr[0].Color.r = r;
		vptr[0].Color.g = g;
		vptr[0].Color.b = b;
		vptr[0].Color.a = a;

		vptr[1].Flags = 0;
		vptr[1].Position.x = rfx2z * XL;
		vptr[1].Position.y = rfy2z * Y;
		vptr[1].Position.z = Z;
		vptr[1].TexCoord.s = UL;
		vptr[1].TexCoord.t = V;
		vptr[1].TexCoord2.s = 0.0f;
		vptr[1].TexCoord2.t = 0.0f;
		vptr[1].TexCoord3.s = 0.0f;
		vptr[1].TexCoord3.t = 0.0f;
		vptr[1].TexCoord4.s = 0.0f;
		vptr[1].TexCoord4.t = 0.0f;
		vptr[1].Color.r = r;
		vptr[1].Color.g = g;
		vptr[1].Color.b = b;
		vptr[1].Color.a = a;

		vptr[2].Flags = 0;
		vptr[2].Position.x = rfx2z * XL;
		vptr[2].Position.y = rfy2z * YL;
		vptr[2].Position.z = Z;
		vptr[2].TexCoord.s = UL;
		vptr[2].TexCoord.t = VL;
		vptr[2].TexCoord2.s = 0.0f;
		vptr[2].TexCoord2.t = 0.0f;
		vptr[2].TexCoord3.s = 0.0f;
		vptr[2].TexCoord3.t = 0.0f;
		vptr[2].TexCoord4.s = 0.0f;
		vptr[2].TexCoord4.t = 0.0f;
		vptr[2].Color.r = r;
		vptr[2].Color.g = g;
		vptr[2].Color.b = b;
		vptr[2].Color.a = a;

		vptr[3].Flags = 0;
		vptr[3].Position.x = rfx2z * X;
		vptr[3].Position.y = rfy2z * YL;
		vptr[3].Position.z = Z;
		vptr[3].TexCoord.s = U;
		vptr[3].TexCoord.t = VL;
		vptr[3].TexCoord2.s = 0.0f;
		vptr[3].TexCoord2.t = 0.0f;
		vptr[3].TexCoord3.s = 0.0f;
		vptr[3].TexCoord3.t = 0.0f;
		vptr[3].TexCoord4.s = 0.0f;
		vptr[3].TexCoord4.t = 0.0f;
		vptr[3].Color.r = r;
		vptr[3].Color.g = g;
		vptr[3].Color.b = b;
		vptr[3].Color.a = a;

		iptr[0] = vpos;
		iptr[1] = vpos + 1;
		iptr[2] = vpos + 2;
		iptr[3] = vpos;
		iptr[4] = vpos + 2;
		iptr[5] = vpos + 3;

		UseVertices(4, 6);
	}

	Stats.Tiles++;
	Timers.DrawTile.Unclock();
	ActiveTimer = nullptr;

	unguardSlow;
}

#endif

void UD3D11RenderDevice::DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags)
{
	guardSlow(UD3D11RenderDevice::DrawTile);

	Timers.DrawTile.Clock();
	ActiveTimer = &Timers.DrawTile;

	// Overlays (RenderOverlays: HACKFLAGS_NoNearZ) are drawn near, no near clip — a Z~1
	// tile with full eye IPD is shoved off-screen (visible on the mono mirror only). These
	// tiles are custom weapon crosshairs AND HUD (bars/ammo). Only the actual crosshair
	// (center + small) converges at VRCrosshairDepth; off-centre HUD stays at VRHudDepth,
	// else the weapon bar rides the aim point.
	if (VRRetaining)
	{
		VRHudMeshArm = false; // a tile ends the post-ClearZ mesh run
		if (GUglyHackFlags & HACKFLAGS_NoNearZ)
		{
			// Centre = crosshair (no size test — sniper/vehicle reticles are huge); off-centre = HUD.
			bool cross = Abs((X + XL * 0.5f) - Frame->FX2) < Frame->FX * 0.06f &&
				Abs((Y + YL * 0.5f) - Frame->FY2) < Frame->FY * 0.06f;
			VRSetProj(cross ? VRPROJ_CROSSHAIR : VRPROJ_HUDOVERLAY);
		}
		else
			VRSetProj(VRPROJ_WORLD);
		VRBeginPostRender();
	}

	// stijn: fix for invisible actor icons in ortho viewports
	if (GIsEditor && Frame->Viewport->Actor && (Frame->Viewport->IsOrtho() || Abs(Z) <= SMALL_NUMBER))
	{
		Z = 1.f;
	}

	PolyFlags = ApplyPrecedenceRules(PolyFlags);

	CachedTexture* tex = Textures->GetTexture(&Info, (PolyFlags & PF_Masked) || 
		(Info.Texture && (Info.Texture->PolyFlags & PF_Masked)));
	float UMult = tex->UMult;
	float VMult = tex->VMult;
	float u0 = U * UMult;
	float v0 = V * VMult;
	float u1 = (U + UL) * UMult;
	float v1 = (V + VL) * VMult;
	bool clamp = (u0 >= 0.0f && u1 <= 1.00001f && v0 >= 0.0f && v1 <= 1.00001f);

	SetPipeline(PolyFlags);
	SetDescriptorSet(PolyFlags, tex, clamp);

	if (SceneBuffers.Multisample > 1)
	{
		XL = std::floor(X + XL + 0.5f);
		YL = std::floor(Y + YL + 0.5f);
		X = std::floor(X + 0.5f);
		Y = std::floor(Y + 0.5f);
		XL = XL - X;
		YL = YL - Y;
	}

	auto alloc = ReserveVertices(4, 6);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

		float r, g, b, a;
		if (PolyFlags & PF_Modulated)
		{
			r = 1.0f;
			g = 1.0f;
			b = 1.0f;
		}
		else
		{
			r = Color.X;
			g = Color.Y;
			b = Color.Z;
		}
		a = 1.0f;

		// Flat HUD (Z~1) -> convergence depth. Sprites (Z != 1) keep their real world Z.
		// Gameplay: centre tiles (the crosshair) get their own depth; free mouse -> UI depth.
		bool vrHud = false;
		if (VRRetaining && (GUglyHackFlags & HACKFLAGS_PostRender) && Abs(1.0f - Z) <= SMALL_NUMBER)
		{
			if (Viewport && !Viewport->bShowWindowsMouse)
			{
				// Crosshair = a tile centred on screen. No size test: sniper scopes and
				// vehicle reticles are huge. Off-centre HUD/text is rejected by position.
				float cx = X + XL * 0.5f;
				float cy = Y + YL * 0.5f;
				bool centre =
					Abs(cx - Frame->FX2) < Frame->FX * 0.06f && Abs(cy - Frame->FY2) < Frame->FY * 0.06f;
				// Bottom zone (weapon bar under the nose): a nearer depth is far easier to fuse
				// there — round lenses clip the lower edge and the nose is close in real life.
				if (centre)
					Z = VRCrosshairDepth;
				else if (VRHudDepthBottom > 0.0f && cy > Frame->FY * VRHudBottomY)
					Z = VRHudDepthBottom;
				else
					Z = VRHudDepth;
			}
			else
			{
				Z = VRUIDepth;
			}
			vrHud = true;
		}

		float rfx2z = RFX2 * Z;
		float rfy2z = RFY2 * Z;
		if (vrHud && Viewport && !Viewport->bShowWindowsMouse) // gameplay only; menu/UI untouched
		{
			rfx2z *= VRHudScaleX;
			rfy2z *= VRHudScaleY;
		}
		X -= Frame->FX2;
		Y -= Frame->FY2;
		XL += X;
		YL += Y;
		U *= UMult;
		UL = U + UL * UMult;
		V *= VMult;
		VL = V + VL * VMult;

		vptr[0].Flags = 0;
		vptr[0].Position.x = rfx2z * X;
		vptr[0].Position.y = rfy2z * Y;
		vptr[0].Position.z = Z;
		vptr[0].TexCoord.s = U;
		vptr[0].TexCoord.t = V;
		vptr[0].TexCoord2.s = 0.0f;
		vptr[0].TexCoord2.t = 0.0f;
		vptr[0].TexCoord3.s = 0.0f;
		vptr[0].TexCoord3.t = 0.0f;
		vptr[0].TexCoord4.s = 0.0f;
		vptr[0].TexCoord4.t = 0.0f;
		vptr[0].Color.r = r;
		vptr[0].Color.g = g;
		vptr[0].Color.b = b;
		vptr[0].Color.a = a;

		vptr[1].Flags = 0;
		vptr[1].Position.x = rfx2z * XL;
		vptr[1].Position.y = rfy2z * Y;
		vptr[1].Position.z = Z;
		vptr[1].TexCoord.s = UL;
		vptr[1].TexCoord.t = V;
		vptr[1].TexCoord2.s = 0.0f;
		vptr[1].TexCoord2.t = 0.0f;
		vptr[1].TexCoord3.s = 0.0f;
		vptr[1].TexCoord3.t = 0.0f;
		vptr[1].TexCoord4.s = 0.0f;
		vptr[1].TexCoord4.t = 0.0f;
		vptr[1].Color.r = r;
		vptr[1].Color.g = g;
		vptr[1].Color.b = b;
		vptr[1].Color.a = a;

		vptr[2].Flags = 0;
		vptr[2].Position.x = rfx2z * XL;
		vptr[2].Position.y = rfy2z * YL;
		vptr[2].Position.z = Z;
		vptr[2].TexCoord.s = UL;
		vptr[2].TexCoord.t = VL;
		vptr[2].TexCoord2.s = 0.0f;
		vptr[2].TexCoord2.t = 0.0f;
		vptr[2].TexCoord3.s = 0.0f;
		vptr[2].TexCoord3.t = 0.0f;
		vptr[2].TexCoord4.s = 0.0f;
		vptr[2].TexCoord4.t = 0.0f;
		vptr[2].Color.r = r;
		vptr[2].Color.g = g;
		vptr[2].Color.b = b;
		vptr[2].Color.a = a;

		vptr[3].Flags = 0;
		vptr[3].Position.x = rfx2z * X;
		vptr[3].Position.y = rfy2z * YL;
		vptr[3].Position.z = Z;
		vptr[3].TexCoord.s = U;
		vptr[3].TexCoord.t = VL;
		vptr[3].TexCoord2.s = 0.0f;
		vptr[3].TexCoord2.t = 0.0f;
		vptr[3].TexCoord3.s = 0.0f;
		vptr[3].TexCoord3.t = 0.0f;
		vptr[3].TexCoord4.s = 0.0f;
		vptr[3].TexCoord4.t = 0.0f;
		vptr[3].Color.r = r;
		vptr[3].Color.g = g;
		vptr[3].Color.b = b;
		vptr[3].Color.a = a;

		iptr[0] = vpos;
		iptr[1] = vpos + 1;
		iptr[2] = vpos + 2;
		iptr[3] = vpos;
		iptr[4] = vpos + 2;
		iptr[5] = vpos + 3;

		UseVertices(4, 6);
	}

	Stats.Tiles++;
	Timers.DrawTile.Unclock();
	ActiveTimer = nullptr;

	unguardSlow;
}

vec4 UD3D11RenderDevice::ApplyInverseGamma(vec4 color)
{
	if (Viewport->IsOrtho())
		return color;
	float brightness = Clamp(Viewport->GetOuterUClient()->Brightness * 2.0, 0.05, 2.99);
	float gammaRed = Max(brightness + GammaOffset + GammaOffsetRed, 0.001f);
	float gammaGreen = Max(brightness + GammaOffset + GammaOffsetGreen, 0.001f);
	float gammaBlue = Max(brightness + GammaOffset + GammaOffsetBlue, 0.001f);
	return vec4(pow(color.r, gammaRed), pow(color.g, gammaGreen), pow(color.b, gammaBlue), color.a);
}

void UD3D11RenderDevice::Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(UD3D11RenderDevice::Draw3DLine);

	P1 = P1.TransformPointBy(Frame->Coords);
	P2 = P2.TransformPointBy(Frame->Coords);
	if (Frame->Viewport->IsOrtho())
	{
		P1.X = (P1.X) / Frame->Zoom + Frame->FX2;
		P1.Y = (P1.Y) / Frame->Zoom + Frame->FY2;
		P1.Z = 1;
		P2.X = (P2.X) / Frame->Zoom + Frame->FX2;
		P2.Y = (P2.Y) / Frame->Zoom + Frame->FY2;
		P2.Z = 1;

		if (Abs(P2.X - P1.X) + Abs(P2.Y - P1.Y) >= 0.2)
		{
			Draw2DLine(Frame, Color, LineFlags, P1, P2);
		}
		else if (Frame->Viewport->Actor->OrthoZoom < ORTHO_LOW_DETAIL)
		{
			Draw2DPoint(Frame, Color, LINE_None, P1.X - 1, P1.Y - 1, P1.X + 1, P1.Y + 1, P1.Z);
		}
	}
	else
	{
#if defined(OLDUNREAL469SDK)
		bool occlude = !!(LineFlags & LINE_DepthCued);
#else
		bool occlude = OccludeLines;
#endif
		SetPipeline(&ScenePass.LinePipeline[occlude]);
		SetDescriptorSet(PF_Highlighted);
		vec4 color = ApplyInverseGamma(vec4(Color.X, Color.Y, Color.Z, 1.0f));

		auto alloc = ReserveVertices(2, 2);
		if (alloc.vptr)
		{
			SceneVertex* vptr = alloc.vptr;
			uint32_t* iptr = alloc.iptr;
			uint32_t vpos = alloc.vpos;

			vptr[0] = { 0, vec3(P1.X, P1.Y, P1.Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };
			vptr[1] = { 0, vec3(P2.X, P2.Y, P2.Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };

			iptr[0] = vpos;
			iptr[1] = vpos + 1;

			UseVertices(2, 2);
		}
	}

	unguard;
}

void UD3D11RenderDevice::Draw2DClippedLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(UD3D11RenderDevice::Draw2DClippedLine);
	URenderDevice::Draw2DClippedLine(Frame, Color, LineFlags, P1, P2);
	unguard;
}

void UD3D11RenderDevice::Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
	guard(UD3D11RenderDevice::Draw2DLine);

#if defined(OLDUNREAL469SDK)
	bool occlude = !!(LineFlags & LINE_DepthCued);
#else
	bool occlude = OccludeLines;
#endif
	SetPipeline(&ScenePass.LinePipeline[occlude]);
	SetDescriptorSet(PF_Highlighted);
	vec4 color = ApplyInverseGamma(vec4(Color.X, Color.Y, Color.Z, 1.0f));

	auto alloc = ReserveVertices(2, 2);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

		vptr[0] = { 0, vec3(RFX2 * P1.Z * (P1.X - Frame->FX2), RFY2 * P1.Z * (P1.Y - Frame->FY2), P1.Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };
		vptr[1] = { 0, vec3(RFX2 * P2.Z * (P2.X - Frame->FX2), RFY2 * P2.Z * (P2.Y - Frame->FY2), P2.Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };

		iptr[0] = vpos;
		iptr[1] = vpos + 1;

		UseVertices(2, 2);
	}

	unguard;
}

void UD3D11RenderDevice::Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
	guard(UD3D11RenderDevice::Draw2DPoint);

	// Hack to fix UED selection problem with selection brush
	if (GIsEditor) Z = 1.0f;

#if defined(OLDUNREAL469SDK)
	bool occlude = !!(LineFlags & LINE_DepthCued);
#else
	bool occlude = OccludeLines;
#endif
	SetPipeline(&ScenePass.PointPipeline[occlude]);
	SetDescriptorSet(PF_Highlighted);
	vec4 color = ApplyInverseGamma(vec4(Color.X, Color.Y, Color.Z, 1.0f));

	auto alloc = ReserveVertices(4, 6);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

		vptr[0] = { 0, vec3(RFX2 * Z * (X1 - Frame->FX2 - 0.5f), RFY2 * Z * (Y1 - Frame->FY2 - 0.5f), Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };
		vptr[1] = { 0, vec3(RFX2 * Z * (X2 - Frame->FX2 + 0.5f), RFY2 * Z * (Y1 - Frame->FY2 - 0.5f), Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };
		vptr[2] = { 0, vec3(RFX2 * Z * (X2 - Frame->FX2 + 0.5f), RFY2 * Z * (Y2 - Frame->FY2 + 0.5f), Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };
		vptr[3] = { 0, vec3(RFX2 * Z * (X1 - Frame->FX2 - 0.5f), RFY2 * Z * (Y2 - Frame->FY2 + 0.5f), Z), vec2(0.0f), vec2(0.0f), vec2(0.0f), vec2(0.0f), color };

		iptr[0] = vpos;
		iptr[1] = vpos + 1;
		iptr[2] = vpos + 2;
		iptr[3] = vpos;
		iptr[4] = vpos + 2;
		iptr[5] = vpos + 3;

		UseVertices(4, 6);
	}

	unguard;
}

void UD3D11RenderDevice::ClearZ(FSceneNode* Frame)
{
	guard(UD3D11RenderDevice::ClearZ);

	if (VRRetaining)
	{
		// Retain mode draws nothing yet; record the boundary so the eye/mirror replay
		// clears depth here (e.g. after the skybox, so the world draws over it).
		AddDrawBatch();
		VRClearZAt.push_back(QueuedBatches.size());
		// A ClearZ during the HUD phase is an explicit "draw the following 3D mesh on top" signal:
		// a mod's rotated actor mesh (radar icon). Arm HUD-mesh mode so the next Gouraud replays at
		// HUD scale/depth. The menu player-mesh preview uses ClearZ=False, so it is NOT armed here —
		// it is detected as a sub-view instead (see SetSceneNode/DrawGouraud VRInSubView).
		if (VRScaleHudMeshes && (GUglyHackFlags & HACKFLAGS_PostRender))
			VRHudMeshArm = true;
		return;
	}

	DrawBatches();

	Context->ClearDepthStencilView(SceneBuffers.DepthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	unguard;
}

void UD3D11RenderDevice::GetStats(TCHAR* Result)
{
	guard(UD3D11RenderDevice::GetStats);
	Result[0] = 0;
	unguard;
}

void UD3D11RenderDevice::ReadPixels(FColor* Pixels)
{
	guard(UD3D11RenderDevice::ReadPixels);

	UnmapVertices();

	ID3D11Texture2D* stagingTexture = nullptr;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.BindFlags = 0;
	texDesc.Width = SceneBuffers.Width;
	texDesc.Height = SceneBuffers.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	HRESULT result = Device->CreateTexture2D(&texDesc, nullptr, &stagingTexture);
	if (FAILED(result))
		return;
	SetDebugName(stagingTexture, "ReadPixels.StagingTexture");

	if (GammaCorrectScreenshots)
	{
		ID3D11RenderTargetView* rtvs[1] = { SceneBuffers.PPImageView[1].get() };
		Context->OMSetRenderTargets(1, rtvs, nullptr);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = CurrentSizeX;
		viewport.Height = CurrentSizeY;
		viewport.MaxDepth = 1.0f;
		Context->RSSetViewports(1, &viewport);

		PresentPushConstants pushconstants = GetPresentPushConstants();

		// Select present shader based on what the user is actually using
		int presentShader = 0;
		if (ActiveHdr) presentShader |= 1;
		if (GammaMode == 1) presentShader |= 2;
		if (pushconstants.Brightness != 0.0f || pushconstants.Contrast != 1.0f || pushconstants.Saturation != 1.0f) presentShader |= (Clamp(GrayFormula, 0, 2) + 1) << 2;

		UINT stride = sizeof(vec2);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get() };
		ID3D11Buffer* cbs[1] = { PresentPass.PresentConstantBuffer.get() };
		ID3D11ShaderResourceView* psResources[] = { SceneBuffers.PPImageShaderView[0], PresentPass.DitherTextureView };
		Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
		Context->IASetInputLayout(PresentPass.PPStepLayout);
		Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
		Context->RSSetState(PresentPass.RasterizerState);
		Context->PSSetShader(PresentPass.Present[presentShader], nullptr, 0);
		Context->PSSetConstantBuffers(0, 1, cbs);
		Context->PSSetShaderResources(0, 2, psResources);
		Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
		Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);
		Context->UpdateSubresource(PresentPass.PresentConstantBuffer, 0, nullptr, &pushconstants, 0, 0);
		Context->Draw(6, 0);

		Context->CopyResource(stagingTexture, SceneBuffers.PPImage[1]);
	}
	else
	{
		Context->CopyResource(stagingTexture, SceneBuffers.PPImage[0]);
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	result = Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
	if (SUCCEEDED(result))
	{
		uint8_t* srcpixels = (uint8_t*)mapped.pData;
		int w = CurrentSizeX;
		int h = CurrentSizeY;
		void* data = Pixels;

		for (int y = 0; y < h; y++)
		{
			int desty = GammaCorrectScreenshots ? y : (h - y - 1);
			uint8_t* dest = (uint8_t*)data + desty * w * 4;
			uint16_t* src = (uint16_t*)(srcpixels + y * mapped.RowPitch);
			for (int x = 0; x < w; x++)
			{
				float red = halfToFloatSimple(*(src++));
				float green = halfToFloatSimple(*(src++));
				float blue = halfToFloatSimple(*(src++));
				float alpha = halfToFloatSimple(*(src++));

				dest[0] = (int)clamp(std::round(blue * 255.0f), 0.0f, 255.0f);
				dest[1] = (int)clamp(std::round(green * 255.0f), 0.0f, 255.0f);
				dest[2] = (int)clamp(std::round(red * 255.0f), 0.0f, 255.0f);
				dest[3] = (int)clamp(std::round(alpha * 255.0f), 0.0f, 255.0f);
				dest += 4;
			}
		}

		Context->Unmap(stagingTexture, 0);
	}

	stagingTexture->Release();

	if (IsLocked)
		MapVertices(false);

	unguard;
}

void UD3D11RenderDevice::EndFlash()
{
	guard(UD3D11RenderDevice::EndFlash);
	if (FlashScale != FPlane(0.5f, 0.5f, 0.5f, 0.0f) || FlashFog != FPlane(0.0f, 0.0f, 0.0f, 0.0f))
	{
		DrawBatches();

		vec4 color(FlashFog.X, FlashFog.Y, FlashFog.Z, 1.0f - Min(FlashScale.X * 2.0f, 1.0f));
		vec2 zero2(0.0f);

		SceneConstants.ObjectToProjection = mat4::identity();
		SceneConstants.NearClip = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		Context->UpdateSubresource(ScenePass.ConstantBuffer, 0, nullptr, &SceneConstants, 0, 0);

		VRSetProj(VRPROJ_FLASH); // VR: the quad is already in clip space -> replay with identity, not an eye frustum

		SetPipeline(PF_Highlighted);
		SetDescriptorSet(0);

		auto alloc = ReserveVertices(4, 6);
		if (alloc.vptr)
		{
			SceneVertex* vptr = alloc.vptr;
			uint32_t* iptr = alloc.iptr;
			uint32_t vpos = alloc.vpos;

			vptr[0] = { 0, vec3(-1.0f, -1.0f, 0.0f), zero2, zero2, zero2, zero2, color };
			vptr[1] = { 0, vec3(1.0f, -1.0f, 0.0f), zero2, zero2, zero2, zero2, color };
			vptr[2] = { 0, vec3(1.0f,  1.0f, 0.0f), zero2, zero2, zero2, zero2, color };
			vptr[3] = { 0, vec3(-1.0f,  1.0f, 0.0f), zero2, zero2, zero2, zero2, color };

			iptr[0] = vpos;
			iptr[1] = vpos + 1;
			iptr[2] = vpos + 2;
			iptr[3] = vpos;
			iptr[4] = vpos + 2;
			iptr[5] = vpos + 3;

			UseVertices(4, 6);
		}

		VRSetProj(VRPROJ_WORLD); // close/isolate the flash batch, restore default tag

		DrawBatches();
		if (CurrentFrame)
			SetSceneNode(CurrentFrame);
	}
	unguard;
}

void UD3D11RenderDevice::SetSceneNode(FSceneNode* Frame)
{
	guardSlow(UD3D11RenderDevice::SetSceneNode);

	DrawBatches();

	CurrentFrame = Frame;
	Aspect = Frame->FY / Frame->FX;
	RProjZ = (float)appTan(radians(Viewport->Actor->FovAngle) * 0.5);

	// The menu player-mesh preview (UMenuPlayerMeshClient::DrawClippedActor) mutates THIS frame's
	// rect + FovAngle in place and never restores the FOV, poisoning every scene node afterwards
	// (an open dropdown drawn at that leftover fov=30 would bake shrunk). Bake RFX2/RFY2 and the
	// replay zoom off the MAIN (first) node's FOV so the per-tile FOV cancels against the zoom —
	// the HUD stays put whatever the leftover FOV, same as the mono mirror's zoom compensation.
	if (!VRMainCaptured)
	{
		VRMainRProjZ = RProjZ;
		VRMainX = Frame->X;
		VRMainCaptured = true;
	}
	// The menu player-mesh preview (UMenuPlayerMeshClient) renders a Gouraud actor into a sub-region
	// (offset/narrower than the window) at a narrow FOV, via DrawClippedActor with ClearZ=False — so
	// there is no ClearZ to arm HUDMESH. Detect the sub-view by its RECT, tag its mesh HUDMESH, and
	// capture its FOV+width so it can be magnified to fill the virtual screen like the 2D render.
	// The leftover full-rect node shares the poisoned FOV but is NOT the sub-view (keying off FOV
	// would let it clobber the sub width), so key off the rect.
	VRInSubView = VRRetaining && VRMainCaptured && (Frame->XB != 0 || Frame->X < VRMainX);
	if (VRInSubView)
	{
		VRSubRProjZ = RProjZ;
		VRSubX = Frame->X;
	}
	if (VRRetaining)
		RProjZ = VRMainRProjZ; // ignore a poisoned leftover FOV for HUD baking (mono frustum below is unused in retain)
	RFX2 = 2.0f * RProjZ / Frame->FX;
	RFY2 = 2.0f * RProjZ * Aspect / Frame->FY;

	SceneViewport = {};
	SceneViewport.TopLeftX = Frame->XB;
	SceneViewport.TopLeftY = SceneBuffers.Height - Frame->YB - Frame->Y;
	SceneViewport.Width = Frame->X;
	SceneViewport.Height = Frame->Y;
	SceneViewport.MinDepth = 0.1f;
	SceneViewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &SceneViewport);

	SceneConstants.ObjectToProjection = mat4::frustum(-RProjZ, RProjZ, -Aspect * RProjZ, Aspect * RProjZ, 1.0f, 32768.0f, handedness::left, clipzrange::zero_positive_w);
	SceneConstants.NearClip = vec4(Frame->NearClip.X, Frame->NearClip.Y, Frame->NearClip.Z, -Frame->NearClip.W);

	Context->UpdateSubresource(ScenePass.ConstantBuffer, 0, nullptr, &SceneConstants, 0, 0);

	unguardSlow;
}

void UD3D11RenderDevice::PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags)
{
	guard(UD3D11RenderDevice::PrecacheTexture);
	PolyFlags = ApplyPrecedenceRules(PolyFlags);
	Textures->GetTexture(&Info, !!(PolyFlags & PF_Masked));
	unguard;
}

void UD3D11RenderDevice::ClearTextureCache()
{
	Textures->ClearCache();
}

void UD3D11RenderDevice::AddDrawBatch()
{
	if (Batch.SceneIndexStart != SceneIndexPos)
	{
		Batch.SceneIndexEnd = SceneIndexPos;
		Batch.VRProj = VRCurProj;
		Batch.VRMeshSX = VRCurMeshSX;
		Batch.VRMeshSY = VRCurMeshSY;
		QueuedBatches.push_back(Batch);
		Batch.SceneIndexStart = SceneIndexPos;
	}
}

// Tag which eye projection a batch replays with (VRPROJ_*): world, first-person weapon
// (reduced IPD), overlay crosshair / overlay HUD (crosshair vs HUD convergence). Forcing a
// boundary on change keeps each batch on exactly one projection, so no fragile range
// ordering is needed — the replay just groups adjacent same-tag batches.
void UD3D11RenderDevice::VRSetProj(int proj)
{
	if (VRRetaining && proj != VRCurProj)
	{
		AddDrawBatch();
		VRCurProj = proj;
	}
}

// Tag the accumulating batch as a HUD mesh with its own view-space scale. Each preview/radar mesh
// carries its own size (subX/FOV differ per sub-view), so boundary the batch on a scale change too —
// otherwise two previews with different scales would merge and share the last one.
void UD3D11RenderDevice::VRSetMeshProj(float sx, float sy)
{
	if (VRRetaining && (VRCurProj != VRPROJ_HUDMESH || sx != VRCurMeshSX || sy != VRCurMeshSY))
	{
		AddDrawBatch();
		VRCurProj = VRPROJ_HUDMESH;
		VRCurMeshSX = sx;
		VRCurMeshSY = sy;
	}
}

// True when this frame renders a skybox zone (UE1 draws the SkyZone's geometry with the
// frame set to that zone). Sky geometry must replay at zero IPD (infinity). Zones never
// move, so the 64 flags are built once per level: for each zone, its SkyZone actor's zone
// is a sky zone.
bool UD3D11RenderDevice::VRIsSkyFrame(const FSceneNode* Frame)
{
	if (!VRRetaining || !Frame || !Frame->Level)
		return false;
	if (Frame->Level != VRSkyZonesLevel)
	{
		appMemzero(VRSkyZones, sizeof(VRSkyZones));
		for (INT z = 0; z < 64; z++)
		{
			AZoneInfo* za = Frame->Level->GetZoneActor(z);
			if (za && za->SkyZone)
				VRSkyZones[za->SkyZone->Region.ZoneNumber & 63] = true;
		}
		VRSkyZonesLevel = Frame->Level;
	}
	return VRSkyZones[Frame->ZoneNumber & 63];
}

void UD3D11RenderDevice::DrawBatches(bool nextBuffer)
{
	AddDrawBatch();

	// VR retain: accumulate the whole frame into QueuedBatches; the actual
	// rasterisation happens once per eye in RenderVREyes() at Unlock.
	if (VRRetaining)
		return;

	if (ActiveTimer)
		ActiveTimer->Unclock();
	Timers.DrawBatches.Clock();

	UnmapVertices();

	for (const DrawBatchEntry& entry : QueuedBatches)
		DrawEntry(entry);
	QueuedBatches.clear();

	MapVertices(nextBuffer);

	if (nextBuffer)
	{
		SceneVertexPos = 0;
		SceneIndexPos = 0;
		Stats.BuffersUsed++;
	}

	Batch.SceneIndexStart = SceneIndexPos;

	Timers.DrawBatches.Unclock();
	if (ActiveTimer)
		ActiveTimer->Clock();
}

void UD3D11RenderDevice::DrawEntry(const DrawBatchEntry& entry, bool forceNoDepth)
{
	size_t icount = entry.SceneIndexEnd - entry.SceneIndexStart;

	ID3D11ShaderResourceView* views[4] =
	{
		entry.Tex->View,
		entry.Lightmap->View,
		entry.Macrotex->View,
		entry.Detailtex->View
	};

	ID3D11SamplerState* samplers[4] =
	{
		ScenePass.Samplers[entry.TexSamplerMode],
		ScenePass.Samplers[0],
		ScenePass.Samplers[entry.MacrotexSamplerMode],
		ScenePass.Samplers[entry.DetailtexSamplerMode]
	};

	if (SceneViewport.MinDepth != entry.Pipeline->MinDepth || SceneViewport.MaxDepth != entry.Pipeline->MaxDepth)
	{
		SceneViewport.MinDepth = entry.Pipeline->MinDepth;
		SceneViewport.MaxDepth = entry.Pipeline->MaxDepth;
		Context->RSSetViewports(1, &SceneViewport);
	}

	Context->PSSetSamplers(0, 4, samplers);
	Context->PSSetShaderResources(0, 4, views);
	Context->PSSetShader(entry.Pipeline->PixelShader, nullptr, 0);

	Context->OMSetBlendState(entry.Pipeline->BlendState, nullptr, 0xffffffff);
	// HUD range draws with depth test off (painter's order) so coplanar HUD tiles
	// (console background + text) don't z-fight.
	Context->OMSetDepthStencilState(forceNoDepth ? VRNoDepthState.get() : entry.Pipeline->DepthStencilState.get(), 0);

	Context->IASetPrimitiveTopology(entry.Pipeline->PrimitiveTopology);

	Context->DrawIndexed(icount, entry.SceneIndexStart, 0);
	Stats.DrawCalls++;
}

// Resolve + tonemap the current ColorBuffer into an arbitrary render target.
// Same operations as the mono Unlock present block, but retargetable so VR can
// present into each eye's swapchain image and into the desktop mirror.
// ponytail: sRGB eye target may double-apply the shader gamma — known M0 tuning
// item (VR_NOTES.md); wire a no-gamma present variant if the image looks washed.
void UD3D11RenderDevice::RunPresentPass(ID3D11RenderTargetView* output, UINT width, UINT height, float brightnessScale, float brightnessOffset, bool bloom, bool alreadyResolved)
{
	// alreadyResolved: the VR mirror renders straight into PPImage[0] at 1 sample, so there's
	// nothing to resolve/copy and no AA cost. Eyes resolve the MSAA ColorBuffer as usual.
	if (!alreadyResolved)
	{
		if (SceneBuffers.Multisample > 1)
			Context->ResolveSubresource(SceneBuffers.PPImage[0], 0, SceneBuffers.ColorBuffer, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
		else
			Context->CopyResource(SceneBuffers.PPImage[0], SceneBuffers.ColorBuffer);
	}

	if (bloom && Bloom)
		RunBloomPass();

	ID3D11RenderTargetView* rtvs[1] = { output };
	Context->OMSetRenderTargets(1, rtvs, nullptr);

	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);

	PresentPushConstants pushconstants = GetPresentPushConstants(brightnessScale, brightnessOffset);

	int presentShader = 0;
	if (ActiveHdr) presentShader |= 1;
	if (GammaMode == 1) presentShader |= 2;
	if (pushconstants.Brightness != 0.0f || pushconstants.Contrast != 1.0f || pushconstants.Saturation != 1.0f) presentShader |= (Clamp(GrayFormula, 0, 2) + 1) << 2;

	UINT stride = sizeof(vec2);
	UINT offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get() };
	ID3D11ShaderResourceView* psResources[] = { SceneBuffers.PPImageShaderView[0], PresentPass.DitherTextureView };
	ID3D11Buffer* cbs[1] = { PresentPass.PresentConstantBuffer.get() };
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetInputLayout(PresentPass.PPStepLayout);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
	Context->RSSetState(PresentPass.RasterizerState);
	Context->PSSetShader(PresentPass.Present[presentShader], nullptr, 0);
	Context->PSSetConstantBuffers(0, 1, cbs);
	Context->PSSetShaderResources(0, 2, psResources);
	Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
	Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);
	Context->UpdateSubresource(PresentPass.PresentConstantBuffer, 0, nullptr, &pushconstants, 0, 0);
	Context->Draw(6, 0);
}

// Half-SBS: squeeze the full-width SBS staging into the half-width backbuffer. Reuses the present
// pipeline (fullscreen quad + linear sampler) so the viewport does the 2:1 horizontal downscale,
// but with NEUTRAL constants (identity gamma, no colour correct) — the staging already holds the
// fully-processed eye pixels, so this is a pass-through resample, not a second tonemap.
void UD3D11RenderDevice::BlitSbsHalf(ID3D11ShaderResourceView* src, ID3D11RenderTargetView* output, UINT width, UINT height)
{
	ID3D11RenderTargetView* rtvs[1] = { output };
	Context->OMSetRenderTargets(1, rtvs, nullptr);

	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);

	PresentPushConstants pushconstants = {};
	pushconstants.Contrast = 1.0f;
	pushconstants.Saturation = 1.0f;
	pushconstants.Brightness = 0.0f;
	pushconstants.HdrScale = 1.0f;
	pushconstants.GammaCorrection = vec4(1.0f, 1.0f, 1.0f, 1.0f); // pow(c,1) = identity

	UINT stride = sizeof(vec2);
	UINT offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { PresentPass.PPStepVertexBuffer.get() };
	ID3D11ShaderResourceView* psResources[] = { src, PresentPass.DitherTextureView };
	ID3D11Buffer* cbs[1] = { PresentPass.PresentConstantBuffer.get() };
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetInputLayout(PresentPass.PPStepLayout);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->VSSetShader(PresentPass.PPStep, nullptr, 0);
	Context->RSSetState(PresentPass.RasterizerState);
	Context->PSSetShader(PresentPass.Present[0], nullptr, 0); // 0 = no HDR, D3D9 gamma, no colour correct
	Context->PSSetConstantBuffers(0, 1, cbs);
	Context->PSSetShaderResources(0, 2, psResources);
	Context->OMSetDepthStencilState(PresentPass.DepthStencilState, 0);
	Context->OMSetBlendState(PresentPass.BlendState, nullptr, 0xffffffff);
	Context->UpdateSubresource(PresentPass.PresentConstantBuffer, 0, nullptr, &pushconstants, 0, 0);
	Context->Draw(6, 0);
}

// Replay the accumulated frame into ColorBuffer with a given projection. Shared
// by each eye and by the flat desktop pass — the geometry is identical, only the
// projection and target size differ.
// Quaternion helpers (x,y,z,w) for the VR head-look modes.
static vec4 VRQuatConj(const vec4& q) { return vec4(-q.x, -q.y, -q.z, q.w); }
static vec4 VRQuatMul(const vec4& a, const vec4& b)
{
	return vec4(
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

// Bind ColorBuffer + scene geometry state for a replay pass (clear, RT, viewport,
// vertex/index buffers). Call once per eye/mirror, then DrawSceneRange one or more times.
void UD3D11RenderDevice::SetupSceneTarget(uint32_t width, uint32_t height)
{
	FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ID3D11RenderTargetView* views[2] = { SceneBuffers.ColorBufferView, SceneBuffers.HitBufferView };
	Context->ClearRenderTargetView(SceneBuffers.ColorBufferView, clearColor);
	Context->ClearDepthStencilView(SceneBuffers.DepthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	Context->OMSetRenderTargets(2, views, SceneBuffers.DepthBufferView);
	VRReplayDepth = SceneBuffers.DepthBufferView.get(); // depth the in-replay ClearZ points clear

	SceneViewport = {};
	SceneViewport.Width = (float)width;
	SceneViewport.Height = (float)height;
	SceneViewport.MinDepth = 0.1f;
	SceneViewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &SceneViewport);

	// Rebind scene geometry + state (a prior present pass changed IA/VS).
	UINT stride = sizeof(SceneVertex);
	UINT offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { ScenePass.VertexBuffer.get() };
	ID3D11Buffer* cbs[1] = { ScenePass.ConstantBuffer.get() };
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetIndexBuffer(ScenePass.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetInputLayout(ScenePass.InputLayout);
	Context->VSSetShader(ScenePass.VertexShader, nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, cbs);
	Context->RSSetState(ScenePass.RasterizerState[SceneBuffers.Multisample > 1]);
	D3D11_RECT scissor = {};
	scissor.right = (LONG)width;
	scissor.bottom = (LONG)height;
	Context->RSSetScissorRects(1, &scissor);
}

// Like SetupSceneTarget but for the VR desktop mirror: render at 1 sample straight into
// PPImage[0] with a 1-sample depth buffer (no MSAA, no resolve, no bloom later). Only the
// colour target is bound (the hit buffer isn't needed for the mirror; the shader's second
// output is simply discarded). Same geometry/state as the scene pass otherwise.
void UD3D11RenderDevice::SetupSceneTargetMirror(uint32_t width, uint32_t height)
{
	// The previous eye present left PPImageShaderView[0] bound as a PS resource; we're about
	// to bind PPImage[0] as the render target, so unbind it first (avoid a read/write hazard).
	ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
	Context->PSSetShaderResources(0, 2, nullSRV);

	FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Context->ClearRenderTargetView(SceneBuffers.PPImageView[0], clearColor);
	Context->ClearDepthStencilView(SceneBuffers.MirrorDepthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	ID3D11RenderTargetView* views[1] = { SceneBuffers.PPImageView[0] };
	Context->OMSetRenderTargets(1, views, SceneBuffers.MirrorDepthBufferView);
	VRReplayDepth = SceneBuffers.MirrorDepthBufferView.get();

	SceneViewport = {};
	SceneViewport.Width = (float)width;
	SceneViewport.Height = (float)height;
	SceneViewport.MinDepth = 0.1f;
	SceneViewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &SceneViewport);

	UINT stride = sizeof(SceneVertex);
	UINT offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { ScenePass.VertexBuffer.get() };
	ID3D11Buffer* cbs[1] = { ScenePass.ConstantBuffer.get() };
	Context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	Context->IASetIndexBuffer(ScenePass.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetInputLayout(ScenePass.InputLayout);
	Context->VSSetShader(ScenePass.VertexShader, nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, cbs);
	Context->RSSetState(ScenePass.RasterizerState[0]); // 1-sample -> non-MSAA rasterizer
	D3D11_RECT scissor = {};
	scissor.right = (LONG)width;
	scissor.bottom = (LONG)height;
	Context->RSSetScissorRects(1, &scissor);
}

// Replay [begin, end) but clear the depth buffer at each recorded VRClearZAt point inside
// the range — reproduces the engine's mid-frame ClearZ (skybox draws first, then depth is
// wiped so the world renders over it) and the HUD depth clear. VRClearZAt is in frame order.
void UD3D11RenderDevice::DrawSceneWithClears(const mat4& objectToProjection, size_t begin, size_t end)
{
	SceneConstants.ObjectToProjection = objectToProjection;
	SceneConstants.NearClip = vec4(0.0f, 0.0f, 0.0f, 1.0f); // no portal near-clip in VR
	Context->UpdateSubresource(ScenePass.ConstantBuffer, 0, nullptr, &SceneConstants, 0, 0);

	if (end > QueuedBatches.size())
		end = QueuedBatches.size();
	for (size_t i = begin; i < end; i++)
	{
		// Clear depth at each recorded point (engine ClearZ after skybox, and once
		// when the HUD begins) so those layers aren't fought by earlier geometry.
		for (size_t clearAt : VRClearZAt)
			if (clearAt == i)
				Context->ClearDepthStencilView(VRReplayDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		// HUD range: depth test off so coplanar HUD tiles don't z-fight.
		// Flat NoNearZ overlay tiles (crosshair, mod HUD bars like a vehicle health bar) also
		// draw with depth off — they're on-top HUD, and depth-tested they z-fight against the
		// world when the phase that clears depth first (the weapon) isn't drawn (e.g. in menu).
		// 3D meshes drawn in the HUD phase (VRPROJ_HUDMESH: radar icons, the menu player-mesh
		// preview) keep depth so their own polygons self-occlude — only flat tiles/overlays go
		// depth-off.
		int pt = QueuedBatches[i].VRProj;
		bool noDepth = pt != VRPROJ_HUDMESH
			&& ((VRHudStart >= 0 && i >= (size_t)VRHudStart)
				|| pt == VRPROJ_CROSSHAIR || pt == VRPROJ_HUDOVERLAY);
		DrawEntry(QueuedBatches[i], noDepth);
	}
}

// Black border masking everything outside the game FOV. The eye FOV is wider than the
// game FOV, so beyond-FOV geometry (skybox rocks etc.) flickers at the edges. 4 black
// quads at the game-FOV edge (RProjZ) out to well past the eye edge; a central hole
// leaves the game view (and the centred weapon) untouched. Drawn depth-off in the HUD
// range with the world projection, so it rotates with the view. Far Z -> negligible IPD.
// Zoom is handled automatically: verts scale with RProjZ, the projection magnifies by
// zoom = default/RProjZ, so the border lands at the default FOV regardless of zoom.
void UD3D11RenderDevice::InjectVRScreenFrame()
{
	const float Z = 1000.0f;
	float xE = RProjZ * Z;
	float yE = Aspect * RProjZ * Z;
	float xB = xE * 4.0f;
	float yB = yE * 4.0f;

	// Capture the hole (= the visible screen edge) for the SBS-record crop: it measures where
	// THESE exact vertices land through the exact replay matrices, instead of re-deriving them.
	VRHoleX = xE;
	VRHoleY = yE;
	VRHoleZ = Z;

	SetPipeline(PF_Highlighted);
	SetDescriptorSet(0);

	auto alloc = ReserveVertices(16, 24);
	if (!alloc.vptr)
		return;
	SceneVertex* v = alloc.vptr;
	uint32_t* idx = alloc.iptr;
	uint32_t b = alloc.vpos;
	vec2 z2(0.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);

	const float qx0[4] = { -xB,  xE, -xE, -xE }; // left, right, top, bottom
	const float qx1[4] = { -xE,  xB,  xE,  xE };
	const float qy0[4] = { -yB, -yB,  yE, -yB };
	const float qy1[4] = {  yB,  yB,  yB, -yE };
	for (int q = 0; q < 4; q++)
	{
		v[q * 4 + 0] = { 0, vec3(qx0[q], qy0[q], Z), z2, z2, z2, z2, black };
		v[q * 4 + 1] = { 0, vec3(qx1[q], qy0[q], Z), z2, z2, z2, z2, black };
		v[q * 4 + 2] = { 0, vec3(qx1[q], qy1[q], Z), z2, z2, z2, z2, black };
		v[q * 4 + 3] = { 0, vec3(qx0[q], qy1[q], Z), z2, z2, z2, z2, black };
		idx[q * 6 + 0] = b + q * 4 + 0; idx[q * 6 + 1] = b + q * 4 + 1; idx[q * 6 + 2] = b + q * 4 + 2;
		idx[q * 6 + 3] = b + q * 4 + 0; idx[q * 6 + 4] = b + q * 4 + 2; idx[q * 6 + 5] = b + q * 4 + 3;
	}
	UseVertices(16, 24);
}

// First PostRender draw of the frame: close the world, start the HUD range (drawn
// depth-off), clear depth once so HUD sits over the world, and inject the screen frame
// as the first HUD-range batch. Called before the tile's own SetPipeline, so the tile
// pipeline that follows cleanly closes the frame batch.
void UD3D11RenderDevice::VRBeginHudRange()
{
	AddDrawBatch();
	VRHudStart = (int)QueuedBatches.size();
	if (VRClearZBeforeHud)
		VRClearZAt.push_back((size_t)VRHudStart);
	// The border draws with the world projection (a far black edge; must not inherit the
	// triggering draw's tag — a HUD-scaled border would move the visible FOV edge).
	int saved = VRCurProj;
	VRCurProj = VRPROJ_WORLD;
	InjectVRScreenFrame();
	AddDrawBatch();
	VRCurProj = saved;
}

void UD3D11RenderDevice::VRBeginPostRender()
{
	if (VRRetaining && VRHudStart < 0 && (GUglyHackFlags & HACKFLAGS_PostRender))
		VRBeginHudRange();
}

// Windowed VR: the OS hardware cursor isn't visible inside the HMD compositor. When the
// engine is using it (bWindowsMouseAvailable) it draws no software cursor, so we draw the
// real CURRENT UWindow cursor ourselves (Root.MouseWindow.Cursor.tex — arrow/resize/hand,
// changes with context; same texture WindowConsole.DrawMouse would draw), positioned at
// mouse - hotspot exactly like DrawMouse. Injected after the scene batches so it lands in
// the eyes only — the desktop mirror already shows the OS cursor. We only READ engine flags,
// never change them (changing bWindowsMouseAvailable switches the mouse-input mode and
// strands the cursor).
void UD3D11RenderDevice::DrawVRCursor()
{
	if (CurrentFullscreen || !CurrentFrame || !Viewport || !Viewport->Console)
		return;
	if (!Viewport->bShowWindowsMouse)
		return;

	if (!Viewport->bWindowsMouseAvailable)
		return;

	// Resolve the cursor property offsets once per console class (FindField is a string search;
	// the classes never unload mid-game). MouseWindow.Cursor is a WindowCursor STRUCT, so tex/
	// HotX/HotY offsets are relative to the struct base. Commit the cache only on full success.
	if (!VRCursor.TexProp || VRCursor.ConsoleClass != Viewport->Console->GetClass())
	{
		VRCursorRefl c;
		c.ConsoleClass = Viewport->Console->GetClass();
		c.RootProp = FindField<UObjectProperty>(c.ConsoleClass, TEXT("Root"));
		UObject* Root = c.RootProp ? *(UObject**)((BYTE*)Viewport->Console + c.RootProp->Offset) : NULL;
		c.MouseWindowProp = Root ? FindField<UObjectProperty>(Root->GetClass(), TEXT("MouseWindow")) : NULL;
		UObject* MW = c.MouseWindowProp ? *(UObject**)((BYTE*)Root + c.MouseWindowProp->Offset) : NULL;
		c.CursorProp = MW ? FindField<UStructProperty>(MW->GetClass(), TEXT("Cursor")) : NULL;
		if (c.CursorProp)
		{
			c.TexProp = FindField<UObjectProperty>(c.CursorProp->Struct, TEXT("tex"));
			c.HotXProp = FindField<UIntProperty>(c.CursorProp->Struct, TEXT("HotX"));
			c.HotYProp = FindField<UIntProperty>(c.CursorProp->Struct, TEXT("HotY"));
			if (c.TexProp)
				VRCursor = c;
		}
	}
	if (!VRCursor.TexProp)
		return;

	// Fast path: cached offsets only, no search. tex/HotX/HotY re-read each frame (cursor changes).
	UObject* Root = *(UObject**)((BYTE*)Viewport->Console + VRCursor.RootProp->Offset);
	UObject* MW = Root ? *(UObject**)((BYTE*)Root + VRCursor.MouseWindowProp->Offset) : NULL;
	if (!MW)
		return;
	BYTE* base = (BYTE*)MW + VRCursor.CursorProp->Offset;
	UTexture* Tex = Cast<UTexture>(*(UObject**)(base + VRCursor.TexProp->Offset));
	if (!Tex)
		return;

	float mx = (float)Viewport->WindowsMouseX - (float)(VRCursor.HotXProp ? *(INT*)(base + VRCursor.HotXProp->Offset) : 0);
	float my = (float)Viewport->WindowsMouseY - (float)(VRCursor.HotYProp ? *(INT*)(base + VRCursor.HotYProp->Offset) : 0);

	FTextureInfo TexInfo;
	Tex->Lock(TexInfo, Viewport->CurrentTime, -1, this);
	CachedTexture* ctex = Textures->GetTexture(&TexInfo, true); // cursors are masked
	float w = (float)TexInfo.USize;
	float h = (float)TexInfo.VSize;

	// Near depth range so the cursor sits on top regardless of its UI-depth stereo Z (same
	// trick as the FP weapon). WORLD projection so it converges at that Z.
	VRCurProj = VRPROJ_WORLD;
	VRCursorPipeline = *GetPipeline(PF_Masked);
	VRCursorPipeline.MinDepth = 0.0f;
	VRCursorPipeline.MaxDepth = 0.05f;
	SetPipeline(&VRCursorPipeline);
	SetDescriptorSet(PF_Masked, ctex, 1);

	auto alloc = ReserveVertices(4, 6);
	if (alloc.vptr)
	{
		SceneVertex* vptr = alloc.vptr;
		uint32_t* iptr = alloc.iptr;
		uint32_t vpos = alloc.vpos;

		float Z = VRUIDepth;
		float rfx2z = RFX2 * Z;
		float rfy2z = RFY2 * Z;
		float X = mx - CurrentFrame->FX2;
		float Y = my - CurrentFrame->FY2;
		float XL = X + w;
		float YL = Y + h;
		float U = 0.0f, V = 0.0f;
		float UL = w * ctex->UMult;
		float VL = h * ctex->VMult;
		vec2 z2(0.0f);
		vec4 col(1.0f, 1.0f, 1.0f, 1.0f);

		vptr[0] = { 0, vec3(rfx2z * X,  rfy2z * Y,  Z), vec2(U,  V),  z2, z2, z2, col };
		vptr[1] = { 0, vec3(rfx2z * XL, rfy2z * Y,  Z), vec2(UL, V),  z2, z2, z2, col };
		vptr[2] = { 0, vec3(rfx2z * XL, rfy2z * YL, Z), vec2(UL, VL), z2, z2, z2, col };
		vptr[3] = { 0, vec3(rfx2z * X,  rfy2z * YL, Z), vec2(U,  VL), z2, z2, z2, col };

		iptr[0] = vpos;
		iptr[1] = vpos + 1;
		iptr[2] = vpos + 2;
		iptr[3] = vpos;
		iptr[4] = vpos + 2;
		iptr[5] = vpos + 3;
		UseVertices(4, 6);
	}
	Tex->Unlock(TexInfo);
}

// Rasterise the frame accumulated in QueuedBatches once per eye (view-space
// geometry is identical for both eyes; only the projection differs), present
// each eye into its OpenXR swapchain image, then render a FLAT mono view for the
// desktop mirror so on-screen text is readable and mouse hit-testing lines up.
// Should the flat desktop mirror render this frame? VRMirrorMode: 0=off, 1=only when the
// headset is removed (default), 2=also in menu (mouse free), 3=always, 4=SBS record (no mono
// replay — handled separately in RenderVREyes; >=3 here doubles as its HDR fallback). The
// headset-worn poll (VR->IsWorn(), which combines session focus + user presence) is throttled
// to ~once a second so these checks never cost per-frame.
bool UD3D11RenderDevice::VRWantMirror()
{
	if (VRMirrorMode == 0)
		return false;
	if (VRMirrorMode >= 3)
		return true;

	if (VR)
	{
		DOUBLE now = appSecondsNew();
		if (now - VRWornCheck > 1.0)
		{
			VRWorn = VR->IsWorn();
			VRWornCheck = now;
		}
	}
	bool menu = Viewport && Viewport->bShowWindowsMouse;
	// 1 = removed only; 2 = removed or in menu.
	return !VRWorn || (VRMirrorMode == 2 && menu);
}

// Project a view-space point through a projection matrix to pixel coordinates (top-left origin).
// Used to find the virtual-screen rectangle inside an eye image for the SBS-record crop: the same
// numeric path the GPU takes, so no hand-derived NDC formulas to get sign conventions wrong in.
static void VRProjectToPixel(const mat4& P, float x, float y, float z, float ew, float eh, float& px, float& py)
{
	float cx = P[0] * x + P[4] * y + P[8] * z + P[12];
	float cy = P[1] * x + P[5] * y + P[9] * z + P[13];
	float cw = P[3] * x + P[7] * y + P[11] * z + P[15];
	if (cw == 0.0f) cw = 1.0f;
	px = (cx / cw * 0.5f + 0.5f) * ew;
	// Eye texture rows run TOP-DOWN opposite to clip-space y (the present pass flips Y): verified
	// by pixel scan (measured border top 325 == eh - projected 637). So NDC.y -> texture row is
	// (y*0.5+0.5), NOT (0.5-y*0.5). X is not flipped (measured left/right matched).
	py = (cy / cw * 0.5f + 0.5f) * eh;
}

void UD3D11RenderDevice::RenderVREyes()
{
	guard(UD3D11RenderDevice::RenderVREyes);

	// Report a full VR vertex buffer (primitives silently dropped), throttled to ~once per
	// 2s so a consistently heavy scene doesn't spam the log every frame.
	if (VRDropped > 0)
	{
		DOUBLE now = appSecondsNew();
		if (now - VRLastDropLog > 2.0)
		{
			debugf(TEXT("D3D11Drv VR: scene exceeded the VR vertex buffer, dropped %d primitives since last report (raise cap or lower detail)"), VRDropped);
			VRLastDropLog = now;
			VRDropped = 0;
		}
	}

	AddDrawBatch();                          // flush the frame's tail batch
	size_t sceneBatches = QueuedBatches.size();
	DrawVRCursor();                          // real UWindow cursor, eyes only (mirror keeps the OS cursor)
	AddDrawBatch();                          // close the cursor batch
	UnmapVertices();                         // can't draw from a mapped dynamic buffer

	uint32_t ew = 0, eh = 0;
	VR->GetEyeResolution(ew, eh);
	float scale = VRWorldScale;

	bool sbs = VRMirrorMode == 4 && !ActiveHdr && ew && eh;
	bool sbsHalf = sbs && VRSBSHalf; // half-SBS: eyes squeezed to half width (backbuffer = 1 eye wide)

	// Weapon zoom (sniper etc.): the game shrinks FovAngle to magnify. We keep the
	// fixed XR eye FOV, so reproduce the magnification by scaling the eye projection
	// by tan(DefaultFOV/2)/tan(FovAngle/2). VRMainRProjZ = tan(main FovAngle/2) — the MAIN
	// scene node, not a leftover sub-view FOV (DrawClippedActor menu preview never restores it).
	// Scaling clip.xy also cancels the FOV-driven narrowing of HUD tiles (RFX2), so the
	// HUD stays put — same net behaviour as the mono desktop mirror.
	float defFov = 90.0f;
	if (Viewport && Viewport->Actor && Viewport->Actor->DefaultFOV > 1.0f)
		defFov = Viewport->Actor->DefaultFOV;
	float rprojDefault = (float)appTan(radians(defFov) * 0.5);
	float zoom = (VRMainRProjZ > 0.0001f) ? (rprojDefault / VRMainRProjZ) : 1.0f;
	if (zoom < 1.0f)
		zoom = 1.0f;

	// SBS record: keep the backbuffer at the wanted SBS size (2 x per-eye screen crop, measured
	// last frame in the block after the eye loop — the crop needs both eyes and is measured from
	// the rendered border, not modelled). Zero until the first measured frame (no copy that
	// frame). See UpdateSwapChain, which reads VRSBSSize. Resize only on a size change; cheap
	// compare otherwise (the mode can be toggled and eye resolution isn't known at SetRes).
	{
		int wantW = (sbs && VRSBSSizeX > 0) ? VRSBSSizeX : CurrentSizeX;
		int wantH = (sbs && VRSBSSizeY > 0) ? VRSBSSizeY : CurrentSizeY;
		if (BackBufferSizeX != wantW || BackBufferSizeY != wantH)
		{
			ReleaseSwapChainResources();
			UpdateSwapChain(false); // swapchain only — the scene buffers are eye-sized mid-frame
		}
	}

	// Half-SBS: the eyes copy into a full-width (2 x eye) staging texture, which is then squeezed to
	// the half-width backbuffer. Sized to the frozen crop (from the previous frame); recreated only
	// on a size change (VRResScale). Independent of the swapchain, so it survives ReleaseSwapChainResources.
	if (sbsHalf && VRSbsCropH > 0)
	{
		int sw = (int)ew * 2, sh = VRSbsCropH;
		if (SbsStagingW != sw || SbsStagingH != sh)
		{
			SbsStaging.reset();
			SbsStagingView.reset();
			SbsStagingW = SbsStagingH = 0;
			D3D11_TEXTURE2D_DESC td = {};
			td.Width = sw; td.Height = sh; td.MipLevels = 1; td.ArraySize = 1;
			td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			td.SampleDesc.Count = 1;
			td.Usage = D3D11_USAGE_DEFAULT;
			td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			if (SUCCEEDED(Device->CreateTexture2D(&td, nullptr, SbsStaging.TypedInitPtr())) &&
				SUCCEEDED(Device->CreateShaderResourceView(SbsStaging, nullptr, SbsStagingView.TypedInitPtr())))
			{
				SbsStagingW = sw; SbsStagingH = sh;
			}
		}
	}
	float sbsRects[2][4] = {}; // per eye this frame: left/top/width/height of the screen rect in eye pixels

	// Head-look. The whole rendered frame is a virtual screen; rotate it by the head
	// orientation. Recenter reference on console request or when leaving a menu.
	bool menuUp = Viewport && Viewport->bShowWindowsMouse;
	float hq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	VR->GetHeadOrientation(hq);
	vec4 headNow(hq[0], hq[1], hq[2], hq[3]);
	if (VRRecenterPending || (VRWasMenu && !menuUp))
	{
		VRHeadRef = headNow;
		VRRecenterPending = false;
	}
	VRWasMenu = menuUp;

	mat4 lookInv = mat4::identity();
	if (VRLookMode == 0)
	{
		// Delta from the reference. Convert XR (right-handed, +y up, -z fwd) to the
		// renderer's view space (left-handed, +y down, +z fwd) = a 180-degree rotation
		// about X, which maps the quaternion (x,y,z,w) -> (x,-y,-z,w).
		vec4 dq = VRQuatMul(headNow, VRQuatConj(VRHeadRef));
		mat4 headRot = mat4::quaternion(dq.x, -dq.y, -dq.z, dq.w);

		if (menuUp)
		{
			if (sbs)
			{
				// SBS record menu: keep yaw/pitch look-around (corner UI is unreachable
				// otherwise) but with the roll REMOVED — the recording must stay level.
				// Rebuild the head basis from its forward axis and the level up axis; the
				// basis up column is +y at rest (headRot == identity), so level up is +y.
				vec3 fwd = normalize(vec3(headRot[8], headRot[9], headRot[10]));
				vec3 upLevel(0.0f, 1.0f, 0.0f);
				vec3 right = cross(upLevel, fwd);
				float rlen = length(right);
				if (rlen > 0.001f)
				{
					right = right / rlen;
					vec3 up = cross(fwd, right);
					// Inverse (transpose) of the no-roll head basis: rows = right/up/fwd.
					lookInv[0] = right.x; lookInv[4] = right.y; lookInv[8]  = right.z;
					lookInv[1] = up.x;    lookInv[5] = up.y;    lookInv[9]  = up.z;
					lookInv[2] = fwd.x;   lookInv[6] = fwd.y;   lookInv[10] = fwd.z;
				}
				else
					lookInv = mat4::transpose(headRot); // straight up/down: roll undefined, momentary
			}
			else
			{
				// Menu: full head orientation, so corner UI can be looked at.
				lookInv = mat4::transpose(headRot);
			}
		}
		else if (!sbs)
		{
			// Gameplay: roll only (head tilt) — never reveals unrendered edges. NOT in SBS
			// record mode: the counter-roll would tilt the recorded frames with every head
			// tilt, and two rolled eye views are unwatchable as 3D video — the recording
			// stays level (lookInv identity).
			// Measure
			// roll as the lean of the reference up-axis within the head's right/up basis
			// (right.y vs up.y). This isolates roll from pitch and yaw for |pitch|<90.
			// Near pitch +-90 both terms are cos(pitch)*{sin,cos}(roll) -> 0: roll is
			// ambiguous (gimbal lock) and flips by pi through the pole, so head-yaw there
			// leaks into a sudden weapon roll/disparity jump. mag = |cos(pitch)|; fade the
			// roll out over the last ~15 degrees to vertical so it can't jump or flip.
			float rightY = headRot[1];
			float upY = headRot[5];
			float roll = atan2f(rightY, upY);
			float rollWeight = Clamp(sqrtf(rightY * rightY + upY * upY) / 0.25f, 0.0f, 1.0f);
			lookInv = mat4::rotate(-roll * rollWeight, 0.0f, 0.0f, 1.0f);
		}
	}

	// SBS record: clear the backbuffer so a frame whose copy is skipped (size mismatch during a
	// resize) shows black instead of stale swapchain garbage.
	if (sbs && BackBufferView)
	{
		FLOAT sbsBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		Context->ClearRenderTargetView(BackBufferView, sbsBlack);
	}

	for (int eye = 0; eye < 2; eye++)
	{
		// Points arrive in the mono camera's view space; translating by the eye
		// offset lays the camera on that eye, then the asymmetric eye frustum projects.
		// Divide the offset by zoom: the projection below magnifies clip.xy by zoom,
		// which would also magnify the inter-eye disparity (eyes diverge/cross). Pre-
		// dividing keeps the net screen disparity the same as unzoomed.
		vec3 off = VREyes[eye].ViewOffset;
		float invZoom = 1.0f / zoom;
		mat4 eyeView = mat4::translate(-off.x * scale * invZoom, -off.y * scale * invZoom, off.z * scale);

		mat4 eyeProj = VREyes[eye].Projection;

		// Zoom: narrow the frustum symmetrically about the eye's forward axis by scaling
		// only the view.x/view.y terms (m[0], m[5]). Do NOT scale the frustum skew
		// (m[8], m[9]) — the XR eye frusta are asymmetric and scaling the skew amplifies
		// each eye's off-centre projection, diverging the eyes.
		if (zoom != 1.0f)
		{
			eyeProj[0] *= zoom;
			eyeProj[5] *= zoom;
		}

		// Comfort vertical shift: a UNIFORM (depth-independent) pan of the whole eye
		// image so world/HUD/crosshair move together — the crosshair keeps marking the
		// true aim point. clip.y += panY*clip.w  =>  NDC.y += panY for every vertex.
		// Fixed scale (NOT /VRHudDepth) so VRHudDepth only controls HUD depth/convergence.
		// Never altered by the SBS record mode — the wearer's view is sacred; the recording
		// crops what the eyes actually rendered and nothing more.
		float panY = -VRHeightOffset / 150.0f;
		eyeProj[1]  += panY * eyeProj[3];
		eyeProj[5]  += panY * eyeProj[7];
		eyeProj[9]  += panY * eyeProj[11];
		eyeProj[13] += panY * eyeProj[15];

		// Head-look rotates the whole virtual screen (world, sky and HUD together).
		mat4 worldProj = eyeProj * eyeView * lookInv;
		mat4 skyProj = eyeProj * lookInv;

		// SBS record: measure where the screen frame actually lands in this eye — the captured
		// hole vertices through the same worldProj the replay draws them with. Project all 4
		// corners (TL,TR,BL,BR in view space) so any roll/skew shows up, not just a 2-corner box.
		if (sbs && VRHoleX > 0.0f)
		{
			float c[8];
			VRProjectToPixel(worldProj, -VRHoleX,  VRHoleY, VRHoleZ, (float)ew, (float)eh, c[0], c[1]); // TL
			VRProjectToPixel(worldProj,  VRHoleX,  VRHoleY, VRHoleZ, (float)ew, (float)eh, c[2], c[3]); // TR
			VRProjectToPixel(worldProj, -VRHoleX, -VRHoleY, VRHoleZ, (float)ew, (float)eh, c[4], c[5]); // BL
			VRProjectToPixel(worldProj,  VRHoleX, -VRHoleY, VRHoleZ, (float)ew, (float)eh, c[6], c[7]); // BR
			float minx = Min(Min(c[0], c[2]), Min(c[4], c[6]));
			float maxx = Max(Max(c[0], c[2]), Max(c[4], c[6]));
			float miny = Min(Min(c[1], c[3]), Min(c[5], c[7]));
			float maxy = Max(Max(c[1], c[3]), Max(c[5], c[7]));
			sbsRects[eye][0] = minx;
			sbsRects[eye][1] = miny;
			sbsRects[eye][2] = maxx - minx;
			sbsRects[eye][3] = maxy - miny;
		}
		// Weapon: reduced IPD so the first-person weapon doesn't sit in your nose.
		// The 0.1 factor keeps the config in a friendly range (1.0 ~= 10% IPD).
		float weaponIpd = scale * invZoom * VRWeaponIPDScale * 0.1f;
		mat4 weaponView = mat4::translate(-off.x * weaponIpd, -off.y * weaponIpd, off.z * scale);
		mat4 weaponProj = eyeProj * weaponView * lookInv;
		// Overlay tiles stay at Z~1 (near, on top) but converge at a chosen depth: the IPD a
		// real object at that depth would have, so disparity ~= off*scale/depth. The crosshair
		// converges at VRCrosshairDepth, off-centre HUD (bars/ammo) at VRHudDepth.
		float overlayIpd = scale * invZoom / Max(VRCrosshairDepth, 1.0f);
		mat4 overlayView = mat4::translate(-off.x * overlayIpd, -off.y * overlayIpd, off.z * scale);
		mat4 overlayProj = eyeProj * overlayView * lookInv;
		float hudOverlayIpd = scale * invZoom / Max(VRHudDepth, 1.0f);
		mat4 hudOverlayView = mat4::translate(-off.x * hudOverlayIpd, -off.y * hudOverlayIpd, off.z * scale);
		mat4 hudOverlayProj = eyeProj * hudOverlayView * lookInv;
		// HUD meshes (menu player-mesh preview, radar icons): honest worldProj (full IPD at the
		// mesh's true Z -> comfortable convergence) with a per-batch view-space scale so each keeps
		// its own size (a preview's FOV ratio * width fraction; a radar's VRHudScale) — see the
		// HUDMESH case in the loop below. The scale is VIEW space (worldProj * scale — scales the
		// vertex BEFORE projection, z untouched), NOT clip space (scale * worldProj): the eye
		// frustum is asymmetric, and scaling clip.x also scales the skew*z disparity term -> stereo
		// diverges past infinity. ponytail: preview stays centred, not offset into its panel —
		// exact position needs the full sub-viewport render (own frustum + scissor).
		SetupSceneTarget(ew, eh);
		size_t total = QueuedBatches.size();
		// Group adjacent batches by their per-batch projection tag (VRPROJ_*) and replay each
		// group with its own eye projection: sky at zero IPD (infinity), weapon/overlay at
		// reduced/convergence IPD, flash as a clip-space full-screen quad (identity), world
		// normal. No range ordering to get wrong — the tags are set per batch on accumulation.
		mat4 flashProj = mat4::identity();
		for (size_t j = 0; j < total; )
		{
			int pt = QueuedBatches[j].VRProj;
			size_t k = j;
			while (k < total && QueuedBatches[k].VRProj == pt)
				k++;
			if (pt == VRPROJ_HUDMESH)
			{
				// Per-batch scale: adjacent HUD meshes may be different previews/icons.
				for (size_t m = j; m < k; m++)
					DrawSceneWithClears(worldProj * mat4::scale(QueuedBatches[m].VRMeshSX, QueuedBatches[m].VRMeshSY, 1.0f), m, m + 1);
				j = k;
				continue;
			}
			const mat4& proj =
				(pt == VRPROJ_SKY) ? skyProj :
				(pt == VRPROJ_WEAPON) ? weaponProj :
				(pt == VRPROJ_CROSSHAIR) ? overlayProj :
				(pt == VRPROJ_HUDOVERLAY) ? hudOverlayProj :
				(pt == VRPROJ_FLASH) ? flashProj : worldProj;
			DrawSceneWithClears(proj, j, k);
			j = k;
		}

		ID3D11Texture2D* image = VR->BeginEye(eye);
		if (image)
		{
			// Explicit view format: OpenXR runtimes often hand back a TYPELESS
			// swapchain texture, and CreateRenderTargetView(nullptr) can't infer
			// a format from that — which would leave the eye image unrendered (black).
			// UNORM (not _SRGB): the present shader already gamma-encodes (same as the plain
			// UNORM desktop backbuffer). An _SRGB view would encode a SECOND time -> overbright.
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			ComPtr<ID3D11RenderTargetView> rtv;
			HRESULT hr = Device->CreateRenderTargetView(image, &rtvDesc, rtv.TypedInitPtr());
			if (SUCCEEDED(hr))
			{
				RunPresentPass(rtv, ew, eh, VRBrightnessScale, VRBrightnessOffset); // HMD brightness override

				// SBS record: copy the full eye width, cropped vertically to the screen rows
				// (strips the letterbox black), each eye butted at the centre seam. The side
				// eye-frame borders are already black in the eye image; the inner (nose) edge
				// runs to the render limit — maximum kept toward the nose. Full-SBS copies straight
				// into the backbuffer; half-SBS copies into the full-width staging (squeezed after).
				ID3D11Texture2D* sbsDst = sbsHalf ? SbsStaging.get() : BackBuffer.get();
				bool sbsDstOk = sbsHalf
					? (SbsStaging && SbsStagingW == VRSbsCropW * 2 && SbsStagingH == VRSbsCropH)
					: (BackBuffer && BackBufferSizeX == VRSbsCropW * 2 && BackBufferSizeY == VRSbsCropH);
				if (sbs && sbsDst && sbsDstOk && VRSbsCropH > 0 && VRSbsCropW >= (int)ew &&
					VRSbsSrcT + VRSbsCropH <= (int)eh)
				{
					D3D11_BOX box = {};
					box.left = 0; box.top = (UINT)VRSbsSrcT; box.front = 0;
					box.right = VRSbsCropW; box.bottom = (UINT)(VRSbsSrcT + VRSbsCropH); box.back = 1;
					Context->CopySubresourceRegion(sbsDst, 0, eye == 0 ? 0 : ((int)VRSbsCropW), 0, 0, image, 0, &box);
				}
			}
			else
				debugf(TEXT("D3D11Drv VR: CreateRenderTargetView(eye) failed 0x%08x"), (unsigned)hr);
			VR->EndEye(eye);
		}
	}

	// Half-SBS: squeeze the full-width staging (both eyes) 2:1 into the half-width backbuffer with a
	// linear blit (pass-through gamma — the eye pixels are already fully processed). Full-SBS wrote
	// straight into the backbuffer above.
	if (sbsHalf && SbsStagingView && BackBufferView && VRSbsCropH > 0 &&
		BackBufferSizeX == VRSbsCropW && BackBufferSizeY == VRSbsCropH && SbsStagingW == VRSbsCropW * 2)
	{
		BlitSbsHalf(SbsStagingView, BackBufferView, (UINT)VRSbsCropW, (UINT)VRSbsCropH);
	}

	// SBS record: vertical crop to the screen rows (removes the letterbox black top/bottom); the
	// full eye width is kept as-is (the side frame border is already black — no gaps to fill, so
	// the copy can never leave the half black). Common rows across both eyes (union, clamped to
	// the image). Freezes while a menu is up so look-around can't thrash the backbuffer size and
	// restart recorders.
	if (sbs && sbsRects[0][2] > 1.0f && sbsRects[1][2] > 1.0f)
	{
		// Intersection of rows where BOTH eyes have screen, rounded INWARD (ceil top, floor
		// bottom) so a fractional border row can't slip in as a black line.
		int top = (int)ceilf(Clamp(Max(sbsRects[0][1], sbsRects[1][1]), 0.0f, (float)eh));
		int bot = (int)floorf(Clamp(Min(sbsRects[0][1] + sbsRects[0][3], sbsRects[1][1] + sbsRects[1][3]), 0.0f, (float)eh));
		int ch = (bot - top) & ~1;
		if (ch >= 16)
		{
			// Establish the backbuffer size ONCE (from a gameplay frame — menu look-around skews
			// the measure) and freeze it: transient view changes (death cam, zoom) wobble the
			// measured height by a few px, and a swapchain resize aborts recorders mid-clip.
			// Re-establish only if the eye resolution itself changed (VRResScale).
			if ((VRSbsCropH == 0 && !menuUp) || (VRSbsCropH > 0 && VRSbsCropW != (int)ew))
			{
				VRSbsCropH = ch;
				VRSbsCropW = (int)ew;
				VRSBSSizeX = sbsHalf ? VRSbsCropW : VRSbsCropW * 2; // half-SBS backbuffer is one eye wide
				VRSBSSizeY = VRSbsCropH;
			}
			// The vertical position keeps tracking the screen at the frozen height.
			if (VRSbsCropH > 0 && VRSbsCropH <= (int)eh)
				VRSbsSrcT = Clamp(top, 0, (int)eh - VRSbsCropH);
			// Half/full toggled mid-session: re-point the backbuffer width once (a deliberate config
			// change, like VRResScale — a one-time resize is fine here).
			int wantX = sbsHalf ? VRSbsCropW : VRSbsCropW * 2;
			if (VRSbsCropH > 0 && VRSBSSizeX != wantX)
				VRSBSSizeX = wantX;
		}
	}

	// Flat desktop mirror: replay once more with the plain mono projection (the same frustum
	// SetSceneNode builds), so the on-screen view matches the engine's 2D menu/mouse layout —
	// a stereo eye view would be distorted and desync clicks. Skipped per VRMirrorMode (e.g.
	// when the headset is worn) to save a whole replay + present. SBS record mode replaces the
	// mono replay entirely — the eyes were already copied into the backbuffer halves above.
	bool mirror = !sbs && VRWantMirror();
	if (CurrentFrame && mirror)
	{
		float rprojz = VRMainRProjZ; // main scene node FOV, not the leftover sub-view FOV
		float aspect = CurrentFrame->FY / CurrentFrame->FX;
		mat4 monoProj = mat4::frustum(-rprojz, rprojz, -aspect * rprojz, aspect * rprojz, 1.0f, 32768.0f, handedness::left, clipzrange::zero_positive_w);
		mat4 flashProj = mat4::identity();
		SetupSceneTargetMirror(ew, eh); // 1-sample into PPImage[0]: no MSAA, no resolve, no bloom
		// Mirror: all scene batches (no cursor) with the plain mono projection, except the flash
		// quad (already clip-space) and HUD meshes (each its own per-batch view-space scale, like
		// the eye path; monoProj is symmetric so the scale order is cosmetic), same depth clears.
		for (size_t j = 0; j < sceneBatches; )
		{
			int pt = QueuedBatches[j].VRProj;
			size_t k = j;
			while (k < sceneBatches && QueuedBatches[k].VRProj == pt)
				k++;
			if (pt == VRPROJ_HUDMESH)
			{
				for (size_t m = j; m < k; m++)
					DrawSceneWithClears(monoProj * mat4::scale(QueuedBatches[m].VRMeshSX, QueuedBatches[m].VRMeshSY, 1.0f), m, m + 1);
				j = k;
				continue;
			}
			DrawSceneWithClears((pt == VRPROJ_FLASH) ? flashProj : monoProj, j, k);
			j = k;
		}
	}

	// Reset the scissor to the full backbuffer (ReplaySceneToColorBuffer left it eye-sized)
	// so the present fills the whole window and aligns with mouse coordinates. Backbuffer size,
	// not window size: in SBS record mode the backbuffer is larger, and this scissor also covers
	// the next frame's present if the session stops rendering (mono fallback).
	D3D11_RECT fullWindow = {};
	fullWindow.right = (BackBufferSizeX > 0) ? BackBufferSizeX : CurrentSizeX;
	fullWindow.bottom = (BackBufferSizeY > 0) ? BackBufferSizeY : CurrentSizeY;
	Context->RSSetScissorRects(1, &fullWindow);

	// Present the mirror to the desktop (only if we rendered it). Without vsync — the HMD's
	// xrWaitFrame is the frame-pacing authority; blocking on the monitor vsync too would fight
	// it. Mirror is already in PPImage[0] at 1 sample (alreadyResolved) and skips bloom. In SBS
	// record mode the backbuffer already holds the two copied eyes — just present it.
	if (mirror || sbs)
	{
		// Mono mirror present fills the whole backbuffer (== window here; mirror never runs in SBS
		// mode, where the eye copies already own the backbuffer and we just present it).
		if (mirror)
			RunPresentPass(BackBufferView, BackBufferSizeX, BackBufferSizeY, 1.0f, 0.0f, false, true);
		if (SwapChain1)
		{
			DXGI_PRESENT_PARAMETERS presentParams = {};
			SwapChain1->Present1(0, 0, &presentParams);
		}
		else
		{
			SwapChain->Present(0, 0);
		}
	}

	// Reset for the next frame (mono Unlock does the equivalent inline).
	QueuedBatches.clear();
	Batch = DrawBatchEntry();
	SceneVertexPos = 0;
	SceneIndexPos = 0;
	VRClearZAt.clear();
	VRHudStart = -1;
	VRCurProj = VRPROJ_WORLD;
	VRCurMeshSX = 1.0f;
	VRCurMeshSY = 1.0f;
	VRHudMeshArm = false;

	unguard;
}

void UD3D11RenderDevice::CreateVertexShader(ComPtr<ID3D11VertexShader>& outShader, const std::string& shaderName, ComPtr<ID3D11InputLayout>& outInputLayout, const std::string& inputLayoutName, const std::vector<D3D11_INPUT_ELEMENT_DESC>& elements, const std::string& filename, const std::vector<std::string> defines)
{
	std::vector<uint8_t> bytecode = CompileHlsl(filename, "vs", defines);
	HRESULT result = Device->CreateVertexShader(bytecode.data(), bytecode.size(), nullptr, outShader.TypedInitPtr());
	ThrowIfFailed(result, ("CreateVertexShader(" + shaderName + ") failed").c_str());
	SetDebugName(outShader, shaderName.c_str());

	result = Device->CreateInputLayout(elements.data(), (UINT)elements.size(), bytecode.data(), bytecode.size(), outInputLayout.TypedInitPtr());
	ThrowIfFailed(result, ("CreateInputLayout(" + inputLayoutName + ") failed").c_str());
	SetDebugName(outInputLayout, inputLayoutName.c_str());
}

void UD3D11RenderDevice::CreatePixelShader(ComPtr<ID3D11PixelShader>& outShader, const std::string& shaderName, const std::string& filename, const std::vector<std::string> defines)
{
	std::vector<uint8_t> bytecode = CompileHlsl(filename, "ps", defines);
	HRESULT result = Device->CreatePixelShader(bytecode.data(), bytecode.size(), nullptr, outShader.TypedInitPtr());
	ThrowIfFailed(result, ("CreatePixelShader(" + shaderName + ") failed").c_str());
	SetDebugName(outShader, shaderName.c_str());
}

void UD3D11RenderDevice::SetDebugName(ID3D11Device* obj, const char* name)
{
	if (UseDebugLayer)
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}

void UD3D11RenderDevice::SetDebugName(ID3D11DeviceChild* obj, const char* name)
{
	if (UseDebugLayer)
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}

std::vector<uint8_t> UD3D11RenderDevice::CompileHlsl(const std::string& filename, const std::string& shadertype, const std::vector<std::string> defines)
{
	std::string code = FileResource::readAllText(filename);

	std::string target;
	switch (FeatureLevel)
	{
	default:
	case D3D_FEATURE_LEVEL_11_1: target = shadertype + "_5_0"; break;
	case D3D_FEATURE_LEVEL_11_0: target = shadertype + "_5_0"; break;
	case D3D_FEATURE_LEVEL_10_1: target = shadertype + "_4_1"; break;
	case D3D_FEATURE_LEVEL_10_0: target = shadertype + "_4_0"; break;
	}

	std::vector<D3D_SHADER_MACRO> macros;
	for (const std::string& define : defines)
	{
		D3D_SHADER_MACRO macro = {};
		macro.Name = define.c_str();
		macro.Definition = "1";
		macros.push_back(macro);
	}
	macros.push_back({});

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errors;
	HRESULT result = D3DCompile(code.data(), code.size(), filename.c_str(), macros.data(), nullptr, "main", target.c_str(), D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, blob.TypedInitPtr(), errors.TypedInitPtr());
	if (FAILED(result))
	{
		std::string msg((const char*)errors->GetBufferPointer(), errors->GetBufferSize());
		if (!msg.empty() && msg.back() == 0) msg.pop_back();
		throw std::runtime_error("Could not compile shader '" + filename + "':" + msg);
	}
	ThrowIfFailed(result, "D3DCompile failed");

	std::vector<uint8_t> bytecode;
	bytecode.resize(blob->GetBufferSize());
	memcpy(bytecode.data(), blob->GetBufferPointer(), bytecode.size());
	return bytecode;
}
