#pragma once

#include "vec.h"
#include "mat.h"
#include "ComPtr.h"
#include "CycleTimer.h"
#include <set>
#include <string>

#include "TextureManager.h"
#include "UploadManager.h"
#include "CachedTexture.h"
#include "VRBackend.h"

struct SceneVertex
{
	uint32_t Flags;
	vec3 Position;
	vec2 TexCoord;
	vec2 TexCoord2;
	vec2 TexCoord3;
	vec2 TexCoord4;
	vec4 Color;
};

struct ScenePushConstants
{
	mat4 ObjectToProjection;
	vec4 NearClip;
	int HitIndex;
	int Padding1, Padding2, Padding3;
};

struct PresentPushConstants
{
	float Contrast;
	float Saturation;
	float Brightness;
	float HdrScale;
	vec4 GammaCorrection;
};

struct BloomPushConstants
{
	float SampleWeights[8];
};

#if defined(OLDUNREAL469SDK)
class UD3D11RenderDevice : public URenderDeviceOldUnreal469
{
public:
	DECLARE_CLASS(UD3D11RenderDevice, URenderDeviceOldUnreal469, CLASS_Config, D3D11Drv)
#else
class UD3D11RenderDevice : public URenderDevice
{
public:
	DECLARE_CLASS(UD3D11RenderDevice, URenderDevice, CLASS_Config)
#endif

	UD3D11RenderDevice();
	void StaticConstructor();

	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) override;
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) override;
	void Exit() override;
#if defined(UNREALGOLD)
	void Flush() override;
#else
	void Flush(UBOOL AllowPrecache) override;
#endif
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize) override;
	void Unlock(UBOOL Blit) override;
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet) override;
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span) override;
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) override;
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector OrigP, FVector OrigQ);
	void Draw2DClippedLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) override;
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z) override;
	void ClearZ(FSceneNode* Frame) override;
	void PushHit(const BYTE* Data, INT Count) override;
	void PopHit(INT Count, UBOOL bForce) override;
	void GetStats(TCHAR* Result) override;
	void ReadPixels(FColor* Pixels) override;
	void EndFlash() override;
	void SetSceneNode(FSceneNode* Frame) override;
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags) override;
	void DrawStats(FSceneNode* Frame) override;

	void SetHitLocation();

#if defined(OLDUNREAL469SDK)
	// URenderDeviceOldUnreal469 extensions
	void DrawGouraudTriangles(const FSceneNode* Frame, const FTextureInfo& Info, FTransTexture* const Pts, INT NumPts, DWORD PolyFlags, DWORD DataFlags, FSpanBuffer* Span) override;
	UBOOL SupportsTextureFormat(ETextureFormat Format) override;
	void UpdateTextureRect(FTextureInfo& Info, INT U, INT V, INT UL, INT VL) override;
	void DrawTileList(const FSceneNode* Frame, const FTextureInfo& Info, const FTileRect* Tiles, INT NumTiles, FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) override;
#endif

	void SetDebugName(ID3D11Device* obj, const char* name);
	void SetDebugName(ID3D11DeviceChild* obj, const char* name);

	int InterfacePadding[64]; // For allowing URenderDeviceOldUnreal469 interface to add things

	HWND WindowHandle = 0;
	ComPtr<ID3D11Device> Device;
	D3D_FEATURE_LEVEL FeatureLevel = {};
	ComPtr<ID3D11DeviceContext> Context;
	ComPtr<IDXGISwapChain> SwapChain;
	ComPtr<IDXGISwapChain1> SwapChain1;
	ComPtr<ID3D11Debug> DebugLayer;
	ComPtr<ID3D11InfoQueue> InfoQueue;
	ComPtr<ID3D11Texture2D> BackBuffer;
	ComPtr<ID3D11RenderTargetView> BackBufferView;
	bool DxgiSwapChainAllowTearing = false;
	int BufferCount = 2;
	std::set<std::string> SeenDebugMessages;
	int TotalSeenDebugMessages = 0;

	struct PPBlurLevel
	{
		ComPtr<ID3D11Texture2D> VTexture;
		ComPtr<ID3D11RenderTargetView> VTextureRTV;
		ComPtr<ID3D11ShaderResourceView> VTextureSRV;
		ComPtr<ID3D11Texture2D> HTexture;
		ComPtr<ID3D11RenderTargetView> HTextureRTV;
		ComPtr<ID3D11ShaderResourceView> HTextureSRV;
		int Width = 0;
		int Height = 0;
	};

	struct
	{
		ComPtr<ID3D11Texture2D> ColorBuffer;
		ComPtr<ID3D11Texture2D> HitBuffer;
		ComPtr<ID3D11Texture2D> DepthBuffer;
		ComPtr<ID3D11Texture2D> MirrorDepthBuffer; // 1-sample depth for the no-AA VR mirror pass
		ComPtr<ID3D11Texture2D> PPImage[2];
		ComPtr<ID3D11Texture2D> PPHitBuffer;
		ComPtr<ID3D11Texture2D> StagingHitBuffer;
		ComPtr<ID3D11RenderTargetView> ColorBufferView;
		ComPtr<ID3D11RenderTargetView> HitBufferView;
		ComPtr<ID3D11DepthStencilView> DepthBufferView;
		ComPtr<ID3D11DepthStencilView> MirrorDepthBufferView;
		ComPtr<ID3D11RenderTargetView> PPHitBufferView;
		ComPtr<ID3D11RenderTargetView> PPImageView[2];
		ComPtr<ID3D11ShaderResourceView> HitBufferShaderView;
		ComPtr<ID3D11ShaderResourceView> PPImageShaderView[2];
		enum { NumBloomLevels = 4 };
		PPBlurLevel BlurLevels[NumBloomLevels];
		int Width = 0;
		int Height = 0;
		int Multisample = 0;
	} SceneBuffers;

	struct ScenePipelineState
	{
		D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ID3D11PixelShader* PixelShader = nullptr;
		ComPtr<ID3D11BlendState> BlendState;
		ComPtr<ID3D11DepthStencilState> DepthStencilState;
		float MinDepth = 0.1f;
		float MaxDepth = 1.0f;
	};

	struct
	{
		ComPtr<ID3D11VertexShader> VertexShader;
		ComPtr<ID3D11InputLayout> InputLayout;
		ComPtr<ID3D11Buffer> VertexBuffer;
		ComPtr<ID3D11Buffer> IndexBuffer;
		ComPtr<ID3D11Buffer> ConstantBuffer;
		ComPtr<ID3D11RasterizerState> RasterizerState[2];
		ComPtr<ID3D11PixelShader> PixelShader;
		ComPtr<ID3D11PixelShader> PixelShaderAlphaTest;
		ComPtr<ID3D11SamplerState> Samplers[16];
		ScenePipelineState Pipelines[32];
		ScenePipelineState LinePipeline[2];
		ScenePipelineState PointPipeline[2];
		FLOAT LODBias = 0.0f;
	} ScenePass;

	static const int SceneVertexBufferSize = 16 * 1024;
	static const int SceneIndexBufferSize = 32 * 1024;

	struct DrawBatchEntry
	{
		size_t SceneIndexStart = 0;
		size_t SceneIndexEnd = 0;
		ScenePipelineState* Pipeline = nullptr;
		CachedTexture* Tex = nullptr;
		CachedTexture* Lightmap = nullptr;
		CachedTexture* Detailtex = nullptr;
		CachedTexture* Macrotex = nullptr;
		uint32_t TexSamplerMode = 0;
		uint32_t DetailtexSamplerMode = 0;
		uint32_t MacrotexSamplerMode = 0;
		int VRProj = 0; // which eye projection this batch replays with (VRPROJ_*)
	} Batch;
	std::vector<DrawBatchEntry> QueuedBatches;
	CachedTexture* nulltex = nullptr;

	SceneVertex* SceneVertices = nullptr;
	size_t SceneVertexPos = 0;

	uint32_t* SceneIndexes = nullptr;
	size_t SceneIndexPos = 0;

	// Runtime buffer capacity. Defaults to the (FPS-tuned) mono sizes; only VR
	// bumps them, so the mono path keeps its exact flush cadence.
	int SceneVertexBufferCap = SceneVertexBufferSize;
	int SceneIndexBufferCap = SceneIndexBufferSize;

	struct
	{
		ComPtr<ID3D11VertexShader> PPStep;
		ComPtr<ID3D11InputLayout> PPStepLayout;
		ComPtr<ID3D11Buffer> PPStepVertexBuffer;
		ComPtr<ID3D11PixelShader> HitResolve;
		ComPtr<ID3D11PixelShader> Present[16];
		ComPtr<ID3D11Buffer> PresentConstantBuffer;
		ComPtr<ID3D11Texture2D> DitherTexture;
		ComPtr<ID3D11ShaderResourceView> DitherTextureView;
		ComPtr<ID3D11BlendState> BlendState;
		ComPtr<ID3D11DepthStencilState> DepthStencilState;
		ComPtr<ID3D11RasterizerState> RasterizerState;
	} PresentPass;

	struct
	{
		ComPtr<ID3D11PixelShader> Extract;
		ComPtr<ID3D11PixelShader> Combine;
		ComPtr<ID3D11PixelShader> BlurVertical;
		ComPtr<ID3D11PixelShader> BlurHorizontal;
		ComPtr<ID3D11Buffer> ConstantBuffer;
		ComPtr<ID3D11BlendState> AdditiveBlendState;
	} BloomPass;

	std::unique_ptr<TextureManager> Textures;
	std::unique_ptr<UploadManager> Uploads;

	// Configuration.
	BITFIELD UseVSync;
	FLOAT GammaOffset;
	FLOAT GammaOffsetRed;
	FLOAT GammaOffsetGreen;
	FLOAT GammaOffsetBlue;
	BYTE LinearBrightness;
	BYTE Contrast;
	BYTE Saturation;
	INT GrayFormula;
	BITFIELD Hdr;
	BYTE HdrScale;
#if !defined(OLDUNREAL469SDK)
	BITFIELD OccludeLines;
#endif
	BITFIELD Bloom;
	BYTE BloomAmount;
	FLOAT LODBias;
	BYTE AntialiasMode;
	BYTE GammaMode;
	BYTE LightMode;
	INT RefreshRate;
	BITFIELD GammaCorrectScreenshots;
	BITFIELD UseDebugLayer;

	// VR (OpenXR). Everything VR is gated on UseVR; mono path is untouched.
	BITFIELD UseVR;
	FLOAT VRWorldScale;   // UU per metre (IPD / eye-offset calibration)
	FLOAT VRHudDepth;     // view-space Z (UU) gameplay HUD is pushed to (convergence)
	FLOAT VRHudDepthBottom; // nearer depth for the bottom-of-screen HUD zone (weapon bar under the nose); <=0 disables (uses VRHudDepth)
	FLOAT VRHudBottomY;   // bottom zone = screen Y fraction below this (0..1); default 0.7 = bottom 30%
	FLOAT VRUIDepth;      // ... depth for HUD/UI when the mouse is free (menu/console)
	FLOAT VRCrosshairDepth; // ... depth for centre tiles (the crosshair) in gameplay (mouse captured)
	FLOAT VRResScale;     // eye render supersampling vs the runtime's recommended resolution
	FLOAT VRHeightOffset; // view-space vertical shift (UU) of the eyes for comfort (M0 has no head pitch)
	BITFIELD VRClearZBeforeHud; // clear depth before HUD so it/the crosshair aren't occluded by world geometry
	BYTE VRLookMode;      // head-look mode. 0 = virtual screen (gameplay: roll only; menu: look around the screen)
	FLOAT VRHudScaleX;    // scale HUD toward the centre horizontally so corner elements stay inside the FOV (gameplay only)
	FLOAT VRHudScaleY;    // ... and vertically
	FLOAT VRWeaponIPDScale; // stereo separation for the first-person weapon (HACKFLAGS_NoNearZ); <1 pulls it out of your nose
	FLOAT VRBrightnessScale;  // HMD eye brightness = game brightness * scale + offset (eye only; mirror keeps the game's)
	FLOAT VRBrightnessOffset;
	BYTE VRMirrorMode;        // desktop mirror: 0=off, 1=when headset removed (default), 2=in menu, 3=always
	UBOOL VRScaleHudMeshes;   // scale PostRender Gouraud meshes (e.g. mod radar icons) with the HUD, at HUD depth

	struct
	{
		int ComplexSurfaces = 0;
		int GouraudPolygons = 0;
		int Tiles = 0;
		int DrawCalls = 0;
		int Uploads = 0;
		int RectUploads = 0;
		int BuffersUsed = 0;
	} Stats;

	struct
	{
		CycleTimer DrawBatches;
		CycleTimer DrawComplexSurface;
		CycleTimer DrawGouraudPolygon;
		CycleTimer DrawTile;
		CycleTimer DrawGouraudTriangles;
		CycleTimer TextureUpload;
		CycleTimer TextureCache;
	} Timers;

	CycleTimer* ActiveTimer = nullptr;

private:
	struct ComplexSurfaceInfo
	{
		FSurfaceFacet* facet;
		CachedTexture* tex;
		CachedTexture* lightmap;
		CachedTexture* macrotex;
		CachedTexture* detailtex;
		CachedTexture* fogmap;
		vec4* editorcolor;
	};
	void DrawComplexSurfaceFaces(const ComplexSurfaceInfo& info);

	void ReleaseSwapChainResources();
	bool UpdateSwapChain();
	void SetColorSpace();
	void ResizeSceneBuffers(int width, int height, int multisample);
	void ClearTextureCache();

	void CreatePresentPass();
	void CreateBloomPass();
	void CreateScenePass();
	void ReleaseScenePass();
	void ReleaseBloomPass();
	void ReleasePresentPass();

	void CreateSceneSamplers();
	void ReleaseSceneSamplers();
	void UpdateLODBias();

	void ReleaseSceneBuffers();

	void RunBloomPass();
	void BlurStep(ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, bool vertical);
	float ComputeBlurGaussian(float n, float theta);
	void ComputeBlurSamples(int sampleCount, float blurAmount, float* sampleWeights);

	void SetPipeline(ScenePipelineState* pipeline);
	void SetPipeline(DWORD polyflags);
	void SetDescriptorSet(DWORD polyflags, CachedTexture* tex = nullptr, bool clamp = false);
	void SetDescriptorSet(DWORD polyflags, const ComplexSurfaceInfo& info);

	void AddDrawBatch();
	void DrawBatches(bool nextBuffer = false);
	void DrawEntry(const DrawBatchEntry& entry, bool forceNoDepth = false);

	// VR runtime state + helpers (only touched when UseVR).
	VRBackend* VR = nullptr;
	bool VRRetaining = false;    // true during a VR frame: DrawBatches accumulates instead of drawing
	bool VRShouldRender = false; // this VR frame wants eye rendering (else compositor said skip)
	VREyePose VREyes[2];
	ScenePipelineState VRCursorPipeline; // cursor pipeline forced to a near depth range so it draws on top
	// Cached UWindow-cursor property offsets (Root.MouseWindow.Cursor.tex/HotX/HotY). Classes
	// never unload mid-game, so these are resolved once per console class — no per-frame FindField.
	struct VRCursorRefl
	{
		UClass* ConsoleClass = nullptr;
		UObjectProperty* RootProp = nullptr;
		UObjectProperty* MouseWindowProp = nullptr;
		UStructProperty* CursorProp = nullptr; // Cursor is a struct, not an object
		UObjectProperty* TexProp = nullptr;
		UIntProperty* HotXProp = nullptr;
		UIntProperty* HotYProp = nullptr;
	} VRCursor;
	std::vector<size_t> VRClearZAt; // batch indices where the depth buffer is cleared during replay (skybox ClearZ, HUD)
	int VRHudStart = -1;            // first HUD batch (PostRender begins); HUD drawn with depth test off (painter's order, no z-fight)
	bool VRSkyZones[64] = {};       // per-zone: geometry in this zone is a skybox (drawn with zero IPD -> infinity)
	ULevel* VRSkyZonesLevel = nullptr; // level the flags were built for (rebuild on change)
	// Per-batch eye projection tags (DrawBatchEntry::VRProj), set during accumulation.
	enum { VRPROJ_WORLD = 0, VRPROJ_WEAPON, VRPROJ_CROSSHAIR, VRPROJ_HUDOVERLAY, VRPROJ_SKY, VRPROJ_FLASH, VRPROJ_HUDMESH };
	int VRCurProj = 0;             // projection tag for the batch currently accumulating
	bool VRHudMeshArm = false;     // a HUD-phase ClearZ armed the next Gouraud as a HUD mesh (VRScaleHudMeshes)
	float VRMainRProjZ = 1.0f;     // main (first) scene node's tan(FovAngle/2) — replay + HUD baking use THIS, not a poisoned leftover sub-view FOV (menu player-mesh preview)
	bool VRMainCaptured = false;   // main scene node captured this frame (reset each Lock)
	ID3D11DepthStencilView* VRReplayDepth = nullptr; // depth view the current replay clears (eyes: MSAA; mirror: 1-sample)
	int VRDropped = 0;             // primitives dropped this window because the VR buffer was full
	DOUBLE VRLastDropLog = 0.0;    // appSecondsNew() of the last overflow report (throttle)
	bool VRWorn = true;            // cached headset-worn state (refreshed ~once/sec, not per frame)
	DOUBLE VRWornCheck = 0.0;      // appSecondsNew() of the last worn poll (throttle)
	ComPtr<ID3D11DepthStencilState> VRNoDepthState; // depth test disabled, for the HUD range
	vec4 VRHeadRef = vec4(0.0f, 0.0f, 0.0f, 1.0f); // recenter reference head orientation (quaternion)
	bool VRRecenterPending = false; // console/menu-exit asked to recenter next frame
	bool VRWasMenu = false;         // mouse-free (UI) state last frame, to recenter on menu->gameplay
	void StartVR();
	void RenderVREyes();
	bool VRWantMirror(); // should the desktop mirror render this frame? (VRMirrorMode + worn + menu; worn poll throttled)
	void DrawVRCursor(); // draw the real UWindow cursor into the eyes (HMD has no OS cursor)
	void VRSetProj(int proj); // tag the accumulating batch's eye projection (VRPROJ_*), boundary on change
	bool VRIsSkyFrame(const FSceneNode* Frame); // is this frame rendering a skybox zone? (cached per level)
	void VRBeginHudRange();   // start the HUD range (VRHudStart, depth clear, screen frame)
	void VRBeginPostRender(); // trigger VRBeginHudRange on the first PostRender draw
	void InjectVRScreenFrame(); // black border masking geometry outside the game FOV (the eye FOV is wider)
	void SetupSceneTarget(uint32_t width, uint32_t height);
	void DrawSceneWithClears(const mat4& objectToProjection, size_t begin, size_t end);
	void RunPresentPass(ID3D11RenderTargetView* output, UINT width, UINT height, float brightnessScale = 1.0f, float brightnessOffset = 0.0f, bool bloom = true, bool alreadyResolved = false);
	void SetupSceneTargetMirror(uint32_t width, uint32_t height); // 1-sample mirror target (PPImage[0] + 1-sample depth), no MSAA

	void PrintDebugLayerMessages();

	struct VertexReserveInfo
	{
		SceneVertex* vptr;
		uint32_t* iptr;
		uint32_t vpos;
	};

	VertexReserveInfo ReserveVertices(size_t vcount, size_t icount)
	{
		if (!SceneVertices || !SceneIndexes)
			return { nullptr, nullptr, 0 };

		// If buffers are full, flush and wait for room.
		if (SceneVertexPos + vcount > (size_t)SceneVertexBufferCap || SceneIndexPos + icount > (size_t)SceneIndexBufferCap)
		{
			// If the request is larger than our buffers we can't draw this.
			if (vcount > (size_t)SceneVertexBufferCap || icount > (size_t)SceneIndexBufferCap)
			{
				if (VRRetaining)
					VRDropped++;
				return { nullptr, nullptr, 0 };
			}

			// VR retains the whole frame and can't flush mid-frame: DrawBatches would
			// not free space, and falling through would hand back a pointer past the
			// buffer end (overrun -> crash). Drop this primitive instead.
			if (VRRetaining)
			{
				VRDropped++;
				return { nullptr, nullptr, 0 };
			}

			DrawBatches(true);
		}

		return { SceneVertices + SceneVertexPos, SceneIndexes + SceneIndexPos, (uint32_t)SceneVertexPos };
	}

	void UseVertices(size_t vcount, size_t icount)
	{
		SceneVertexPos += vcount;
		SceneIndexPos += icount;
	}

	int GetSettingsMultisample();

	ScenePipelineState* GetPipeline(DWORD PolyFlags);

	void CreateVertexShader(ComPtr<ID3D11VertexShader>& outShader, const std::string& shaderName, ComPtr<ID3D11InputLayout>& outInputLayout, const std::string& inputLayoutName, const std::vector<D3D11_INPUT_ELEMENT_DESC>& elements, const std::string& filename, const std::vector<std::string> defines = {});
	void CreatePixelShader(ComPtr<ID3D11PixelShader>& outShader, const std::string& shaderName, const std::string& filename, const std::vector<std::string> defines = {});
	std::vector<uint8_t> CompileHlsl(const std::string& filename, const std::string& shadertype, const std::vector<std::string> defines = {});

	vec4 ApplyInverseGamma(vec4 color);

	PresentPushConstants GetPresentPushConstants(float brightnessScale = 1.0f, float brightnessOffset = 0.0f);

	bool VerticesMapped() const { return SceneVertices && SceneIndexes; }
	void MapVertices(bool nextBuffer);
	void UnmapVertices();

	UBOOL UsePrecache;
	FPlane FlashScale;
	FPlane FlashFog;
	FSceneNode* CurrentFrame = nullptr;
	float Aspect;
	float RProjZ;
	float RFX2;
	float RFY2;
	ScenePushConstants SceneConstants = {};
	D3D11_VIEWPORT SceneViewport = {};
	bool DepthCuedActive = false;

	struct HitQuery
	{
		INT Start = 0;
		INT Count = 0;
	};

	BYTE* HitData = nullptr;
	INT* HitSize = nullptr;
	std::vector<BYTE> HitQueryStack;
	std::vector<HitQuery> HitQueries;
	std::vector<BYTE> HitBuffer;

	int ForceHitIndex = -1;
	HitQuery ForceHit;

	bool IsLocked = false;
	bool ActiveHdr = false;

	struct
	{
		int Width = 0;
		int Height = 0;
	} DesktopResolution;

	bool InSetResCall = false;
	UBOOL CurrentFullscreen = 0;
	int CurrentSizeX = 0;
	int CurrentSizeY = 0;

	struct DestructorLog
	{
		~DestructorLog() { debugf(TEXT("D3D11Drv: destroying instance")); }
	} logDestruction;
};

inline void ThrowIfFailed(HRESULT result, const char* msg) { if (FAILED(result)) throw std::runtime_error(msg); }

inline float GetUMult(const FTextureInfo& Info) { return 1.0f / (Info.UScale * Info.USize); }
inline float GetVMult(const FTextureInfo& Info) { return 1.0f / (Info.VScale * Info.VSize); }

inline void UD3D11RenderDevice::SetPipeline(ScenePipelineState* pipeline)
{
	if (pipeline != Batch.Pipeline)
	{
		AddDrawBatch();
		Batch.Pipeline = pipeline;
	}
}

inline void UD3D11RenderDevice::SetPipeline(DWORD PolyFlags)
{
	auto pipeline = GetPipeline(PolyFlags);
	if (pipeline != Batch.Pipeline)
	{
		AddDrawBatch();
		Batch.Pipeline = pipeline;
	}
}

inline void UD3D11RenderDevice::SetDescriptorSet(DWORD PolyFlags, CachedTexture* tex, bool clamp)
{
	if (!tex) tex = nulltex;

	uint32_t samplermode = 0;
	if (PolyFlags & PF_NoSmooth) samplermode |= 1;
	if (clamp) samplermode |= 2;
	samplermode |= (tex->DummyMipmapCount << 2);

	if (Batch.Tex != tex || Batch.TexSamplerMode != samplermode || Batch.Lightmap != nulltex || Batch.Detailtex != nulltex || Batch.DetailtexSamplerMode != 0 || Batch.Macrotex != nulltex || Batch.MacrotexSamplerMode != 0)
	{
		AddDrawBatch();
		Batch.Tex = tex;
		Batch.Lightmap = nulltex;
		Batch.Detailtex = nulltex;
		Batch.Macrotex = nulltex;
		Batch.TexSamplerMode = samplermode;
		Batch.DetailtexSamplerMode = 0;
		Batch.MacrotexSamplerMode = 0;
	}
}

inline void UD3D11RenderDevice::SetDescriptorSet(DWORD PolyFlags, const ComplexSurfaceInfo& info)
{
	uint32_t samplermode = 0;
	if (PolyFlags & PF_NoSmooth) samplermode |= 1;
	samplermode |= (info.tex->DummyMipmapCount << 2);

	int detailsamplermode = info.detailtex->DummyMipmapCount << 2;
	int macrosamplermode = info.macrotex->DummyMipmapCount << 2;

	if (Batch.Tex != info.tex || Batch.TexSamplerMode != samplermode || Batch.Lightmap != info.lightmap || Batch.Detailtex != info.detailtex || Batch.DetailtexSamplerMode != detailsamplermode || Batch.Macrotex != info.macrotex || Batch.MacrotexSamplerMode != macrosamplermode)
	{
		AddDrawBatch();
		Batch.Tex = info.tex;
		Batch.Lightmap = info.lightmap;
		Batch.Detailtex = info.detailtex;
		Batch.Macrotex = info.macrotex;
		Batch.TexSamplerMode = samplermode;
		Batch.DetailtexSamplerMode = detailsamplermode;
		Batch.MacrotexSamplerMode = macrosamplermode;
	}
}

inline DWORD ApplyPrecedenceRules(DWORD PolyFlags)
{
	// Adjust PolyFlags according to Unreal's precedence rules.
	if (!(PolyFlags & (PF_Translucent | PF_Modulated)))
		PolyFlags |= PF_Occlude;
	else if (PolyFlags & PF_Translucent)
		PolyFlags &= ~PF_Masked;
	return PolyFlags;
}
