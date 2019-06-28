#include "Graphics_impl.h"
using namespace rsdn;
using namespace rsdn::graphics;

HMODULE Graphics_impl::lib = NULL;

static void*(*PlotAPI_CreatePlot)(const std::wstring& title);
static void(*PlotAPI_DestoryPlot)(void* plot);
static std::wstring(*PlotAPI_GetTitle)(void* plot);
static void(*PlotAPI_SetTitle)(void* plot, const std::wstring& title);
static void(*PlotAPI_AddScatter)(void* plot, double x, double y, double p, double mks, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_AddLineSeries)(void* plot, float* data, size_t len, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_AddScatterSeries)(void* plot, float* data, size_t len, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_AddScatterSeries2)(void* plot, Point2* data, size_t len, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_AddLineAtX)(void* plot, double x, double thick, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_RemoveAxes)(void* plot);
static void(*PlotAPI_AddLineMarkerXAt)(void* plot, double x, double y, double miny, double maxy, double yoffset, unsigned char r, unsigned char g, unsigned char b);
static void(*PlotAPI_SetAxisYLimit)(void* plot, double minx, double maxx);
static void(*PlotAPI_Save)(void* plot, const std::wstring& path, int width, int height);
static void(*PlotAPI_Save1)(void* plot, const std::wstring& path, int width, int height);

static void* (*PlotAPI_CreateImage)(int w, int h);
static void (*PlotAPI_DestoryImage)(void*);
static void (*PlotAPI_DrawPixel)(void*, int x, int y, unsigned char r, unsigned char g, unsigned char b);
static void (*PlotAPI_SaveImage)(void*, const std::wstring& path);

void Graphics_impl::Initilize()
{
	if (!lib)
	{
		lib = LoadLibraryW(L"NativePlotWarpper.dll");
		PlotAPI_CreatePlot = (void* (*)(const std::wstring&))GetProcAddress(lib, "CreatePlot");
		PlotAPI_DestoryPlot = (void(*)(void*))GetProcAddress(lib, "DestoryPlot");
		PlotAPI_GetTitle = (std::wstring(*)(void*))GetProcAddress(lib, "GetTitle");
		PlotAPI_SetTitle = (void (*)(void*, const std::wstring&))GetProcAddress(lib, "SetTitle");
		PlotAPI_AddLineSeries = (void(*)(void*, float*, size_t, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "AddLineSeries");
		PlotAPI_AddScatter = (void(*)(void*, double, double, double, double, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "AddScatter");
		PlotAPI_AddScatterSeries = (void(*)(void*, float*, size_t, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "AddScatterSeries");
		PlotAPI_AddScatterSeries2 = (void(*)(void*, Point2*, size_t, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "AddScatterSeries2");
		PlotAPI_AddLineAtX = (void(*)(void*, double, double, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "AddLineAtX");
		PlotAPI_RemoveAxes = (void(*)(void*))GetProcAddress(lib, "RemoveAxes");
		PlotAPI_AddLineMarkerXAt = (void(*)(void*, double, double, double, double, double, unsigned char, unsigned char, unsigned char)) GetProcAddress(lib, "AddLineMarkerXAt");
		PlotAPI_SetAxisYLimit = (void(*)(void*, double, double)) GetProcAddress(lib, "SetAxisYLimit");
		PlotAPI_Save = (void(*)(void*, const std::wstring&, int, int))GetProcAddress(lib, "Save");
		PlotAPI_Save1 = (void(*)(void*, const std::wstring&, int, int))GetProcAddress(lib, "Save1");

		PlotAPI_CreateImage = (void* (*)(int, int))GetProcAddress(lib, "CreateImage");
		PlotAPI_DestoryImage = (void(*)(void*))GetProcAddress(lib, "DestoryImage");
		PlotAPI_DrawPixel= (void(*)(void*, int, int, unsigned char, unsigned char, unsigned char))GetProcAddress(lib, "DrawPixel");
		PlotAPI_SaveImage = (void(*)(void*, const std::wstring&))GetProcAddress(lib, "SaveImage");
	}
}

void Graphics_impl::Shutdown()
{
	if (lib)
	{
		PlotAPI_CreatePlot = nullptr;
		PlotAPI_DestoryPlot = nullptr;
		PlotAPI_GetTitle = nullptr;
		PlotAPI_SetTitle = nullptr;
		PlotAPI_AddLineSeries = nullptr;
		PlotAPI_AddScatter = nullptr;
		PlotAPI_AddScatterSeries = nullptr;
		PlotAPI_AddScatterSeries2 = nullptr;
		PlotAPI_AddLineAtX = nullptr;
		PlotAPI_RemoveAxes = nullptr;
		PlotAPI_AddLineMarkerXAt = nullptr;
		PlotAPI_SetAxisYLimit = nullptr;
		PlotAPI_Save = nullptr;
		PlotAPI_Save1 = nullptr;
	}
}


void* Graphics_impl::CreatePlot()
{
	if (PlotAPI_CreatePlot) return PlotAPI_CreatePlot(L"");
	return nullptr;
}

void Graphics_impl::DestoryPlot(void*& plot)
{
	if (PlotAPI_DestoryPlot && plot)
	{
		PlotAPI_DestoryPlot(plot);
		plot = nullptr;
	}
}

const std::wstring& Graphics_impl::GetTitle(void* plot)
{
	if (PlotAPI_GetTitle) return PlotAPI_GetTitle(plot);
	return L"";
}

void Graphics_impl::SetTitle(void* plot, const std::wstring& title)
{
	if (PlotAPI_SetTitle) PlotAPI_SetTitle(plot, title);
}

void Graphics_impl::AddScatter(void* plot, datatype x, datatype ym, datatype markersize, unsigned r, unsigned char g, unsigned char b)
{
	if (PlotAPI_AddScatter) PlotAPI_AddScatter(plot, x, ym, 1.0, markersize, r, g, b);
}

void Graphics_impl::AddLineSeries(void* plot, const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	datatype* dataptr = const_cast<datatype*>(data);
	if (PlotAPI_AddLineSeries) PlotAPI_AddLineSeries(plot, dataptr, length, r, g, b);
}

void Graphics_impl::AddScatterSeries(void* plot, const datatype* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	datatype* dataptr = const_cast<datatype*>(data);
	if (PlotAPI_AddScatterSeries) PlotAPI_AddScatterSeries(plot, dataptr, length, r, g, b);
}

void Graphics_impl::AddScatterSeries(void* plot, const std::pair<datatype, datatype>* data, size_t length, unsigned r, unsigned char g, unsigned char b)
{
	std::pair<datatype, datatype>* dataptr = const_cast<std::pair<datatype, datatype>*>(data);
	if (PlotAPI_AddScatterSeries2) PlotAPI_AddScatterSeries2(plot, (Point2*)dataptr, length, r, g, b);
}

void Graphics_impl::AddLineAtX(void* plot, datatype x, datatype thick, unsigned char r, unsigned char g, unsigned char b)
{
	if (PlotAPI_AddLineAtX) PlotAPI_AddLineAtX(plot, x, thick, r, g, b);
}

void Graphics_impl::RemoveAxes(void* plot)
{
	if (PlotAPI_RemoveAxes) PlotAPI_RemoveAxes(plot);
}

void Graphics_impl::AddLineMarkerXAt(void* plot, datatype x, datatype y, datatype miny, datatype maxy, datatype yoffset, unsigned char r, unsigned char g, unsigned char b)
{
	if (PlotAPI_AddLineMarkerXAt) PlotAPI_AddLineMarkerXAt(plot, x, y, miny, maxy, yoffset, r, g, b);
}

void Graphics_impl::SetAxisYLimit(void* plot, datatype minx, datatype maxx)
{
	if (PlotAPI_SetAxisYLimit) PlotAPI_SetAxisYLimit(plot, minx, maxx);
}

void Graphics_impl::SaveAsPdf(void* plot, const std::wstring& path, int width, int height)
{
	if (PlotAPI_Save) PlotAPI_Save(plot, path, width, height);
}

void Graphics_impl::SaveAsSvg(void* plot, const std::wstring& path, int width, int height)
{
	if (PlotAPI_Save1) PlotAPI_Save1(plot, path, width, height);
}

void Graphics::Initilize()
{
	Graphics_impl::Initilize();
}

void Graphics::Shutdown()
{
	Graphics_impl::Shutdown();
}


void* Graphics_impl::CreateImage(int w, int h)
{
	if (PlotAPI_CreateImage) return PlotAPI_CreateImage(w, h);
	return nullptr;
}

void Graphics_impl::DestoryImage(void*& bmp)
{
	if (PlotAPI_DestoryImage && bmp)
	{
		PlotAPI_DestoryImage(bmp);
		bmp = nullptr;
	}
}

void Graphics_impl::DrawPixel(void* bmp, int x, int y, unsigned r, unsigned char g, unsigned char b)
{
	if (PlotAPI_DrawPixel) PlotAPI_DrawPixel(bmp, x, y, r, g, b);
}

void Graphics_impl::SaveImage(void* bmp, const std::wstring& path)
{
	if (PlotAPI_SaveImage) PlotAPI_SaveImage(bmp, path);
}