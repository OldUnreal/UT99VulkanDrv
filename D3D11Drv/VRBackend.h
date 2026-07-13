#pragma once

// VR backend seam. Deliberately free of any OpenXR/OpenVR headers so the hot
// renderer translation unit that holds a VRBackend* stays clean. Only the
// backend .cpp pulls in the runtime SDK.
//
// Everything here is driver-internal and only ever touched when UseVR is on.

#include "mat.h"
#include <cstdint>

struct ID3D11Device;
struct ID3D11Texture2D;

struct VREyePose
{
	mat4 Projection;   // clip-from-eye, asymmetric frustum from the runtime fov (near=1, far=32768, same convention as SetSceneNode)
	vec3 ViewOffset;   // eye position relative to head, in METERS, view-space axes (x=right). Renderer scales by VRWorldScale.
};

class VRBackend
{
public:
	virtual ~VRBackend() {}

	// Phase 1: before the D3D11 device exists. Returns the adapter the runtime
	// wants the device created on (packed LUID). false = no usable runtime.
	virtual bool QueryAdapterLuid(uint64_t& outLuid) = 0;

	// Phase 2: hand over the created device. Builds session + swapchains.
	// resScale multiplies the runtime's recommended eye resolution (supersampling).
	virtual bool Start(ID3D11Device* device, float resScale) = 0;

	// Per-eye render target size the runtime recommends (both eyes equal here).
	virtual void GetEyeResolution(uint32_t& width, uint32_t& height) = 0;

	// Head orientation this frame as a quaternion (x,y,z,w) in the runtime's LOCAL
	// reference space. Valid after BeginFrame returned true.
	virtual void GetHeadOrientation(float outQuat[4]) = 0;

	// Frame start: pumps events, waits/begins the XR frame, locates the eyes.
	// Returns true if the app should render this frame; false = skip rendering
	// but the caller must still call EndFrame(false). If the session is not in
	// a rendering state this returns false and Running() is false — caller
	// should fall back to the normal mono present.
	virtual bool BeginFrame(VREyePose outEyes[2]) = 0;

	// Acquire the swapchain color image for an eye (sRGB RGBA, eye resolution).
	// Caller renders its present/tonemap pass straight into it, then EndEye.
	virtual ID3D11Texture2D* BeginEye(int eye) = 0;
	virtual void EndEye(int eye) = 0;

	// Submits the composited frame. Pass rendered=false to end a skipped frame.
	virtual void EndFrame(bool rendered) = 0;

	// True while the session is in a state that expects rendered frames.
	virtual bool Running() const = 0;

	// True while the headset is actively worn/focused. Combines every reliable signal the
	// runtime offers (session focus + XR_EXT_user_presence when available). Cheap to call
	// (reads cached event state); the caller still throttles how often it polls.
	virtual bool IsWorn() const = 0;

	virtual void Stop() = 0;
};

// Returns nullptr if the OpenXR loader/runtime is not present.
VRBackend* CreateOpenXRBackend();
