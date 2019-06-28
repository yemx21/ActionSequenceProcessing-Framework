#if _DEBUG
#using "..\Debug\PlotWarpper.dll"
#else
#using "..\Release\PlotWarpper.dll"
#endif
#include "API.h"
#include <msclr\marshal_cppstd.h>  
#include <Windows.h>
#include <unordered_map>

#define CAPI extern "C" __declspec(dllexport)
using namespace msclr::interop;
using namespace System;
CAPI void* CreatePlot(const std::wstring& title)
{
	String^ clrtitle = marshal_as<String^>(title);
	auto ptr= PlotWarpper::API::CreatePlot(clrtitle);
	return ptr.ToPointer();
}

CAPI void DestoryPlot(void* plot)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::DestoryPlot(ptr);
}

CAPI const std::wstring GetPlotTitle(void* plot)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	String^ clrtitle = PlotWarpper::API::GetTitle(ptr);
	return marshal_as<std::wstring>(clrtitle);
}

CAPI void SetPlotTitle(void* plot, const std::wstring& title)
{
	String^ clrtitle = marshal_as<String^>(title);
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::SetTitle(ptr, clrtitle);
}

CAPI void AddLineSeries(void* plot, float* data, size_t len, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::DrawLineSeries(ptr, data, (int)len, r, g, b);
}

CAPI void AddScatterSeries(void* plot, float* data, size_t len, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::DrawScatterSeries(ptr, data, (int)len, r, g, b);
}

struct Point2
{
	float X;
	float Y;

	Point2(float x, float y) :X(x), Y(y) {}
};

CAPI void AddScatter(void* plot, double x, double y, double p, double mks, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::DrawScatter(ptr, x, y, p, mks, r, g,b);
}

CAPI void AddScatterSeries2(void* plot, Point2* data, size_t length, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::DrawScatterSeries(ptr, (PlotWarpper::Point2*)data, (int)length, r, g, b);
}

CAPI void RemoveAxes(void* plot)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::RemoveAxis(ptr);
}

CAPI void AddLineAtX(void* plot, double x, double thick, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::AddLineAtX(ptr, x, thick, r, g, b);
}

CAPI void AddLineMarkerXAt(void* plot, double x, double y, double miny, double maxy, double yoffset, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::AddLineMarkerXAt(ptr, x, y, miny, maxy, yoffset, r, g, b);
}

CAPI void SetAxisYLimit(void* plot, double minx, double maxx)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::SetAxisYLimit(ptr, minx, maxx);
}

CAPI void Save(void* plot, const std::wstring& path, int width, int height)
{
	String^ clrpath = marshal_as<String^>(path);
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::SaveToPdf(ptr, clrpath, width, height);
}

CAPI void Save1(void* plot, const std::wstring& path, int width, int height)
{
	String^ clrpath = marshal_as<String^>(path);
	System::IntPtr ptr = System::IntPtr::IntPtr(plot);
	PlotWarpper::API::SaveToSvg(ptr, clrpath, width, height);
}


CAPI void* CreateImage(int width, int height)
{
	auto ptr = PlotWarpper::API::CreateBitmap(width, height);
	return ptr.ToPointer();
}

CAPI void DestoryImage(void* bmp)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(bmp);
	PlotWarpper::API::DestoryBitmap(ptr);
}

CAPI void DrawPixel(void* bmp, int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	System::IntPtr ptr = System::IntPtr::IntPtr(bmp);
	PlotWarpper::API::DrawPixel(ptr, x, y, r, g, b);
}

CAPI void SaveImage(void* bmp, const std::wstring& path)
{
	String^ clrpath = marshal_as<String^>(path);
	System::IntPtr ptr = System::IntPtr::IntPtr(bmp);
	PlotWarpper::API::SaveBitmap(ptr, clrpath);
}