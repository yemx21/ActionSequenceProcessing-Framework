#pragma once
#include "Graphics_Config.h"
#include <string>
#include <memory>

namespace rsdn
{
	namespace graphics
	{
		class GRAPHICS_API Plot2D
		{
		private:
			void* warpper;
		public:
			Plot2D();
			~Plot2D();

			const std::wstring& GetTitle() const;
			void SetTitle(const std::wstring&);

			void AddScatter(datatype x, datatype ym, datatype markersize, unsigned r, unsigned char g, unsigned char b);
			void AddLineSeries(const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			void AddScatterSeries(const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			void AddScatterSeries(const std::pair<datatype, datatype>* data, size_t length, unsigned r, unsigned char g, unsigned char b);
			void AddLineAtX(datatype x, datatype thick, unsigned char r, unsigned char g, unsigned char b);
			void RemoveAxes();
			void AddLineMarkerXAt(datatype x, datatype y, datatype miny, datatype maxy, datatype yoffset, unsigned char r, unsigned char g, unsigned char b);
			void SetAxisYLimit(datatype minx, datatype maxx);
			void SaveAsPdf(const std::wstring& path, int width, int height);
			void SaveAsSvg(const std::wstring& path, int width, int height);

			_declspec(property(get = GetTitle, put = SetTitle)) const std::wstring& Title;
		};

		typedef std::shared_ptr<Plot2D> Plot2DPtr;
	}
}