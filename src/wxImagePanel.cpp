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
	wxNcVisFrame * parent
) :
	wxPanel(parent),
	m_fGridLinesOn(false),
	m_fResize(false)
{
	m_pncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	_ASSERT(m_pncvisparent != NULL);

	int nPanelWidth = DISPLAY_WIDTH_DEFAULT + LABELBAR_IMAGEWIDTH + 2*DISPLAY_BORDER;
	int nPanelHeight = DISPLAY_HEIGHT_DEFAULT + 2*DISPLAY_BORDER;

	m_image.Create(nPanelWidth, nPanelHeight);

	m_sft.xScale = LABELBAR_FONTHEIGHT;
	m_sft.yScale = LABELBAR_FONTHEIGHT;
	m_sft.xOffset = 0;
	m_sft.yOffset = 0;
	m_sft.flags = SFT_DOWNWARD_Y;

	std::string strFontPath = m_pncvisparent->GetResourceDir() + "/Ubuntu-Regular.ttf";

	m_sft.font = sft_loadfile(strFontPath.c_str());
	if (m_sft.font == NULL) {
		std::cout << "ERROR loading \"" << strFontPath.c_str() <<"\"" << std::endl;
		_EXCEPTION();
	}

	SetSize(wxSize(nPanelWidth, nPanelHeight));
	SetMinSize(wxSize(nPanelWidth, nPanelHeight));
	SetCoordinateRange(0.0, 1.0, 0.0, 1.0);

	m_dDataRange[0] = 0.0;
	m_dDataRange[1] = 1.0;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnPaint(wxPaintEvent & evt) {
	std::cout << "PAINT" << std::endl;
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
		SetCoordinateRange(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1], true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseMotion(wxMouseEvent & evt) {
	wxPoint pos = evt.GetPosition();

	pos.x -= DISPLAY_BORDER;
	pos.y -= DISPLAY_BORDER;

	if ((pos.x < 0) || (pos.x >= m_dSampleX.GetRows())) {
		m_pncvisparent->SetStatusMessage(_T(""), true);
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleY.GetRows())) {
		m_pncvisparent->SetStatusMessage(_T(""), true);
		return;
	}

	double dX = m_dSampleX[pos.x];
	double dY = m_dSampleY[m_dSampleY.GetRows() - pos.y - 1];

	size_t sI = static_cast<size_t>((m_dSampleY.GetRows() - pos.y - 1) * m_dSampleX.GetRows() + pos.x);

	if (m_imagemap.GetRows() <= sI) {
		return;
	}

	const DataArray1D<float> & data = m_pncvisparent->GetData();
	if (m_imagemap[sI] >= data.GetRows()) {
		return;
	}

	char szMessage[64];
	snprintf(szMessage, 64, " (X: %f Y: %f I: %i) %f", dX, dY, m_imagemap[sI], data[m_imagemap[sI]]);

	m_pncvisparent->SetStatusMessage(szMessage, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseLeaveWindow(wxMouseEvent & evt) {
	m_pncvisparent->SetStatusMessage(_T(""), true);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseLeftDoubleClick(wxMouseEvent & evt) {
	std::cout << "DOUBLE CLICK" << std::endl;

	wxPoint pos = evt.GetPosition();

	pos.x -= DISPLAY_BORDER;
	pos.y -= DISPLAY_BORDER;

	if ((pos.x < 0) || (pos.x >= m_dSampleX.GetRows())) {
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleY.GetRows())) {
		return;
	}

	double dX = m_dSampleX[pos.x];
	double dY = m_dSampleY[m_dSampleY.GetRows() - pos.y - 1];

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
	} else if ((fabs(dValue) >= 1.0e6) || (fabs(dValue) < 1.0e-3)) {
		snprintf(sz, 16, "%.3g", dValue);
	} else if (fabs(dValue) < 0.01) {
		snprintf(sz, 16, "%.4g", dValue);
	} else if (fabs(dValue) < 0.1) {
		snprintf(sz, 16, "%.5g", dValue);
	} else if (fabs(dValue) < 1.0) {
		snprintf(sz, 16, "%.6g", dValue);
	} else {
		snprintf(sz, 16, "%.7g", dValue);
	}

	/*
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
	*/
	str = sz;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::RealCoordToImageCoord(
	double dX,
	double dY,
	size_t sImageWidth,
	size_t sImageHeight,
	int & iXcoord,
	int & iYcoord
) {
	dX = (LonDegToStandardRange(dX) - m_dXrange[0]) / (m_dXrange[1] - m_dXrange[0]);
	dY = (dY - m_dYrange[0]) / (m_dYrange[1] - m_dYrange[0]);

	iXcoord = static_cast<int>(static_cast<double>(sImageWidth) * dX);
	iYcoord = static_cast<int>(static_cast<double>(sImageHeight) * dY);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::GenerateImageFromImageMap(
	bool fRedraw
) {
	std::cout << "GENERATE IMAGE" << std::endl;

	wxSize wxs = this->GetSize();

	size_t sWidth = wxs.GetWidth();
	size_t sHeight = wxs.GetHeight();

	size_t sImageWidth = sWidth - LABELBAR_IMAGEWIDTH - 2 * DISPLAY_BORDER;
	size_t sImageHeight = sHeight - 2 * DISPLAY_BORDER;

	m_image.Resize(wxs, wxPoint(0,0), 0, 0, 0);

	unsigned char * imagedata = m_image.GetData();

	const DataArray1D<float> & data = m_pncvisparent->GetData();

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
				data[m_imagemap[s]],
				m_dDataRange[0],
				m_dDataRange[1],
				imagedata[3 * sWidth * jx + 3 * ix + 0],
				imagedata[3 * sWidth * jx + 3 * ix + 1],
				imagedata[3 * sWidth * jx + 3 * ix + 2]);

			s++;
		}
	}

	// Draw grid lines
	if (m_fGridLinesOn) {
		static const int nGridThickness = 2;

		double dMajorDeltaX = m_dXrange[1] - m_dXrange[0];
		if (dMajorDeltaX >= 90.0) {
			dMajorDeltaX = 30.0;
		} else {
			_ASSERT(dMajorDeltaX > 0.0);
			int iDeltaXMag10 = static_cast<int>(std::log10(dMajorDeltaX));
			dMajorDeltaX = pow(10.0, static_cast<double>(iDeltaXMag10));
		}

		for (int i = nGridThickness; i < sImageWidth-nGridThickness; i++) {
			if (std::floor(m_dSampleX[i] / dMajorDeltaX) !=
			    std::floor(m_dSampleX[i+nGridThickness] / dMajorDeltaX)
			) {
				size_t ix = i + DISPLAY_BORDER;
				for (int j = 0; j < sImageHeight; j+=2) {
					size_t jx = sHeight - (j + DISPLAY_BORDER) - 1;

					imagedata[3 * sWidth * jx + 3 * ix + 0] = 255;
					imagedata[3 * sWidth * jx + 3 * ix + 1] = 255;
					imagedata[3 * sWidth * jx + 3 * ix + 2] = 255;
				}
			}
		}
		for (int j = nGridThickness; j < sImageHeight-nGridThickness; j++) {
			if (std::floor(m_dSampleY[j] / dMajorDeltaX) !=
			    std::floor(m_dSampleY[j+nGridThickness] / dMajorDeltaX)
			) {
				size_t jx = sHeight - (j + DISPLAY_BORDER) - 1;
				for (int i = 0; i < sImageWidth; i+=2) {
					size_t ix = i + DISPLAY_BORDER;

					imagedata[3 * sWidth * jx + 3 * ix + 0] = 255;
					imagedata[3 * sWidth * jx + 3 * ix + 1] = 255;
					imagedata[3 * sWidth * jx + 3 * ix + 2] = 255;
				}
			}
		}
	}

	// Draw overlay
	if (m_overlaydata.faces.size() != 0) {
		for (size_t f = 0; f < m_overlaydata.faces.size(); f++) {
			const std::vector<int> & face = m_overlaydata.faces[f];
			if (m_overlaydata.faces[f].size() == 0) {
				continue;
			}

			int iXprev = 0;
			int iYprev = 0;

			int iXnext = 0;
			int iYnext = 0;

			RealCoordToImageCoord(
				m_overlaydata.coords[face[0]].first,
				m_overlaydata.coords[face[0]].second,
				sImageWidth,
				sImageHeight,
				iXnext,
				iYnext);

			for (size_t v = 1; v < m_overlaydata.faces[f].size(); v++) {
				iXprev = iXnext;
				iYprev = iYnext;

				RealCoordToImageCoord(
					m_overlaydata.coords[face[v]].first,
					m_overlaydata.coords[face[v]].second,
					sImageWidth,
					sImageHeight,
					iXnext,
					iYnext);

				int nDistX = abs(iXnext - iXprev);
				int nDistY = abs(iYnext - iYprev);

				int nDistMax = nDistX;
				if (nDistY > nDistX) {
					nDistMax = nDistY;
				}
				if (nDistMax > 0.8 * sImageWidth) {
					continue;
				}

				// TODO: Handle polyline and polygon geometries separately
				double dXstep = static_cast<double>(iXnext - iXprev) / static_cast<double>(nDistMax);
				double dYstep = static_cast<double>(iYnext - iYprev) / static_cast<double>(nDistMax);

				for (int i = 0; i < nDistMax; i+=1) {
					int iXcoord = iXprev + static_cast<int>(dXstep * i);
					int iYcoord = iYprev + static_cast<int>(dYstep * i);

					if ((iXcoord >= 0) && (iXcoord < sImageWidth) && (iYcoord >= 0) && (iYcoord < sImageHeight)) {
						size_t ix = iXcoord + DISPLAY_BORDER;
						size_t jx = sHeight - (iYcoord + DISPLAY_BORDER) - 1;

						imagedata[3 * sWidth * jx + 3 * ix + 0] = 255;
						imagedata[3 * sWidth * jx + 3 * ix + 1] = 255;
						imagedata[3 * sWidth * jx + 3 * ix + 2] = 255;

						jx++;
						imagedata[3 * sWidth * jx + 3 * ix + 0] = 255;
						imagedata[3 * sWidth * jx + 3 * ix + 1] = 255;
						imagedata[3 * sWidth * jx + 3 * ix + 2] = 255;

						//ix++;
						//imagedata[3 * sWidth * jx + 3 * ix + 0] = imagedata[3 * sWidth * jx + 3 * ix + 0] / 2;
						//imagedata[3 * sWidth * jx + 3 * ix + 1] = imagedata[3 * sWidth * jx + 3 * ix + 1] / 2;
						//imagedata[3 * sWidth * jx + 3 * ix + 2] = imagedata[3 * sWidth * jx + 3 * ix + 2] / 2;
					}
				}
			}
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
	m_pncvisparent->GetColorMapLibrary().GenerateColorMap(strColorMap, m_colormap);

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

	m_dSampleX.Allocate(sImageWidth);
	for (size_t i = 0; i < sImageWidth; i++) {
		m_dSampleX[i] = m_dXrange[0] + (m_dXrange[1] - m_dXrange[0]) * (static_cast<double>(i) + 0.5) / static_cast<double>(sImageWidth);
	}

	m_dSampleY.Allocate(sImageHeight);
	for (size_t j = 0; j < sImageHeight; j++) {
		m_dSampleY[j] = m_dYrange[0] + (m_dYrange[1] - m_dYrange[0]) * (static_cast<double>(j) + 0.5) / static_cast<double>(sImageHeight);
	}

	m_pncvisparent->SetDisplayedBounds(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1]);

	m_imagemap.Allocate(m_dSampleX.GetRows() * m_dSampleY.GetRows());
	m_pncvisparent->SampleData(m_dSampleX, m_dSampleY, m_imagemap);

	if (fRedraw) {
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

	m_pncvisparent->SetDisplayedDataRange(dDataMin, dDataMax);

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetGridLinesOn(
	bool fGridLinesOn,
	bool fRedraw
) {
	if (m_fGridLinesOn == fGridLinesOn) {
		return;
	}

	m_fGridLinesOn = fGridLinesOn;

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

