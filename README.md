# Unreal Tournament Render Devices
This project implements Vulkan, Direct3D 12, and Direct3D 11 render devices for Unreal Tournament (UT99).

## Compiling the source

The project files were made for Visual Studio 2022. Open VulkanDrv.sln, select the release configuration and press build.

Note: This project requires the 469 SDK. It also requires 469c or newer to run.

## Using VulkanDrv, D3D11Drv or D3D12Drv as the render device

Copy the .dll and .int files files to the Unreal Tournament system folder.

In the [Engine.Engine] section of UnrealTournament.ini, change GameRenderDevice to VulkanDrv.VulkanRenderDevice, D3D12Drv.D3D12RenderDevice or D3D11Drv.D3D11RenderDevice. Then launch the game.

All the render devices supports the following renderdev specific settings in each their section of the UnrealTournament.ini file:

	LODBias=-0.500000
	GammaOffsetBlue=0.000000
	GammaOffsetGreen=0.000000
	GammaOffsetRed=0.000000
	GammaOffset=0.000000
	GammaMode=D3D9
	UseVSync=True
	LightMode=Normal
	OccludeLines=True
	Hdr=False
	AntialiasMode=MSAA_4x
	Saturation=255
	Contrast=128
	LinearBrightness=128

VulkanDrv specific settings:

	VkDebug=False
	VkDeviceIndex=0
	VkExclusiveFullscreen=False

D3D12Drv specific settings:

	UseDebugLayer=False

D3D11Drv specific settings (OpenXR virtual reality):

	UseVR=False
	VRResScale=1.500000
	VRWorldScale=52.500000
	VRHeightOffset=0.000000
	VRLookMode=0
	VRHudDepth=150.000000
	VRHudDepthBottom=30.000000
	VRHudBottomY=0.700000
	VRUIDepth=150.000000
	VRCrosshairDepth=150.000000
	VRWeaponIPDScale=1.000000
	VRHudScaleX=1.000000
	VRHudScaleY=1.000000
	VRScaleHudMeshes=False
	VRClearZBeforeHud=True
	VRBrightnessScale=1.000000
	VRBrightnessOffset=0.000000
	VRMirrorMode=1

## Description of settings

- LightMode:
  - Normal: The default rendering of the Direct3D render devices that shipped with the game
  - OneXBlending: Halve the brightness of the light maps (all map surfaces are half as bright). This effectively makes the actors brighter relative to actors
  - BrighterActors: This doubles the brightness of the actors instead of making the light maps dimmer
- GammaOffset, GammaOffsetRed, GammaOffsetGreen, GammaOfsetBlue: Add additional gamma to all pixels. GammaOffset for all color channels, the others for specific ones
- GammaMode:
  - D3D9: Use the gamma calculations from the other Direct3D render devices
  - XOpenGL: Use the gamma calculations from the XOpenGL render device
- Hdr: Overbright pixels will use the high dynamic range of the monitor. Note: this will only work if HDR is enabled in the Windows display settings and if you have a HDR capable monitor
- AntialiasMode:
  - Off: No anti alias applied
  - MSAA_2x: 2x multisampling
  - MSAA_4x: 4x multisampling
- Saturation: Saturation color control. 255 is the default. 128 is black and white. Zero inversely saturates the colors.
- Contrast: Contrast color control. 128 is the default.
- LinearBrightness: True brightness control (UT's brightness control is actually gamma control). 128 is the default.
- OccludeLines: If true, lines are occluded by geometry in the Unreal editor.
- LODBias: Adjusts the level-of-detail bias for textures. A number greater than zero will bias it towards using lower detail mipmaps. A negative number will bias it towards using higher level mipmaps
- Bloom: Adds bloom to overbright pixels
- BloomAmount: Controls how strong the bloom blur should be. 128 is the default.

## Description of VulkanDrv specific settings

- VkDebug enables the vulkan debug layer and will make the render device output extra information into the UnrealTournament.log file. 'VkMemStats' can also be typed into the console.
- VkExclusiveFullscreen enables vulkan's exclusive full screen feature. It is off by default as some users have reported problems with it.
- VkDeviceIndex selects which vulkan device in the system the render device should use. Type 'GetVkDevices' in the system console to get the list of available devices.

## Description of D3D12Drv specific settings

- UseDebugLayer enables the D3D12 debug layer and will make the render device output extra information into the UnrealTournament.log file for any errors or warnings.

## Description of D3D11Drv VR settings

D3D11Drv can render the game in stereo to an OpenXR headset (SteamVR, Oculus/Meta, WMR). The game is presented as a large virtual screen floating in front of you: the world renders once per eye with real stereo depth, the first person weapon, crosshair and HUD are placed at their own comfortable depths, and head tilt keeps the picture level. Requires the OpenXR runtime of your headset to be active; openxr_loader.dll must be next to the game executable or in the system folder.

- UseVR: Master switch. When enabled, the render device probes for an OpenXR runtime and headset at startup and falls back to normal mono rendering if none is found.
- VRResScale: Eye render resolution as a multiple of the resolution recommended by the runtime (supersampling). 1.0 = recommended, 1.5 = default. Values are clamped to what the runtime allows. Raising it sharpens the image at a GPU cost.
- VRWorldScale: World scale in Unreal units per metre, used to derive the stereo eye separation. Larger values make the world feel smaller. 52.5 is calibrated for UT's proportions.
- VRHeightOffset: Vertical comfort shift of the whole picture in the headset, in screen units. 0 = centred; negative values move the image down.
- VRLookMode: 0 = head look enabled: during gameplay head tilt (roll) is compensated so the horizon stays level, and while a menu is open you can look around the virtual screen to reach its corners. Any other value = the screen is fixed to your head.
- VRHudDepth: Distance (in Unreal units) at which flat HUD elements (bars, ammo counters, chat) converge in stereo during gameplay. Larger = further away.
- VRHudDepthBottom: A nearer convergence distance for the bottom strip of the screen (the weapon bar sits "under your nose", where a close depth is much easier on the eyes). Set to 0 or below to disable and use VRHudDepth everywhere.
- VRHudBottomY: Where the bottom strip begins, as a fraction of screen height (0.7 = bottom 30% of the screen uses VRHudDepthBottom).
- VRUIDepth: Convergence distance for the UI while the mouse is free (menus, console).
- VRCrosshairDepth: Convergence distance of the crosshair. The crosshair is detected as the tile drawn at the screen centre, so custom crosshairs work too.
- VRWeaponIPDScale: Stereo separation of the first person weapon relative to the world (1.0 corresponds to 10% of the eye distance). Lower values flatten the weapon and pull it visually out of your face; the default already reduces it, as the weapon model is rendered extremely close.
- VRHudScaleX, VRHudScaleY: Scale the gameplay HUD towards the screen centre so elements in the corners stay inside the headset's field of view. 1.0 = no scaling, 0.8 = HUD occupies 80% of the screen. Menus are never scaled.
- VRScaleHudMeshes: Also apply the HUD scale (and HUD depth) to 3D meshes that mods draw as part of the HUD after a depth clear (for example rotating radar icons). Off by default; enable if a mod's HUD meshes end up outside the visible area.
- VRClearZBeforeHud: Clear the depth buffer before the HUD is drawn so the HUD and crosshair are never occluded by world geometry. Leave on unless you are debugging.
- VRBrightnessScale, VRBrightnessOffset: Brightness correction applied to the headset image only (eye brightness = game brightness * scale + offset). The desktop window keeps the game's normal brightness. Useful because HMD panels often render the game darker or brighter than a monitor.
- VRMirrorMode: What the desktop window shows while VR is active:
  - 0: Nothing (best performance).
  - 1: The game view, but only while the headset is not being worn (default). Wear detection combines the OpenXR session state with the headset's presence sensor and is polled about once per second.
  - 2: Like 1, but the mirror is also active whenever a menu is open (mouse visible).
  - 3: Always mirror. The mirror renders the mono game view aligned with the window, so menus stay clickable with the mouse.
  - 4: Side-by-side recording mode. The window backbuffer is resized to hold both eye images at full eye resolution side by side, cropped vertically to the game screen. A game recorder that hooks the game itself (Bandicam game recording mode, OBS game capture) will capture a full resolution SBS 3D video; YouTube and 3D players accept it directly. The backbuffer size is locked for the whole session so the recording is never interrupted, and head roll is not applied so the recorded frames stay level. The window itself just shows a squeezed preview in this mode.

Console commands available while D3D11Drv is active:

- `VRRECENTER`: Reset the head reference so the virtual screen re-centres on your current head pose. Bindable (`set input F12 VRRECENTER`).
- `VR GETPARAM <name>`: Prints the current value of any VR setting (for example `VR GETPARAM VRHudDepth`). Intended for mods: `PlayerPawn.ConsoleCommand("VR GETPARAM VRHudDepth")` returns the value as a string, so a mod can draw its own overlays at the depth the user configured, regardless of which VR render device is installed.
- `VR INGAME`: Prints 1 while the mouse is captured (playing) and 0 while a menu or the console is open — tells a mod whether gameplay HUD depth or UI depth currently applies.

## License

Please see LICENSE.md for the details.
