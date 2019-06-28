#pragma once
#ifndef GRAPHICS_CAPI
#ifdef GRAPHICS_EXPORTS
#define GRAPHICS_CAPI extern "C" __declspec(dllexport)
#else
#define GRAPHICS_CAPI extern "C" __declspec(dllimport)
#endif
#endif

#ifndef GRAPHICS_API
#ifdef GRAPHICS_EXPORTS
#define GRAPHICS_API __declspec(dllexport)
#else
#define GRAPHICS_API __declspec(dllimport)
#endif
#endif

#include "..\Core\Core.h"