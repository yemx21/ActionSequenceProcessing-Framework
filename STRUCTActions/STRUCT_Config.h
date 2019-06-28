#pragma once
#include "..\Core\Core.h"
#include "..\Learning\Learning.h"
#ifndef STRUCT_CAPI
#ifdef STRUCT_EXPORTS
#define STRUCT_CAPI extern "C" __declspec(dllexport)
#else
#define STRUCT_CAPI extern "C" __declspec(dllimport)
#endif
#endif

#ifndef STRUCT_API
#ifdef STRUCT_EXPORTS
#define STRUCT_API __declspec(dllexport)
#else
#define STRUCT_API __declspec(dllimport)
#endif
#endif
