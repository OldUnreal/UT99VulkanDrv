#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#pragma pack(push, 8)
#include <Windows.h>
#include <D3D11.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#undef min
#undef max
#include <mutex>
#include <vector>
#include <algorithm>
#include <memory>
#include <map>
#include <unordered_map>
#pragma pack(pop)

#define UTGLR_NO_APP_MALLOC
#include <stdlib.h>

#ifdef _MSC_VER
#pragma warning(disable: 4297) // warning C4297: 'UObject::operator delete': function assumed not to throw an exception but does
#endif

#include "Engine.h"
#include "UnRender.h"

#if !defined(UNREALGOLD) && !defined(DEUSEX)
#define OLDUNREAL469SDK
#endif

#if defined(OLDUNREAL469SDK)
#include "Render.h"
#endif
