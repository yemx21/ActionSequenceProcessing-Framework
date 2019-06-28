#include "Image.h"
#include "Graphics_impl.h"
using namespace rsdn::graphics;

Image::Image(int width, int height)
{
	warpper = Graphics_impl::CreateImage(width, height);
}

Image::~Image()
{
	if (warpper) Graphics_impl::DestoryImage(warpper);
}


void Image::Draw(int x, int y, unsigned r, unsigned char g, unsigned char b)
{
	if (warpper) return Graphics_impl::DrawPixel(warpper, x, y, r, g, b);
}

void Image::SaveAsPng(const std::wstring& path)
{
	if (warpper) return Graphics_impl::SaveImage(warpper, path);
}