using OxyPlot;
using OxyPlot.Annotations;
using OxyPlot.Axes;
using OxyPlot.Series;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace PlotWarpper
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Point2
    {
        public float X;
        public float Y;
    }

    public class API
    {

        public static IntPtr CreateBitmap(int width, int height)
        {
            System.Drawing.Bitmap img = new System.Drawing.Bitmap(width, height, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            using (var g = System.Drawing.Graphics.FromImage(img))
            {
                g.Clear(System.Drawing.Color.White);
            }
            GCHandle handle = GCHandle.Alloc(img);
            return GCHandle.ToIntPtr(handle);
        }

        public static void DestoryBitmap(IntPtr pointer)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                handle.Free();
            }
        }

        public static void DrawPixel(IntPtr pointer, int x, int y, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                System.Drawing.Bitmap img = handle.Target as System.Drawing.Bitmap;
                if (img != null)
                {
                    img.SetPixel(x, y, System.Drawing.Color.FromArgb(255, r, g, b));
                }
            }
        }

        public static void SaveBitmap(IntPtr pointer, string path)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                System.Drawing.Bitmap img = handle.Target as System.Drawing.Bitmap;
                if (img != null)
                {
                    img.Save(path, System.Drawing.Imaging.ImageFormat.Png);
                }
            }
        }

        public static IntPtr CreatePlot(string title)
        {
            PlotModel model = new PlotModel();
            model.Title = title;

            GCHandle handle = GCHandle.Alloc(model);
            return GCHandle.ToIntPtr(handle);

        }

        public static void DestoryPlot(IntPtr pointer)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                handle.Free();
            }
        }

        public static string GetTitle(IntPtr pointer)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    return model.Title;
                }
            }
            return string.Empty;
        }

        public static void SetTitle(IntPtr pointer, string title)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.Title = title;
                }
            }
        }

        public static unsafe void DrawLineSeries(IntPtr pointer, float* data, int length, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if(model!=null)
                {
                    List<double> ydata = new List<double>();
                    List<double> xdata = Enumerable.Range(0, length-1).Select(x=> (double)x).ToList();
                    for (int i=0; i< length; i++)
                    {
                        ydata.Add((double)data[i]);
                    }
                    model.AddLineSeries(xdata, ydata, OxyColor.FromRgb(r, g, b), MarkerType.None, 1.0);
                }
            }
        }

        public static unsafe void DrawScatterSeries(IntPtr pointer, float* data, int length, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> ydata = new List<double>();
                    List<double> xdata = Enumerable.Range(0, length - 1).Select(x => (double)x).ToList();
                    for (int i = 0; i < length; i++)
                    {
                        ydata.Add((double)data[i]);
                    }
                    model.AddScatterSeries(xdata, ydata, OxyColor.FromRgb(r, g, b), MarkerType.Triangle, 1.0);
                }
            }
        }

        public static unsafe void DrawScatterSeries(IntPtr pointer, Point2* data, int length, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> ydata = new List<double>();
                    List<double> xdata = new List<double>();
                    for (int i = 0; i < length; i++)
                    {
                        xdata.Add((double)data[i].X);
                        ydata.Add((double)data[i].Y);
                    }
                    model.AddScatterSeries(xdata, ydata, OxyColor.FromRgb(r, g, b), MarkerType.Circle, 1.5);
                }
            }
        }

        public static unsafe void DrawLineSeries(IntPtr pointer, List<double> data, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> xdata = Enumerable.Range(0, data.Count - 1).Select(x => (double)x).ToList();
                    model.AddLineSeries(xdata, data, OxyColor.FromRgb(r, g, b), MarkerType.None, 1.0);
                }
            }
        }

        public static unsafe void DrawScatterSeries(IntPtr pointer, List<double> data, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> xdata = Enumerable.Range(0, data.Count - 1).Select(x => (double)x).ToList();
                    model.AddScatterSeries(xdata, data, OxyColor.FromRgb(r, g, b), MarkerType.Triangle, 1.0);
                }
            }
        }

        public static unsafe void DrawScatterSeries(IntPtr pointer, List<Point2> data, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> ydata = new List<double>();
                    List<double> xdata = new List<double>();
                    for (int i = 0; i < data.Count; i++)
                    {
                        xdata.Add((double)data[i].X);
                        ydata.Add((double)data[i].Y);
                    }
                    model.AddScatterSeries(xdata, ydata, OxyColor.FromRgb(r, g, b), MarkerType.Triangle, 3.0);
                }
            }
        }

        public static unsafe void DrawScatter(IntPtr pointer, float x, float y, float p, double mksize, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    byte lr = (byte)(255.0 - (255.0 - (double)r) * (p));
                    byte lg = (byte)(255.0 - (255.0 - (double)g) * (p));
                    byte lb = (byte)(255.0 - (255.0 - (double)b) * (p));
                    ScatterSeries scatterSeries = new ScatterSeries
                    {
                        MarkerFill = OxyColor.FromRgb(lr, lg, lb),
                        MarkerType = MarkerType.Square,
                        MarkerSize = mksize,
                    };

                    scatterSeries.Points.Add(new ScatterPoint(x, y, double.NaN, double.NaN, null));
                    model.Series.Add(scatterSeries);
                }
            }
        }

        public static unsafe void DrawScatterSeries1(IntPtr pointer, List<Point2> data, MarkerType mkty, double mksize, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    List<double> ydata = new List<double>();
                    List<double> xdata = new List<double>();
                    for (int i = 0; i < data.Count; i++)
                    {
                        xdata.Add((double)data[i].X);
                        ydata.Add((double)data[i].Y);
                    }

                    List<double> txdata = new List<double>();
                    List<double> tydata = new List<double>();

                    txdata.Add(xdata[0]);
                    tydata.Add(ydata[0]);

                    for (int i=1; i< data.Count; i++)
                    {
                        if(xdata[i]- xdata[i-1]>1.5)
                        {
                            if (txdata.Count > 0)
                            {
                                model.AddLineSeries(txdata, tydata, OxyColor.FromRgb(r, g, b), LineStyle.Dash);
                                txdata.Clear();
                                tydata.Clear();
                            }
                            txdata.Add(xdata[i]);
                            tydata.Add(ydata[i]);
                        }
                        else
                        {
                            txdata.Add(xdata[i]);
                            tydata.Add(ydata[i]);
                        }
                    }

                    if (txdata.Count > 0)
                    {
                        model.AddLineSeries(txdata, tydata, OxyColor.FromRgb(r, g, b), LineStyle.Dash);
                        txdata.Clear();
                        tydata.Clear();
                    }

                    model.AddScatterSeries(xdata, ydata, OxyColor.FromRgb(r, g, b), mkty,mksize);
                }
            }
        }


        public static unsafe void AddLineAtX(IntPtr pointer, double x, double thick, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.AddLineAtX(x, OxyColor.FromRgb(r, g, b), thick);
                }
            }
        }

        public static unsafe void AddLineMarkerXAt(IntPtr pointer, double x, double y, double miny, double maxy, double yoffset, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.AddLineMarkerXAt(x, y, miny, maxy, yoffset, OxyColor.FromRgb(r, g, b));
                }
            }
        }

        public static unsafe void AddLineAtX1(IntPtr pointer, double x, double thick, LineStyle sty, byte r, byte g, byte b)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.AddLineAtX1(x, OxyColor.FromRgb(r, g, b), sty, thick);
                }
            }
        }

        public static unsafe void RemoveAxis(IntPtr pointer)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.RemoveAxis();
                }
            }
        }

        public static unsafe void SetAxisYLimit(IntPtr pointer, double minx, double maxx)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.SetAxisYLimit(minx, maxx);
                }
            }
        }

        public static unsafe void SetAxisXLabel(IntPtr pointer, string title, double margin, double fontsize=22, double titlefontsize=28)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.Axes.Add(new LinearAxis(AxisPosition.Bottom, title) { AxisTitleDistance=margin, FontSize= fontsize, TitleFontSize= titlefontsize });
                }
            }
        }

        public static unsafe void SetAxisYLabel(IntPtr pointer, string title, double margin, double fontsize = 22, double titlefontsize = 28)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.Axes.Add(new LinearAxis(AxisPosition.Left, title) { AxisTitleDistance = margin, FontSize = fontsize, TitleFontSize = titlefontsize });
                }
            }
        }

        public static void SaveToPdf(IntPtr pointer, string path, int width, int height)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    if (File.Exists(path)) File.Delete(path);
                    using (var stream = File.Create(path))
                    {
                        var pdfExporter = new PdfExporter() { Width = width, Height = height, Background = OxyColors.Transparent };
                        pdfExporter.Export(model, stream);
                    }
                }
            }
        }

        public static void SaveToSvg(IntPtr pointer, string path, int width, int height)
        {
            GCHandle handle = GCHandle.FromIntPtr(pointer);
            if (handle.IsAllocated)
            {
                PlotModel model = handle.Target as PlotModel;
                if (model != null)
                {
                    model.DefaultFontSize = 20;
                    if (File.Exists(path)) File.Delete(path);
                    using (var stream = File.Create(path))
                    {
                        var svgExporter = new SvgExporter1() { Width = width, Height = height, TextMeasurer=new SvgTextMeasurer()};
                        svgExporter.Export(model, stream);
                    }
                }
            }
        }
    }

    public class SvgExporter1
    {
        /// <summary>
        /// Gets or sets the width (in user units) of the output area.
        /// </summary>
        public double Width
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the height (in user units) of the output area.
        /// </summary>
        public double Height
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a value indicating whether the xml headers should be included.
        /// </summary>
        public bool IsDocument
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the text measurer.
        /// </summary>
        public IRenderContext TextMeasurer
        {
            get;
            set;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:OxyPlot.SvgExporter" /> class.
        /// </summary>
        public SvgExporter1()
        {
            this.Width = 600.0;
            this.Height = 400.0;
            this.IsDocument = true;
        }

        /// <summary>
        /// Exports the specified model to a stream.
        /// </summary>
        /// <param name="model">The model.</param>
        /// <param name="stream">The output stream.</param>
        /// <param name="width">The width (points).</param>
        /// <param name="height">The height (points).</param>
        /// <param name="isDocument">if set to <c>true</c>, the xml headers will be included (?xml and !DOCTYPE).</param>
        /// <param name="textMeasurer">The text measurer.</param>
        public static void Export(IPlotModel model, Stream stream, double width, double height, bool isDocument, IRenderContext textMeasurer = null)
        {
            using (SvgRenderContext1 svgRenderContext = new SvgRenderContext1(stream, width, height, true, textMeasurer, model.Background))
            {
                model.Update(true);
                model.Render(svgRenderContext, width, height);
                svgRenderContext.Complete();
                svgRenderContext.Flush();
            }
        }

        /// <summary>
        /// Exports to string.
        /// </summary>
        /// <param name="model">The model.</param>
        /// <param name="width">The width (points).</param>
        /// <param name="height">The height (points).</param>
        /// <param name="isDocument">if set to <c>true</c>, the xml headers will be included (?xml and !DOCTYPE).</param>
        /// <param name="textMeasurer">The text measurer.</param>
        /// <returns>The plot as a svg string.</returns>
        public static string ExportToString(IPlotModel model, double width, double height, bool isDocument, IRenderContext textMeasurer = null)
        {
            string result;
            using (MemoryStream memoryStream = new MemoryStream())
            {
                SvgExporter1.Export(model, memoryStream, width, height, isDocument, textMeasurer);
                memoryStream.Flush();
                memoryStream.Position = 0l;
                StreamReader streamReader = new StreamReader(memoryStream);
                result = streamReader.ReadToEnd();
            }
            return result;
        }

        /// <summary>
        /// Exports the specified <see cref="T:OxyPlot.PlotModel" /> to a <see cref="T:System.IO.Stream" />.
        /// </summary>
        /// <param name="model">The model to export.</param>
        /// <param name="stream">The target stream.</param>
        public void Export(IPlotModel model, Stream stream)
        {
            SvgExporter1.Export(model, stream, this.Width, this.Height, this.IsDocument, this.TextMeasurer);
        }

        /// <summary>
        /// Exports the specified <see cref="T:OxyPlot.PlotModel" /> to a string.
        /// </summary>
        /// <param name="model">The model.</param>
        /// <returns>the SVG content as a string.</returns>
        public string ExportToString(IPlotModel model)
        {
            return SvgExporter1.ExportToString(model, this.Width, this.Height, this.IsDocument, this.TextMeasurer);
        }
    }

    public class SvgRenderContext1 : RenderContextBase, IDisposable
    {
        /// <summary>
        /// The writer.
        /// </summary>
        private readonly SvgWriter w;

        /// <summary>
        /// The disposed flag.
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Gets or sets the text measurer.
        /// </summary>
        /// <value>The text measurer.</value>
        public IRenderContext TextMeasurer
        {
            get;
            set;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:OxyPlot.SvgRenderContext" /> class.
        /// </summary>
        /// <param name="s">The s.</param>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        /// <param name="isDocument">Create an SVG document if set to <c>true</c>.</param>
        /// <param name="textMeasurer">The text measurer.</param>
        /// <param name="background">The background.</param>
        public SvgRenderContext1(Stream s, double width, double height, bool isDocument, IRenderContext textMeasurer, OxyColor background)
        {
            if (textMeasurer == null)
            {
                throw new ArgumentNullException("textMeasurer", "A text measuring render context must be provided.");
            }
            this.w = new SvgWriter(s, width, height, isDocument);
            this.TextMeasurer = textMeasurer;
            if (background.IsVisible())
            {
                this.w.WriteRectangle(0.0, 0.0, width, height, this.w.CreateStyle(background, OxyColors.Undefined, 0.0, null, LineJoin.Miter));
            }
        }

        /// <summary>
        /// Closes the svg writer.
        /// </summary>
        public void Close()
        {
            this.w.Close();
        }

        /// <summary>
        /// Completes the svg element.
        /// </summary>
        public void Complete()
        {
            this.w.Complete();
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Draws an ellipse.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        /// <param name="fill">The fill color.</param>
        /// <param name="stroke">The stroke color.</param>
        /// <param name="thickness">The thickness.</param>
        public override void DrawEllipse(OxyRect rect, OxyColor fill, OxyColor stroke, double thickness)
        {
            this.w.WriteEllipse(rect.Left, rect.Top, rect.Width, rect.Height, this.w.CreateStyle(fill, stroke, thickness, null, LineJoin.Miter));
        }

        /// <summary>
        /// Draws the polyline from the specified points.
        /// </summary>
        /// <param name="points">The points.</param>
        /// <param name="stroke">The stroke color.</param>
        /// <param name="thickness">The stroke thickness.</param>
        /// <param name="dashArray">The dash array.</param>
        /// <param name="lineJoin">The line join type.</param>
        /// <param name="aliased">if set to <c>true</c> the shape will be aliased.</param>
        public override void DrawLine(IList<ScreenPoint> points, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            this.w.WritePolyline(points, this.w.CreateStyle(OxyColors.Undefined, stroke, thickness, dashArray, lineJoin));
        }

        /// <summary>
        /// Draws the polygon from the specified points. The polygon can have stroke and/or fill.
        /// </summary>
        /// <param name="points">The points.</param>
        /// <param name="fill">The fill color.</param>
        /// <param name="stroke">The stroke color.</param>
        /// <param name="thickness">The stroke thickness.</param>
        /// <param name="dashArray">The dash array.</param>
        /// <param name="lineJoin">The line join type.</param>
        /// <param name="aliased">if set to <c>true</c> the shape will be aliased.</param>
        public override void DrawPolygon(IList<ScreenPoint> points, OxyColor fill, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            this.w.WritePolygon(points, this.w.CreateStyle(fill, stroke, thickness, dashArray, lineJoin));
        }

        /// <summary>
        /// Draws the rectangle.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        /// <param name="fill">The fill color.</param>
        /// <param name="stroke">The stroke color.</param>
        /// <param name="thickness">The stroke thickness.</param>
        public override void DrawRectangle(OxyRect rect, OxyColor fill, OxyColor stroke, double thickness)
        {
            this.w.WriteRectangle(rect.Left, rect.Top, rect.Width, rect.Height, this.w.CreateStyle(fill, stroke, thickness, null, LineJoin.Miter));
        }

        /// <summary>
        /// Draws the text.
        /// </summary>
        /// <param name="p">The p.</param>
        /// <param name="text">The text.</param>
        /// <param name="c">The c.</param>
        /// <param name="fontFamily">The font family.</param>
        /// <param name="fontSize">Size of the font.</param>
        /// <param name="fontWeight">The font weight.</param>
        /// <param name="rotate">The rotate.</param>
        /// <param name="halign">The horizontal alignment.</param>
        /// <param name="valign">The vertical alignment.</param>
        /// <param name="maxSize">Size of the max.</param>
        public override void DrawText(ScreenPoint p, string text, OxyColor c, string fontFamily, double fontSize, double fontWeight, double rotate, OxyPlot.HorizontalAlignment halign, OxyPlot.VerticalAlignment valign, OxySize? maxSize)
        {
            if (string.IsNullOrEmpty(text))
            {
                return;
            }
            string[] array = Regex.Split(text, "\r\n");
            if (valign == OxyPlot.VerticalAlignment.Bottom)
            {
                for (int i = array.Length - 1; i >= 0; i--)
                {
                    string text2 = array[i];
                    OxySize oxySize = this.MeasureText(text2, fontFamily, fontSize, fontWeight);
                    this.w.WriteText(p, text2, c, fontFamily, fontSize, fontWeight, rotate, halign, valign);
                    p += new ScreenVector(Math.Sin(rotate / 180.0 * 3.1415926535897931) * oxySize.Height, Math.Cos(rotate / 180.0 * 3.1415926535897931) * oxySize.Height);
                }
                return;
            }
            string[] array2 = array;

            if (text.Contains("."))
            {
                p += new ScreenVector(0.0, 8.0);
            }
            else
            {
                p += new ScreenVector(0.0, 14.0);
            }

            for (int j = 0; j < array2.Length; j++)
            {
                string text3 = array2[j];
                OxySize oxySize2 = this.MeasureText(text3, fontFamily, fontSize, fontWeight);
                this.w.WriteText(p, text3, c, fontFamily, fontSize, fontWeight, rotate, halign, valign);
                p += new ScreenVector(-Math.Sin(rotate / 180.0 * 3.1415926535897931) * oxySize2.Height, Math.Cos(rotate / 180.0 * 3.1415926535897931) * oxySize2.Height);
            }
        }

        /// <summary>
        /// Flushes this instance.
        /// </summary>
        public void Flush()
        {
            this.w.Flush();
        }

        /// <summary>
        /// Measures the text.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="fontFamily">The font family.</param>
        /// <param name="fontSize">Size of the font.</param>
        /// <param name="fontWeight">The font weight.</param>
        /// <returns>The text size.</returns>
        public override OxySize MeasureText(string text, string fontFamily, double fontSize, double fontWeight)
        {
            if (string.IsNullOrEmpty(text))
            {
                return OxySize.Empty;
            }
            return this.TextMeasurer.MeasureText(text, fontFamily, fontSize, fontWeight);
        }

        /// <summary>
        /// Draws the specified portion of the specified <see cref="T:OxyPlot.OxyImage" /> at the specified location and with the specified size.
        /// </summary>
        /// <param name="source">The source.</param>
        /// <param name="srcX">The x-coordinate of the upper-left corner of the portion of the source image to draw.</param>
        /// <param name="srcY">The y-coordinate of the upper-left corner of the portion of the source image to draw.</param>
        /// <param name="srcWidth">Width of the portion of the source image to draw.</param>
        /// <param name="srcHeight">Height of the portion of the source image to draw.</param>
        /// <param name="destX">The x-coordinate of the upper-left corner of drawn image.</param>
        /// <param name="destY">The y-coordinate of the upper-left corner of drawn image.</param>
        /// <param name="destWidth">The width of the drawn image.</param>
        /// <param name="destHeight">The height of the drawn image.</param>
        /// <param name="opacity">The opacity.</param>
        /// <param name="interpolate">Interpolate if set to <c>true</c>.</param>
        public override void DrawImage(OxyImage source, double srcX, double srcY, double srcWidth, double srcHeight, double destX, double destY, double destWidth, double destHeight, double opacity, bool interpolate)
        {
            this.w.WriteImage(srcX, srcY, srcWidth, srcHeight, destX, destY, destWidth, destHeight, source);
        }

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources
        /// </summary>
        /// <param name="disposing"><c>true</c> to release both managed and unmanaged resources; <c>false</c> to release only unmanaged resources.</param>
        private void Dispose(bool disposing)
        {
            if (!this.disposed && disposing)
            {
                this.w.Dispose();
            }
            this.disposed = true;
        }
    }

    public class SvgTextMeasurer : IRenderContext
    {
        bool IRenderContext.RendersToScreen
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        void IRenderContext.CleanUp()
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawEllipse(OxyRect extents, OxyColor fill, OxyColor stroke, double thickness)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawEllipses(IList<OxyRect> extents, OxyColor fill, OxyColor stroke, double thickness)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawImage(OxyImage source, double srcX, double srcY, double srcWidth, double srcHeight, double destX, double destY, double destWidth, double destHeight, double opacity, bool interpolate)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawLine(IList<ScreenPoint> points, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawLineSegments(IList<ScreenPoint> points, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawPolygon(IList<ScreenPoint> points, OxyColor fill, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawPolygons(IList<IList<ScreenPoint>> polygons, OxyColor fill, OxyColor stroke, double thickness, double[] dashArray, LineJoin lineJoin, bool aliased)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawRectangle(OxyRect rectangle, OxyColor fill, OxyColor stroke, double thickness)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawRectangles(IList<OxyRect> rectangles, OxyColor fill, OxyColor stroke, double thickness)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.DrawText(ScreenPoint p, string text, OxyColor fill, string fontFamily, double fontSize, double fontWeight, double rotation, OxyPlot.HorizontalAlignment horizontalAlignment, OxyPlot.VerticalAlignment verticalAlignment, OxySize? maxSize)
        {
            throw new NotImplementedException();
        }

        OxySize IRenderContext.MeasureText(string text, string fontFamily, double fontSize, double fontWeight)
        {
            FormattedText ft = new FormattedText(text,
                                               CultureInfo.CurrentCulture,
                                               FlowDirection.LeftToRight,
                                               new Typeface(new FontFamily(fontFamily), FontStyles.Normal, FontWeight.FromOpenTypeWeight((int)fontWeight), FontStretches.Normal),
                                               fontSize,
                                               Brushes.Black);
            return new OxySize(ft.Width, ft.Height+12);
        }

        void IRenderContext.ResetClip()
        {
            throw new NotImplementedException();
        }

        bool IRenderContext.SetClip(OxyRect clippingRectangle)
        {
            throw new NotImplementedException();
        }

        void IRenderContext.SetToolTip(string text)
        {
            throw new NotImplementedException();
        }
    }


    public static class Common
    {
        public static void RemoveAxis(this PlotModel model)
        {
            if (model.Axes.Count == 0)
            {
                model.Axes.Add(new LinearAxis()
                {
                    TicklineColor = OxyColors.Transparent,
                    MajorTickSize = 0.0,
                    MinorTickSize = 0.0,
                    TickStyle = TickStyle.None,
                    TextColor = OxyColors.Transparent,
                    Position = AxisPosition.Bottom,
                    IsAxisVisible = false
                });

                model.Axes.Add(new LinearAxis()
                {
                    TicklineColor = OxyColors.Transparent,
                    MajorTickSize = 0.0,
                    MinorTickSize = 0.0,
                    TickStyle = TickStyle.None,
                    TextColor = OxyColors.Transparent,
                    Position = AxisPosition.Left,
                    IsAxisVisible = false
                });
            }
            else if(model.Axes.Count == 1)
            {
                if(model.Axes[0].Position== AxisPosition.Bottom)
                {
                    model.Axes.Add(new LinearAxis()
                    {
                        TicklineColor = OxyColors.Transparent,
                        MajorTickSize = 0.0,
                        MinorTickSize = 0.0,
                        TickStyle = TickStyle.None,
                        TextColor = OxyColors.Transparent,
                        Position = AxisPosition.Left,
                        IsAxisVisible = false
                    });
                }
                else if(model.Axes[0].Position == AxisPosition.Left)
                {
                    model.Axes.Add(new LinearAxis()
                    {
                        TicklineColor = OxyColors.Transparent,
                        MajorTickSize = 0.0,
                        MinorTickSize = 0.0,
                        TickStyle = TickStyle.None,
                        TextColor = OxyColors.Transparent,
                        Position = AxisPosition.Bottom,
                        IsAxisVisible = false
                    });
                }
                foreach (var axis in model.Axes)
                {
                    axis.TicklineColor = OxyColors.Transparent;
                    axis.MajorTickSize = 0.0;
                    axis.MinorTickSize = 0.0;
                    axis.TickStyle = TickStyle.None;
                    axis.TextColor = OxyColors.Transparent;
                    axis.IsAxisVisible = false;
                }
            }
            else
            {
                foreach (var axis in model.Axes)
                {
                    axis.TicklineColor = OxyColors.Transparent;
                    axis.MajorTickSize = 0.0;
                    axis.MinorTickSize = 0.0;
                    axis.TickStyle = TickStyle.None;
                    axis.TextColor = OxyColors.Transparent;
                    axis.IsAxisVisible = false;
                }
            }
            model.PlotAreaBorderColor = OxyColors.Transparent;
        }

        public static void SetAxisYLimit(this PlotModel model, double minx, double maxx)
        {
            if (model.DefaultYAxis != null)
            {
                model.DefaultYAxis.Minimum = minx;
                model.DefaultYAxis.Maximum = maxx;
            }
            else
            {
                model.Axes.Add(new LinearAxis(AxisPosition.Left, minx, maxx, 1.0, 1.0));
            }
        }

        public static void AddLineAtX1(this PlotModel model, double x, OxyColor color, LineStyle sty, double thick = 1.0)
        {
            model.Annotations.Add(new LineAnnotation() { Type = LineAnnotationType.Vertical, X = x, Color = color, LineStyle = sty, StrokeThickness = thick });
        }

        public static void AddLineAtX(this PlotModel model, double x, OxyColor color, double thick=1.0)
        {
            model.Annotations.Add(new LineAnnotation() { Type = LineAnnotationType.Vertical, X = x, Color = color, LineStyle = LineStyle.Dash, StrokeThickness = thick });
        }

        public static void AddLineMarkerXAt(this PlotModel model, double x, double y, double minx, double maxy, double yoffset, OxyColor color)
        {
            model.Annotations.Add(new LineAnnotation(){ Type = LineAnnotationType.Vertical, X = x, MinimumY= minx, MaximumY= maxy, Color = color, LineStyle = LineStyle.Dash, StrokeThickness = 1 });

            model.Annotations.Add(new TextAnnotation() {Text= "×", TextColor=color, TextPosition=new DataPoint(x, y+yoffset), FontSize=12, Stroke=OxyColors.Transparent, StrokeThickness=0, Background=OxyColors.Transparent, TextVerticalAlignment= OxyPlot.VerticalAlignment.Middle, TextHorizontalAlignment=OxyPlot.HorizontalAlignment.Center });
            }

        public static void AddLineSeries(this PlotModel model, IEnumerable<double> xSeries, IEnumerable<double> ySeries, OxyColor color, MarkerType mk, double mksize)
        {
            var lineSeries = new LineSeries();
            lineSeries.Color = color;
            foreach (var current in xSeries.Zip(ySeries, (double x, double y) => new
            {
                x,
                y
            }))
            {
                if (!double.IsNaN(current.y) && !double.IsInfinity(current.y))
                {
                    lineSeries.Points.Add(new DataPoint(current.x, current.y));
                }
            }
            model.Series.Add(lineSeries);
        }

        public static void AddLineSeries(this PlotModel model, IEnumerable<double> xSeries, IEnumerable<double> ySeries, OxyColor color, LineStyle sty)
        {
            var lineSeries = new LineSeries();
            lineSeries.Color = color;
            lineSeries.LineStyle = sty;
            foreach (var current in xSeries.Zip(ySeries, (double x, double y) => new
            {
                x,
                y
            }))
            {
                if (!double.IsNaN(current.y) && !double.IsInfinity(current.y))
                {
                    lineSeries.Points.Add(new DataPoint(current.x, current.y));
                }
            }
            model.Series.Add(lineSeries);
        }

        public static void AddScatterSeries(this PlotModel model, IEnumerable<double> xSeries, IEnumerable<double> ySeries, OxyColor color, MarkerType mk, double mksize)
        {
            ScatterSeries scatterSeries = new ScatterSeries
            {
                MarkerFill = color,
                MarkerType = mk,
                MarkerSize = mksize,
            };
            foreach (var current in xSeries.Zip(ySeries, (double x, double y) => new
            {
                x,
                y
            }))
            {
                scatterSeries.Points.Add(new ScatterPoint(current.x, current.y, double.NaN, double.NaN, null));
            }
            model.Series.Add(scatterSeries);
        }

        public static void AddHighlightedPoint(this PlotModel model, double x, double y, OxyColor color, MarkerType mk, double mksize)
        {
            ScatterSeries scatterSeries = new ScatterSeries
            {
                MarkerFill = color,
                MarkerType = mk,
                MarkerSize = mksize
            };
            scatterSeries.Points.Add(new ScatterPoint(x, y, double.NaN, double.NaN, null));
            model.Series.Add(scatterSeries);
        }

        public static void AddCustomLegend(this PlotModel model, string title, OxyColor color, MarkerType mk = MarkerType.None, double mksize = 1.0)
        {
            ScatterSeries scatterSeries = new ScatterSeries
            {
                MarkerFill = color,
                MarkerType = mk,
                MarkerSize = mksize,
                Title = title
            };
            model.Series.Add(scatterSeries);
        }
    }
}
