#pragma once
#include "..\Core\Core.h"
#ifndef LEARNING_CAPI
#ifdef LEARNING_EXPORTS
#define LEARNING_CAPI extern "C" __declspec(dllexport)
#else
#define LEARNING_CAPI extern "C" __declspec(dllimport)
#endif
#endif

#ifndef LEARNING_API
#ifdef LEARNING_EXPORTS
#define LEARNING_API __declspec(dllexport)
#else
#define LEARNING_API __declspec(dllimport)
#endif
#endif
