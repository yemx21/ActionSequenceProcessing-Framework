#pragma once
#include "..\Core\Core.h"
#include "..\Learning\Learning.h"
#ifndef HUMAN_CAPI
#ifdef HUMAN_EXPORTS
#define HUMAN_CAPI extern "C" __declspec(dllexport)
#else
#define HUMAN_CAPI extern "C" __declspec(dllimport)
#endif
#endif

#ifndef HUMAN_API
#ifdef HUMAN_EXPORTS
#define HUMAN_API __declspec(dllexport)
#else
#define HUMAN_API __declspec(dllimport)
#endif
#endif
