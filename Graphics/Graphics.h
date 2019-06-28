#pragma once
#include "Plot.h"
#include "Image.h"

namespace rsdn
{
	namespace graphics
	{
		class Graphics_impl;

		class GRAPHICS_API Graphics
		{
		public:
			static void Initilize();
			static void Shutdown();
		};
	}
}