#pragma once
#include "Graphics.h"

namespace rsdn
{
	namespace graphics
	{
		class Graphics_impl
		{
		private:
			static HMODULE lib;
		public:
			static void Initilize();
			static void Shutdown();


		public:
			static void* CreatePlot();
			static void DestoryPlot(void*&);
			static const std::wstring& GetTitle(void*);
			static void SetTitle(void*, const std::wstring&);

			static void AddScatter(void*, datatype x, datatype ym, datatype markersize, unsigned r, unsigned char g, unsigned char b);
			static void AddLineSeries(void*, const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			static void AddScatterSeries(void*, const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			static void AddScatterSeries(void*, const std::pair<datatype, datatype>* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			static void AddLineAtX(void*, datatype x, datatype thick, unsigned char r, unsigned char g, unsigned char b);
			static void RemoveAxes(void*);
			static void AddLineMarkerXAt(void*, datatype x, datatype y, datatype miny, datatype maxy, datatype yoffset, unsigned char r, unsigned char g, unsigned char b);
			static void SetAxisYLimit(void*, datatype minx, datatype maxx);
			static void SaveAsPdf(void*, const std::wstring& path, int width, int height);
			static void SaveAsSvg(void*, const std::wstring& path, int width, int height);

			static void* CreateImage(int w, int h);
			static void DestoryImage(void*&);
			static void DrawPixel(void*, int x, int y, unsigned r, unsigned char g, unsigned char b);
			static void SaveImage(void*, const std::wstring& path);
		};
	}
}