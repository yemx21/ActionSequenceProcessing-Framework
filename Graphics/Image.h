#pragma once
#include "Graphics_Config.h"
#include <string>
#include <memory>

namespace rsdn
{
	namespace graphics
	{
		class GRAPHICS_API Image
		{
		private:
			void* warpper;
		public:
			Image(int width, int height);
			~Image();

			void Draw(int x, int y, unsigned r, unsigned char g, unsigned char b);
			void SaveAsPng(const std::wstring& path);
		};

		typedef std::shared_ptr<Image> ImagePtr;
	}
}
