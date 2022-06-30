///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxImagePanel.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxImagePanel.h"
#include "wxNcVisFrame.h"
#include "utf8_to_utf32.h"

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(wxImagePanel, wxPanel)
EVT_PAINT(wxImagePanel::OnPaint)
EVT_SIZE(wxImagePanel::OnSize)
EVT_IDLE(wxImagePanel::OnIdle)
EVT_LEFT_DCLICK(wxImagePanel::OnMouseLeftDoubleClick)
EVT_MOTION(wxImagePanel::OnMouseMotion)
EVT_LEAVE_WINDOW(wxImagePanel::OnMouseLeaveWindow)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Default width of the image panel.
///	</summary>
static const int DISPLAY_WIDTH_DEFAULT = 720;

///	<summary>
///		Default height of the image panel.
///	</summary>
static const int DISPLAY_HEIGHT_DEFAULT = 360;

///	<summary>
///		Border size around image panel.
///	</summary>
static const int DISPLAY_BORDER = 1;

///	<summary>
///		Default width of the label bar.
///	</summary>
static const int LABELBAR_IMAGEWIDTH = 120;

///	<summary>
///		Number of boxes in label bar.
///	</summary>
static const int LABELBAR_BOXCOUNT = 16;

///	<summary>
///		Font height of the label bar.
///	</summary>
static const int LABELBAR_FONTHEIGHT = 14;

////////////////////////////////////////////////////////////////////////////////

wxImagePanel::wxImagePanel(
	wxFrame * parent,
	const GridDataSampler * pgds,
	const DataArray1D<float> * pdata
) :
	wxPanel(parent),
	m_pgds(pgds),
	m_pdata(pdata),
	m_fResize(false)
{
	wxNcVisFrame * ncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	_ASSERT(ncvisparent != NULL);

	int nPanelWidth = DISPLAY_WIDTH_DEFAULT + LABELBAR_IMAGEWIDTH + 2*DISPLAY_BORDER;
	int nPanelHeight = DISPLAY_HEIGHT_DEFAULT + 2*DISPLAY_BORDER;

	m_image.Create(nPanelWidth, nPanelHeight);

	ColorMapLibrary colormaplib;
	SetColorMap(colormaplib.GetColorMapName(0));

	m_sft.xScale = LABELBAR_FONTHEIGHT;
	m_sft.yScale = LABELBAR_FONTHEIGHT;
	m_sft.xOffset = 0;
	m_sft.yOffset = 0;
	m_sft.flags = SFT_DOWNWARD_Y;

	std::string strFontPath = ncvisparent->GetResourceDir() + "/Ubuntu-Regular.ttf";

	m_sft.font = sft_loadfile(strFontPath.c_str());
	if (m_sft.font == NULL) {
		std::cout << "ERROR loading \"" << strFontPath.c_str() <<"\"" << std::endl;
		_EXCEPTION();
	}

	SetSize(wxSize(nPanelWidth, nPanelHeight));
	SetMinSize(wxSize(nPanelWidth, nPanelHeight));
	SetCoordinateRange(0.0, 360.0, -90.0, 90.0);

	m_dDataRange[0] = 0.0;
	m_dDataRange[1] = 1.0;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnPaint(wxPaintEvent & evt) {
	std::cout << "PAINT" << std::endl;
	// depending on your system you may need to look at double-buffered dcs
	wxPaintDC dc(this);
	Render(dc);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnSize(wxSizeEvent & evt) {
	wxSize wxs = evt.GetSize();
	//image.Rescale(wxs.GetWidth(), wxs.GetHeight(), wxIMAGE_QUALITY_HIGH);
	//image.Resize(wxs, wxPoint(0,0), 255, 255, 255);
	std::cout << "RESIZE " << wxs.GetWidth() << " " << wxs.GetHeight() << std::endl;
	m_fResize = true;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnIdle(wxIdleEvent & evt) {
	if (m_fResize) {
		std::cout << "FINAL RESIZE" << std::endl;
		m_fResize = false;

		// Generate coordinates
		SetCoordinateRange(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1]);

		// Sample the datafile
		m_pgds->Sample(m_dSampleLon, m_dSampleLat, m_imagemap);

		// Generate the image
		GenerateImageFromImageMap();

		// Paint
		PaintNow();
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseMotion(wxMouseEvent & evt) {
	wxPoint pos = evt.GetPosition();

	pos.x -= DISPLAY_BORDER;
	pos.y -= DISPLAY_BORDER;

	wxNcVisFrame * ncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	if ((pos.x < 0) || (pos.x >= m_dSampleLon.GetRows())) {
		ncvisparent->SetStatusMessage(_T(""), true);
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleLat.GetRows())) {
		ncvisparent->SetStatusMessage(_T(""), true);
		return;
	}

	double dX = m_dSampleLon[pos.x];
	double dY = m_dSampleLat[m_dSampleLat.GetRows() - pos.y - 1];

	size_t sI = static_cast<size_t>((m_dSampleLat.GetRows() - pos.y - 1) * m_dSampleLon.GetRows() + pos.x);

	if (m_imagemap.GetRows() <= sI) {
		return;
	}
	if (m_pdata == NULL) {
		return;
	}

	char szMessage[64];
	snprintf(szMessage, 64, " (X: %f Y: %f I: %i) %f", dX, dY, m_imagemap[sI], (*m_pdata)[m_imagemap[sI]]);

	if (ncvisparent != NULL) {
		ncvisparent->SetStatusMessage(szMessage, true);
	}

}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseLeaveWindow(wxMouseEvent & evt) {
	wxNcVisFrame * ncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	if (ncvisparent != NULL) {
		ncvisparent->SetStatusMessage(_T(""), true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseLeftDoubleClick(wxMouseEvent & evt) {
	std::cout << "DOUBLE CLICK" << std::endl;

	wxPoint pos = evt.GetPosition();

	pos.x -= DISPLAY_BORDER;
	pos.y -= DISPLAY_BORDER;

	if ((pos.x < 0) || (pos.x >= m_dSampleLon.GetRows())) {
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleLat.GetRows())) {
		return;
	}

	double dX = m_dSampleLon[pos.x];
	double dY = m_dSampleLat[m_dSampleLat.GetRows() - pos.y - 1];

	double dXdelta = m_dXrange[1] - m_dXrange[0];
	double dYdelta = m_dYrange[1] - m_dYrange[0];

	m_dXrange[0] = dX - 0.25 * dXdelta;
	m_dXrange[1] = dX + 0.25 * dXdelta;

	m_dYrange[0] = dY - 0.25 * dYdelta;
	m_dYrange[1] = dY + 0.25 * dYdelta;

	if (m_dYrange[0] < -90.0) {
		double dShiftY = -90.0 - m_dYrange[0];
		m_dYrange[0] += dShiftY;
		m_dYrange[1] += dShiftY;
	}
	if (m_dYrange[1] > 90.0) {
		double dShiftY = m_dYrange[1] - 90.0;
		m_dYrange[0] -= dShiftY;
		m_dYrange[1] -= dShiftY;
	}
	if (m_dYrange[0] < -90.0) {
		m_dYrange[0] = -90.0;
	}

	SetCoordinateRange(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1], true);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::FormatLabelBarLabel(
	double dValue,
	std::string & str
) {
	char sz[16];
	if (dValue == 0.0) {
		str = "0";
		return;
	}
	if (fabs(dValue) >= 1000.0) {
		snprintf(sz, 16, "%1.3e", dValue);
	} else if (fabs(dValue) > 100.0) {
		snprintf(sz, 16, "%3.3f", dValue);
	} else if (fabs(dValue) > 10.0) {
		snprintf(sz, 16, "%2.4f", dValue);
	} else if (fabs(dValue) > 1.0e-2) {
		snprintf(sz, 16, "%1.5f", dValue);
	} else {
		snprintf(sz, 16, "%1.3e", dValue);
	}
	str = sz;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::GenerateImageFromImageMap(
	bool fRedraw
) {

	wxSize wxs = this->GetSize();

	size_t sWidth = wxs.GetWidth();
	size_t sHeight = wxs.GetHeight();

	size_t sImageWidth = sWidth - LABELBAR_IMAGEWIDTH - 2 * DISPLAY_BORDER;
	size_t sImageHeight = sHeight - 2 * DISPLAY_BORDER;

	m_image.Resize(wxs, wxPoint(0,0), 0, 0, 0);

	unsigned char * imagedata = m_image.GetData();

	// Draw border
	for (size_t j = 0; j < sHeight; j++) {
		imagedata[3 * sWidth * j + 0] = 0;
		imagedata[3 * sWidth * j + 1] = 0;
		imagedata[3 * sWidth * j + 2] = 0;

		imagedata[3 * (sWidth * (j+1) - 1) + 0] = 0;
		imagedata[3 * (sWidth * (j+1) - 1) + 1] = 0;
		imagedata[3 * (sWidth * (j+1) - 1) + 2] = 0;
	}
	for (size_t i = 0; i < sWidth; i++) {
		imagedata[3 * i + 0] = 0;
		imagedata[3 * i + 1] = 0;
		imagedata[3 * i + 2] = 0;

		imagedata[3 * (sWidth * (sHeight-1) + i) + 0] = 0;
		imagedata[3 * (sWidth * (sHeight-1) + i) + 1] = 0;
		imagedata[3 * (sWidth * (sHeight-1) + i) + 2] = 0;
	}

	// Draw map
	int s = 0;
	for (size_t j = 0; j < sImageHeight; j++) {
		size_t jx = sHeight - (j + DISPLAY_BORDER) - 1;
		for (size_t i = 0; i < sImageWidth; i++) {
			size_t ix = i + DISPLAY_BORDER;

			m_colormap.Sample(
				(*m_pdata)[m_imagemap[s]],
				m_dDataRange[0],
				m_dDataRange[1],
				imagedata[3 * sWidth * jx + 3 * ix + 0],
				imagedata[3 * sWidth * jx + 3 * ix + 1],
				imagedata[3 * sWidth * jx + 3 * ix + 2]);

			s++;
		}
	}

	// Create label bar
	if (LABELBAR_IMAGEWIDTH != 0) {
		size_t sLabelBarXStart = sImageWidth + DISPLAY_BORDER;
		size_t sLabelBarYStart = DISPLAY_BORDER;

		size_t sLabelBarWidth = LABELBAR_IMAGEWIDTH;
		size_t sLabelBarHeight = sImageHeight;

		size_t sLabelBarXEnd = sLabelBarXStart + sLabelBarWidth;
		size_t sLabelBarYEnd = sLabelBarYStart + sLabelBarHeight;

		size_t sLabelBarBoxHeight = sLabelBarHeight / (LABELBAR_BOXCOUNT + 2);
		size_t sLabelBarBoxHalfHeight = sLabelBarBoxHeight / 2;
		size_t sLabelBarBoxYBegin = sLabelBarBoxHeight;
		size_t sLabelBarBoxWidth = sLabelBarWidth / 8;
		size_t sLabelBarBoxHalfWidth = sLabelBarBoxWidth / 2;

		for (size_t j = 0; j < sLabelBarHeight; j++) {
			size_t jx = sLabelBarYStart + j;
			for (size_t i = 0; i < sLabelBarWidth; i++) {
				size_t ix = sLabelBarXStart + i;

				imagedata[3 * sWidth * jx + 3 * ix + 0] = 255;
				imagedata[3 * sWidth * jx + 3 * ix + 1] = 255;
				imagedata[3 * sWidth * jx + 3 * ix + 2] = 255;
			}
		}

		for (size_t sBox = 0; sBox < LABELBAR_BOXCOUNT; sBox++) {
			unsigned char cR, cG, cB;

			double dColorValue = m_dDataRange[0]
				+ (m_dDataRange[1] - m_dDataRange[0])
					* (static_cast<double>(sBox) + 0.5)
					/ static_cast<double>(LABELBAR_BOXCOUNT);

			m_colormap.Sample(
				dColorValue,
				m_dDataRange[0],
				m_dDataRange[1],
				cR, cG, cB);

			//std::cout << "BOX: " << sHeight - (sLabelBarBoxYBegin + sBox * sLabelBarBoxHeight) - 1 << std::endl;
			//std::cout << "BOX2: " << sLabelBarYEnd - (sBox * sLabelBarBoxHeight) - 1 << std::endl;

			for (size_t j = 0; j < sLabelBarBoxHeight; j++) {
				size_t jx = sLabelBarYEnd - ((sBox+1) * sLabelBarBoxHeight + j) - 1;
				for (size_t i = 0; i < sLabelBarBoxWidth; i++) {
					size_t ix = sLabelBarXStart + sLabelBarBoxWidth + i;
					imagedata[3 * sWidth * jx + 3 * ix + 0] = cR;
					imagedata[3 * sWidth * jx + 3 * ix + 1] = cG;
					imagedata[3 * sWidth * jx + 3 * ix + 2] = cB;
				}
			}
		}

		std::string strLabel;
		for (size_t sBox = 0; sBox <= LABELBAR_BOXCOUNT; sBox++) {
			double dDataValue = m_dDataRange[0]
				+ (m_dDataRange[1] - m_dDataRange[0])
					* static_cast<double>(sBox)
					/ static_cast<double>(LABELBAR_BOXCOUNT);

			FormatLabelBarLabel(dDataValue, strLabel);

			DrawString(
				strLabel,
				sLabelBarXStart + 2 * sLabelBarBoxWidth + sLabelBarBoxHalfWidth, 
				sLabelBarYEnd - ((sBox+1) * sLabelBarBoxHeight - LABELBAR_FONTHEIGHT/2 + 1) - 1); 

			//std::cout << "LABEL: " << sHeight - (sLabelBarBoxYBegin + sBox * sLabelBarBoxHeight + LABELBAR_FONTHEIGHT) - 1 << std::endl;
		}

	}

	if (fRedraw) {
		PaintNow();
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetColorMap(
	const std::string & strColorMap,
	bool fRedraw
) {
	ColorMapLibrary::GenerateColorMap(strColorMap, m_colormap);

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetCoordinateRange(
	double dX0,
	double dX1,
	double dY0,
	double dY1,
	bool fRedraw
) {
	wxSize wxs = this->GetSize();

	size_t sWidth = wxs.GetWidth();
	size_t sHeight = wxs.GetHeight();

	_ASSERT(sWidth >= LABELBAR_IMAGEWIDTH + 2 * DISPLAY_BORDER);
	_ASSERT(sHeight >= 2 * DISPLAY_BORDER);

	size_t sImageWidth = sWidth - LABELBAR_IMAGEWIDTH - 2 * DISPLAY_BORDER;
	size_t sImageHeight = sHeight - 2 * DISPLAY_BORDER;

	m_dXrange[0] = dX0;
	m_dXrange[1] = dX1;
	m_dYrange[0] = dY0;
	m_dYrange[1] = dY1;

	m_dSampleLon.Allocate(sImageWidth);
	for (size_t i = 0; i < sImageWidth; i++) {
		m_dSampleLon[i] = m_dXrange[0] + (m_dXrange[1] - m_dXrange[0]) * (static_cast<double>(i) + 0.5) / static_cast<double>(sImageWidth);
		m_dSampleLon[i] = LonDegToStandardRange(m_dSampleLon[i]);
	}

	m_dSampleLat.Allocate(sImageHeight);
	for (size_t j = 0; j < sImageHeight; j++) {
		m_dSampleLat[j] = m_dYrange[0] + (m_dYrange[1] - m_dYrange[0]) * (static_cast<double>(j) + 0.5) / static_cast<double>(sImageHeight);
	}

	wxNcVisFrame * ncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	if (ncvisparent != NULL) {
		ncvisparent->SetDisplayedBounds(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1]);
	}

	if (fRedraw) {
		m_pgds->Sample(m_dSampleLon, m_dSampleLat, m_imagemap);

		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetDataRange(
	float dDataMin,
	float dDataMax,
	bool fRedraw
) {
	_ASSERT(dDataMin <= dDataMax);

	m_dDataRange[0] = dDataMin;
	m_dDataRange[1] = dDataMax;

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::DrawCharacter(
	unsigned char c,
	size_t x,
	size_t y,
	int * pwidth,
	int * pheight
) {
	//uint32_t utf32 = utf8_to_utf32(c);

	SFT_Glyph gid;
	if (sft_lookup(&m_sft, c, &gid) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " missing" << std::endl;
		return;
	}

	SFT_GMetrics mtx;
	if (sft_gmetrics(&m_sft, gid, &mtx) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " bad glyph metrics" << std::endl;
		return;
	}

	SFT_Image img;
	img.width = (mtx.minWidth + 3) & ~3;
	img.height = mtx.minHeight;
	char * pixels = new char[img.width * img.height];
	img.pixels = pixels;
	if (sft_render(&m_sft, gid, img) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " not rendered" << std::endl;
		return;
	}

	if (pwidth != NULL) {
		(*pwidth) = mtx.minWidth;
	}
	if (pheight != NULL) {
		(*pheight) = mtx.minHeight;
	}

	wxSize wxs = this->GetSize();
	size_t sWidth = wxs.GetWidth();
	size_t sHeight = wxs.GetHeight();

	unsigned char * imagedata = m_image.GetData();

	int s = 0;
	for (int j = 0; j < img.height; j++) {
		size_t jx = static_cast<size_t>(j) + y + mtx.yOffset;
		for (int i = 0; i < img.width; i++) {
			size_t ix = static_cast<size_t>(i) + x;
			imagedata[3 * sWidth * jx + 3 * ix + 0] = 255 - pixels[s];
			imagedata[3 * sWidth * jx + 3 * ix + 1] = 255 - pixels[s];
			imagedata[3 * sWidth * jx + 3 * ix + 2] = 255 - pixels[s];
			s++;
		}
	}

	delete[] pixels;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::DrawString(
	const std::string & str,
	size_t x,
	size_t y,
	int * pwidth,
	int * pheight
) {
	int cumulative_height = 0;
	int width = 0;
	int height = 0;
	for (int i = 0; i < str.length(); i++) {
		DrawCharacter(str[i], x, y, &width, &height);
		x += width;
		if ((pheight != NULL) && (height > cumulative_height)) {
			cumulative_height = height;
		}
	}

	if (pwidth != NULL) {
		(*pwidth) = static_cast<int>(x);
	}
	if (pheight != NULL) {
		(*pheight) = cumulative_height;
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::PaintNow() {
	// depending on your system you may need to look at double-buffered dcs
	wxClientDC dc(this);
	Render(dc);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::Render(wxDC & dc) {
	dc.DrawBitmap( m_image, 0, 0, false );
}

////////////////////////////////////////////////////////////////////////////////

