#include "Plot.h"
#include "Graphics_impl.h"
using namespace rsdn;
using namespace rsdn::graphics;

Plot2D::Plot2D()
{
	warpper = Graphics_impl::CreatePlot();
}

Plot2D::~Plot2D()
{
	if (warpper) Graphics_impl::DestoryPlot(warpper);
}

const std::wstring& Plot2D::GetTitle() const
{
	if (warpper) return Graphics_impl::GetTitle(warpper);
	return L"";
}

void Plot2D::SetTitle(const std::wstring& title)
{
	if (warpper) return Graphics_impl::SetTitle(warpper, title);
}

void Plot2D::AddScatter(datatype x, datatype ym, datatype markersize, unsigned r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddScatter(warpper, x, ym, markersize, r, g, b);
}

void Plot2D::AddLineSeries(const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddLineSeries(warpper, data, length, r, g, b);
}

void Plot2D::AddScatterSeries(const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddScatterSeries(warpper, data, length, r, g, b);
}

void Plot2D::AddScatterSeries(const std::pair<datatype, datatype>* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddScatterSeries(warpper, data, length, r, g, b);
}

void Plot2D::AddLineAtX(datatype x, datatype thick, unsigned char r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddLineAtX(warpper, x, thick, r, g, b);
}

void Plot2D::RemoveAxes()
{
	if (warpper) Graphics_impl::RemoveAxes(warpper);
}

void Plot2D::AddLineMarkerXAt(datatype x, datatype y, datatype miny, datatype maxy, datatype yoffset, unsigned char r, unsigned char g, unsigned char b)
{
	if (warpper) Graphics_impl::AddLineMarkerXAt(warpper, x, y, miny, maxy, yoffset, r, g, b);
}

void Plot2D::SetAxisYLimit(datatype minx, datatype maxx)
{
	if (warpper) Graphics_impl::SetAxisYLimit(warpper, minx, maxx);
}

void Plot2D::SaveAsPdf(const std::wstring& path, int width, int height)
{
	if (warpper) Graphics_impl::SaveAsPdf(warpper, path, width, height);
}

void Plot2D::SaveAsSvg(const std::wstring& path, int width, int height)
{
	if (warpper) Graphics_impl::SaveAsSvg(warpper, path, width, height);
}