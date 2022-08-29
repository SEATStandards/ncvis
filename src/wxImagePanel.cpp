///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxImagePanel.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxImagePanel.h"
#include "wxNcVisFrame.h"
#include "lodepng.h"
#include "NcVisPlotOptions.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/kbdstate.h>

#include <map>

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
static const int MAP_WIDTH_DEFAULT = 720;

///	<summary>
///		Default height of the image panel.
///	</summary>
static const int MAP_HEIGHT_DEFAULT = 360;

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
static const int LABELBAR_FONTHEIGHT = 16;

///	<summary>
///		Font height of the title.
///	</summary>
static const int TITLE_FONTHEIGHT = 18;

///	<summary>
///		Margin width of the title.
///	</summary>
static const int TITLE_MARGINHEIGHT = 6;

///	<summary>
///		Thickness of grid lines.
///	</summary>
static const int GRID_THICKNESS = 2;

///	<summary>
///		Margin width for tickmark labels.
///	</summary>
static const int TICKMARK_MARGINWIDTH = 80;

///	<summary>
///		Margin height for tickmark labels.
///	</summary>
static const int TICKMARK_MARGINHEIGHT = 32;

///	<summary>
///		Length of a major tickmark.
///	</summary>
static const int TICKMARK_MAJORLENGTH = 8;

///	<summary>
///		Tickmark label font height.
///	</summary>
static const int TICKMARKLABEL_TICKLABELSPACING = 4;

///	<summary>
///		Tickmark label font height.
///	</summary>
static const int TICKMARKLABEL_FONTHEIGHT = 16;

////////////////////////////////////////////////////////////////////////////////

wxImagePanel::wxImagePanel(
	wxNcVisFrame * parent
) :
	wxPanel(parent),
	m_dColorMapScalingFactor(1.0),
	m_fEnableRedraw(true),
	m_fGridLinesOn(false),
	m_fResize(false)
{
	m_pncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
	_ASSERT(m_pncvisparent != NULL);

	m_dXrange[0] = 0.0;
	m_dXrange[1] = 1.0;
	m_dYrange[0] = 0.0;
	m_dYrange[1] = 1.0;

	m_dDataRange[0] = 0.0;
	m_dDataRange[1] = 1.0;

	// Initialize the bitmap
	wxSize wxs = GetPanelSize();

	m_image.Create(wxs.GetWidth(), wxs.GetHeight());

	SetSize(wxSize(wxs.GetWidth(), wxs.GetHeight()));
	SetMinSize(wxSize(wxs.GetWidth(), wxs.GetHeight()));
	SetCoordinateRange(0.0, 1.0, 0.0, 1.0);

	// Initialize font information
	m_sftTitleBar.xScale = TITLE_FONTHEIGHT;
	m_sftTitleBar.yScale = TITLE_FONTHEIGHT;
	m_sftTitleBar.xOffset = 0;
	m_sftTitleBar.yOffset = 0;
	m_sftTitleBar.flags = SFT_DOWNWARD_Y;

	m_sftLabelBar.xScale = LABELBAR_FONTHEIGHT;
	m_sftLabelBar.yScale = LABELBAR_FONTHEIGHT;
	m_sftLabelBar.xOffset = 0;
	m_sftLabelBar.yOffset = 0;
	m_sftLabelBar.flags = SFT_DOWNWARD_Y;

	m_sftTickmarkLabels.xScale = TICKMARKLABEL_FONTHEIGHT;
	m_sftTickmarkLabels.yScale = TICKMARKLABEL_FONTHEIGHT;
	m_sftTickmarkLabels.xOffset = 0;
	m_sftTickmarkLabels.yOffset = 0;
	m_sftTickmarkLabels.flags = SFT_DOWNWARD_Y;

	wxFileName wxfnFontPath(m_pncvisparent->GetResourceDir(), _T("Ubuntu-Regular.ttf"));
	wxString wxstrFontPath = wxfnFontPath.GetFullPath();

	m_sftTitleBar.font = sft_loadfile(wxstrFontPath.ToStdString().c_str());
	if (m_sftTitleBar.font == NULL) {
		std::cout << "ERROR: Font \"" << wxstrFontPath <<"\" not found" << std::endl;
		exit(-1);
	}

	m_sftLabelBar.font = m_sftTitleBar.font;
	m_sftTickmarkLabels.font = m_sftTitleBar.font;
}

////////////////////////////////////////////////////////////////////////////////

wxSize wxImagePanel::GetPanelSize(
	int nMapWidth,
	int nMapHeight
) const {
	if (nMapWidth == (-1)) {
		nMapWidth = MAP_WIDTH_DEFAULT;
	}
	if (nMapHeight == (-1)) {
		nMapHeight = MAP_HEIGHT_DEFAULT;
	}

	wxSize wxs(
		nMapWidth + 2*DISPLAY_BORDER + LABELBAR_IMAGEWIDTH,
		nMapHeight + 2*DISPLAY_BORDER);

	if (m_pncvisparent->GetPlotOptions().m_fShowTitle) {
		wxs.SetHeight(wxs.GetHeight() + TITLE_FONTHEIGHT + TITLE_MARGINHEIGHT);
	}
	if (m_pncvisparent->GetPlotOptions().m_fShowTickmarkLabels) {
		wxs.SetHeight(wxs.GetHeight() + TICKMARK_MARGINHEIGHT);
		wxs.SetWidth(wxs.GetWidth() + TICKMARK_MARGINWIDTH);
	}

	return wxs;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::GetMapPositionSize(
	wxSize & wxs,
	wxPosition & wxp,
	int nImageWidth,
	int nImageHeight
) const {

	_ASSERT(m_pncvisparent != NULL);

	wxs = GetSize();

	if (nImageWidth != (-1)) {
		wxs.SetWidth(nImageWidth);
	} else {
		wxs.SetWidth(wxs.GetWidth() - 2*DISPLAY_BORDER);
	}
	if (nImageHeight != (-1)) {
		wxs.SetHeight(nImageHeight);
	} else {
		wxs.SetHeight(wxs.GetHeight() - 2*DISPLAY_BORDER);
	}

	wxs.SetHeight(wxs.GetHeight());
	wxs.SetWidth(wxs.GetWidth() - LABELBAR_IMAGEWIDTH);

	wxp.SetColumn(0);
	wxp.SetRow(0);

	if (m_pncvisparent->GetPlotOptions().m_fShowTitle) {
		wxs.SetHeight(wxs.GetHeight() - (TITLE_FONTHEIGHT + TITLE_MARGINHEIGHT));
		wxp.SetRow(wxp.GetRow() + (TITLE_FONTHEIGHT + TITLE_MARGINHEIGHT));
	}
	if (m_pncvisparent->GetPlotOptions().m_fShowTickmarkLabels) {
		wxs.SetHeight(wxs.GetHeight() - TICKMARK_MARGINHEIGHT);
		wxs.SetWidth(wxs.GetWidth() - TICKMARK_MARGINWIDTH);
		wxp.SetCol(wxp.GetCol() + TICKMARK_MARGINWIDTH);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool wxImagePanel::ResetPanelSize() {

	_ASSERT(m_pncvisparent != NULL);

	// New panel size
	wxSize wxsUpdated = GetPanelSize(m_dSampleX.size(), m_dSampleY.size());

	// Current panel size
	wxSize wxsCurrent = GetSize();

	if ((wxsUpdated.GetWidth() != wxsCurrent.GetWidth()) ||
	    (wxsUpdated.GetHeight() != wxsCurrent.GetHeight())
	) {
		SetSize(wxsUpdated);
		SetMinSize(wxsUpdated);

		SetCoordinateRange(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1], false);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnPaint(wxPaintEvent & evt) {
	_ASSERT(m_pncvisparent != NULL);
	if (m_pncvisparent->IsVerbose()) {
		std::cout << "PAINT" << std::endl;
	}
	wxPaintDC dc(this);
	Render(dc);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnSize(wxSizeEvent & evt) {
	wxSize wxs = evt.GetSize();

	_ASSERT(m_pncvisparent != NULL);
	if (m_pncvisparent->IsVerbose()) {
		std::cout << "RESIZE " << wxs.GetWidth() << " " << wxs.GetHeight() << std::endl;
	}
	m_fResize = true;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnIdle(wxIdleEvent & evt) {
	if (m_fResize) {
		_ASSERT(m_pncvisparent != NULL);
		if (m_pncvisparent->IsVerbose()) {
			std::cout << "FINAL RESIZE" << std::endl;
		}
		m_fResize = false;

		// Generate coordinates
		SetCoordinateRange(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1], true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::OnMouseMotion(wxMouseEvent & evt) {
	wxPoint posMouse = evt.GetPosition();

	wxSize wxsMap;
	wxPosition wxpMap;
	GetMapPositionSize(wxsMap, wxpMap);

	posMouse.x -= wxpMap.GetCol();
	posMouse.y -= wxpMap.GetRow();

	if ((posMouse.x < 0) || (posMouse.x >= m_dSampleX.size())) {
		m_pncvisparent->SetStatusMessage(_T(""), true);
		return;
	}
	if ((posMouse.y < 0) || (posMouse.y >= m_dSampleY.size())) {
		m_pncvisparent->SetStatusMessage(_T(""), true);
		return;
	}

	double dX = m_dSampleX[posMouse.x];
	double dY = m_dSampleY[m_dSampleY.size() - posMouse.y - 1];

	size_t sI = static_cast<size_t>((m_dSampleY.size() - posMouse.y - 1) * m_dSampleX.size() + posMouse.x);

	if (m_imagemap.size() <= sI) {
		return;
	}

	const std::vector<float> & data = m_pncvisparent->GetData();
	if (m_imagemap[sI] >= data.size()) {
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
	wxKeyboardState wxkeystate;

	wxPoint pos = evt.GetPosition();

	pos.x -= DISPLAY_BORDER;
	pos.y -= DISPLAY_BORDER;

	if ((pos.x < 0) || (pos.x >= m_dSampleX.size())) {
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleY.size())) {
		return;
	}

	double dX = m_dSampleX[pos.x];
	double dY = m_dSampleY[m_dSampleY.size() - pos.y - 1];

	double dXdelta = m_dXrange[1] - m_dXrange[0];
	double dYdelta = m_dYrange[1] - m_dYrange[0];

	double dXmin = m_pncvisparent->GetDisplayedDimensionMin(1);
	double dXmax = m_pncvisparent->GetDisplayedDimensionMax(1);

	double dYmin = m_pncvisparent->GetDisplayedDimensionMin(0);
	double dYmax = m_pncvisparent->GetDisplayedDimensionMax(0);

	bool fXperiodic = m_pncvisparent->IsDisplayedDimensionPeriodic(1);
	bool fYperiodic = m_pncvisparent->IsDisplayedDimensionPeriodic(0);

	// Zoom out
	if (evt.ShiftDown() || wxkeystate.ShiftDown()) {
		_ASSERT(m_pncvisparent != NULL);
		if (m_pncvisparent->IsVerbose()) {
			std::cout << "DOUBLE CLICK + SHIFT" << std::endl;
		}

		m_dXrange[0] = dX - dXdelta;
		m_dXrange[1] = dX + dXdelta;

		m_dYrange[0] = dY - dYdelta;
		m_dYrange[1] = dY + dYdelta;

	// Zoom in
	} else {
		_ASSERT(m_pncvisparent != NULL);
		if (m_pncvisparent->IsVerbose()) {
			std::cout << "DOUBLE CLICK" << std::endl;
		}

		m_dXrange[0] = dX - 0.25 * dXdelta;
		m_dXrange[1] = dX + 0.25 * dXdelta;

		m_dYrange[0] = dY - 0.25 * dYdelta;
		m_dYrange[1] = dY + 0.25 * dYdelta;
	}

	// Impose bounds
	if (!fXperiodic) {
		if (m_dXrange[0] < dXmin) {
			double dShiftX = dXmin - m_dXrange[0];
			m_dXrange[0] += dShiftX;
			m_dXrange[1] += dShiftX;
		}
		if (m_dXrange[1] > dXmax) {
			double dShiftX = m_dXrange[1] - dXmax;
			m_dXrange[0] -= dShiftX;
			m_dXrange[1] -= dShiftX;
		}
		if (m_dXrange[0] < dXmin) {
			m_dXrange[0] = dXmin;
		}
	}
	if (!fYperiodic) {
		if (m_dYrange[0] < dYmin) {
			double dShiftY = dYmin - m_dYrange[0];
			m_dYrange[0] += dShiftY;
			m_dYrange[1] += dShiftY;
		}
		if (m_dYrange[1] > dYmax) {
			double dShiftY = m_dYrange[1] - dYmax;
			m_dYrange[0] -= dShiftY;
			m_dYrange[1] -= dShiftY;
		}
		if (m_dYrange[0] < dYmin) {
			m_dYrange[0] = dYmin;
		}
	}

	// Set coordinate range
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

	str = sz;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::FormatTickmarkLabel(
	double dValue,
	std::string & str
) {
	char sz[16];
	if (dValue == 0.0) {
		str = "0";
		return;
	} else if ((fabs(dValue) >= 1.0e5) || (fabs(dValue) < 1.0e-2)) {
		snprintf(sz, 16, "%.2g", dValue);
	} else if (fabs(dValue) < 0.1) {
		snprintf(sz, 16, "%.4g", dValue);
	} else if (fabs(dValue) < 1.0) {
		snprintf(sz, 16, "%.5g", dValue);
	} else {
		snprintf(sz, 16, "%.6g", dValue);
	}

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

template <int NDIM>
void wxImagePanel::GenerateImageDataFromImageMap(
	const size_t sImageOffsetX,
	const size_t sImageOffsetY,
	const size_t sImageWidth,
	const size_t sImageHeight,
	const size_t sPanelWidth,
	const size_t sPanelHeight,
	unsigned char * imagedata
) {
	_ASSERT(m_pncvisparent != NULL);

	const std::vector<float> & data = m_pncvisparent->GetData();

	// Clear background
	memset(imagedata, 255, NDIM * sPanelWidth * sPanelHeight * sizeof(unsigned char));

	// Plot options
	const NcVisPlotOptions & plotopts = m_pncvisparent->GetPlotOptions();

	// Map size and position
	wxSize wxsMap;
	wxPosition wxpMap;
	GetMapPositionSize(wxsMap, wxpMap, sImageWidth, sImageHeight);

	size_t sMapWidth = wxsMap.GetWidth();
	size_t sMapHeight = wxsMap.GetHeight();

	_ASSERT(m_imagemap.size() == sMapWidth * sMapHeight);

	size_t sMapOffsetX = wxpMap.GetCol() + sImageOffsetX;
	size_t sMapOffsetY = wxpMap.GetRow() + sImageOffsetY;

	// Draw map, using different loops 
	if (!m_pncvisparent->DataHasMissingValue()) {
		if (m_dColorMapScalingFactor == 1.0) {
			for (size_t j = 0, s = 0; j < sMapHeight; j++) {
				size_t jx = sMapOffsetY + sMapHeight - j - 1;
				for (size_t i = 0; i < sMapWidth; i++, s++) {
					size_t ix = sMapOffsetX + i;

					m_colormap.Sample(
						data[m_imagemap[s]],
						m_dDataRange[0],
						m_dDataRange[1],
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0],
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1],
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2]);
				}
			}

		} else {
			for (size_t j = 0, s = 0; j < sMapHeight; j++) {
				size_t jx = sMapOffsetY + sMapHeight - j - 1;
				for (size_t i = 0; i < sMapWidth; i++, s++) {
					size_t ix = i + sMapOffsetX;

					m_colormap.SampleWithScaling(
						data[m_imagemap[s]],
						m_dDataRange[0],
						m_dDataRange[1],
						m_dColorMapScalingFactor,
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0],
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1],
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2]);
				}
			}
		}

	} else {
		float dMissingValue = m_pncvisparent->GetMissingValueFloat();

		if (m_dColorMapScalingFactor == 1.0) {
			for (size_t j = 0, s = 0; j < sMapHeight; j++) {
				size_t jx = sMapOffsetY + sMapHeight - j - 1;
				for (size_t i = 0; i < sMapWidth; i++, s++) {
					size_t ix = i + sMapOffsetX;

					float dValue = data[m_imagemap[s]];
					if (dValue != dMissingValue) {
						m_colormap.Sample(
							dValue,
							m_dDataRange[0],
							m_dDataRange[1],
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0],
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1],
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2]);
					} else {
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;
					}
				}
			}

		} else {
			for (size_t j = 0, s = 0; j < sMapHeight; j++) {
				size_t jx = sMapOffsetY + sMapHeight - j - 1;
				for (size_t i = 0; i < sMapWidth; i++, s++) {
					size_t ix = i + sMapOffsetX;

					float dValue = data[m_imagemap[s]];
					if (dValue != dMissingValue) {
						m_colormap.SampleWithScaling(
							dValue,
							m_dDataRange[0],
							m_dDataRange[1],
							m_dColorMapScalingFactor,
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0],
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1],
							imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2]);
					} else {
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;
					}
				}
			}
		}
	}

	// Draw grid lines
	if ((plotopts.m_fShowGrid) || (plotopts.m_fShowTickmarkLabels)) {

		// Generate tickmark positions
		double dMajorDeltaX = m_dXrange[1] - m_dXrange[0];
		if (dMajorDeltaX >= 90.0) {
			dMajorDeltaX = 30.0;
		} else {
			_ASSERT(dMajorDeltaX > 0.0);
			int iDeltaXMag10 = static_cast<int>(std::log10(dMajorDeltaX));
			dMajorDeltaX = pow(10.0, static_cast<double>(iDeltaXMag10));
		}

		std::map<int,double> mapTickmarkMajorX;
		std::map<int,double> mapTickmarkMajorY;

		for (int i = 0; i < m_dSampleX.size()-1; i++) {
			if (std::floor(m_dSampleX[i] / dMajorDeltaX) !=
			    std::floor(m_dSampleX[i+1] / dMajorDeltaX)
			) {
				mapTickmarkMajorX.insert(
					std::pair<int,double>(
						i,
						std::floor(m_dSampleX[i+1] / dMajorDeltaX) * dMajorDeltaX));
			}
		}

		mapTickmarkMajorX.insert(std::pair<int,double>(-1, m_dXrange[0]));
		mapTickmarkMajorX.insert(std::pair<int,double>(static_cast<int>(sMapWidth), m_dXrange[1]));
		mapTickmarkMajorY.insert(std::pair<int,double>(-1, m_dYrange[1]));
		mapTickmarkMajorY.insert(std::pair<int,double>(static_cast<int>(sMapHeight), m_dYrange[0]));

		for (int j = 0, s = 0; j < m_dSampleY.size()-1; j++) {
			if (std::floor(m_dSampleY[j] / dMajorDeltaX) !=
			    std::floor(m_dSampleY[j+1] / dMajorDeltaX)
			) {
				mapTickmarkMajorY.insert(
					std::pair<int,double>(
						static_cast<int>(sMapHeight) - j - 1,
						std::floor(m_dSampleY[j+1] / dMajorDeltaX) * dMajorDeltaX));
			}
		}

		// Draw the grid
		if (plotopts.m_fShowGrid) {
			for (int i = GRID_THICKNESS; i < sMapWidth-GRID_THICKNESS; i++) {
				if ((mapTickmarkMajorX.find(i) != mapTickmarkMajorX.end()) ||
				    (mapTickmarkMajorX.find(i+1) != mapTickmarkMajorX.end())
				) {
					size_t ix = i + sMapOffsetX;
					for (int j = 0; j < sMapHeight; j+=2) {
						size_t jx = sMapOffsetY + j;

						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;
					}
				}
			}
			for (int j = GRID_THICKNESS; j < sMapHeight-GRID_THICKNESS; j++) {
				if ((mapTickmarkMajorY.find(j) != mapTickmarkMajorY.end()) ||
				    (mapTickmarkMajorY.find(j+1) != mapTickmarkMajorY.end())
				) {
					size_t jx = sMapOffsetY + j;
					for (int i = 0; i < sMapWidth; i+=2) {
						size_t ix = i + sMapOffsetX;

						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;
					}
				}
			}
		}

		// Draw tickmarks and tickmark labels
		if (plotopts.m_fShowTickmarkLabels) {
			std::string strTickmarkLabel;

			int nStringWidth = 0;

			int lasti = -100;

			for (int i = -1; i < static_cast<int>(sMapWidth+1); i++) {
				auto itTickmarkMajorX = mapTickmarkMajorX.find(i);
				if (itTickmarkMajorX != mapTickmarkMajorX.end()) {
					size_t ix = i + sMapOffsetX;
					for (int j = 1; j < TICKMARK_MAJORLENGTH+1; j++) {
						size_t jx = sMapOffsetY + sMapHeight + j;

						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 64;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 64;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 64;
					}

					// Prevent text from overwriting itself (need a better alg)
					if (i != static_cast<int>(sMapWidth)) {
						if (i - lasti < nStringWidth) {
							continue;
						}
						if (i > static_cast<int>(sMapWidth) - nStringWidth) {
							continue;
						}
					}
					lasti = i;

					// Generate label
					FormatTickmarkLabel(itTickmarkMajorX->second, strTickmarkLabel);
					DrawString<NDIM>(
						m_sftTickmarkLabels,
						strTickmarkLabel,
						ix,
						sMapOffsetY + sMapHeight + TICKMARK_MAJORLENGTH + TICKMARKLABEL_FONTHEIGHT + TICKMARKLABEL_TICKLABELSPACING,
						TextAlignment_Center,
						sPanelWidth,
						sPanelHeight,
						imagedata,
						&nStringWidth);

				}
			}
			int lastj = -100;
			for (int j = -1; j < static_cast<int>(sMapHeight+1); j++) {
				auto itTickmarkMajorY = mapTickmarkMajorY.find(j);
				if (itTickmarkMajorY != mapTickmarkMajorY.end()) {
					size_t jx = sMapOffsetY + j;
					for (int i = -TICKMARK_MAJORLENGTH-1; i < -1; i++) {
						size_t ix = i + sMapOffsetX;

						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 64;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 64;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 64;
					}

					// Prevent text from overwriting itself
					if (j != static_cast<int>(sMapHeight)) {
						if (j - lastj < TICKMARKLABEL_FONTHEIGHT) {
							continue;
						}
						if (j > static_cast<int>(sMapHeight) - TICKMARKLABEL_FONTHEIGHT) {
							continue;
						}
					}
					lastj = j;

					// Generate label
					FormatTickmarkLabel(itTickmarkMajorY->second, strTickmarkLabel);
					DrawString<NDIM>(
						m_sftTickmarkLabels,
						strTickmarkLabel,
						sMapOffsetX - TICKMARK_MAJORLENGTH - TICKMARKLABEL_TICKLABELSPACING,
						sMapOffsetY + j + (TICKMARKLABEL_FONTHEIGHT / 2) - 2,
						TextAlignment_Right,
						sPanelWidth,
						sPanelHeight,
						imagedata); 
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
				sMapWidth,
				sMapHeight,
				iXnext,
				iYnext);

			for (size_t v = 1; v < m_overlaydata.faces[f].size(); v++) {
				iXprev = iXnext;
				iYprev = iYnext;

				RealCoordToImageCoord(
					m_overlaydata.coords[face[v]].first,
					m_overlaydata.coords[face[v]].second,
					sMapWidth,
					sMapHeight,
					iXnext,
					iYnext);

				int nDistX = abs(iXnext - iXprev);
				int nDistY = abs(iYnext - iYprev);

				int nDistMax = nDistX;
				if (nDistY > nDistX) {
					nDistMax = nDistY;
				}
				if (nDistMax > 0.8 * sMapWidth) {
					continue;
				}

				// TODO: Handle polyline and polygon geometries separately
				double dXstep = static_cast<double>(iXnext - iXprev) / static_cast<double>(nDistMax);
				double dYstep = static_cast<double>(iYnext - iYprev) / static_cast<double>(nDistMax);

				for (int i = 0; i < nDistMax; i+=1) {
					int iXcoord = iXprev + static_cast<int>(dXstep * i);
					int iYcoord = iYprev + static_cast<int>(dYstep * i);

					if ((iXcoord >= 0) && (iXcoord < sMapWidth) && (iYcoord >= 0) && (iYcoord < sMapHeight)) {
						size_t ix = iXcoord + sImageOffsetX;
						size_t jx = sPanelHeight - (iYcoord + sImageOffsetY) - 1;

						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;

						jx++;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 255;
						imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 255;

						//ix++;
						//imagedata[3 * sPanelWidth * jx + 3 * ix + 0] = imagedata[3 * sWidth * jx + 3 * ix + 0] / 2;
						//imagedata[3 * sPanelWidth * jx + 3 * ix + 1] = imagedata[3 * sWidth * jx + 3 * ix + 1] / 2;
						//imagedata[3 * sPanelWidth * jx + 3 * ix + 2] = imagedata[3 * sWidth * jx + 3 * ix + 2] / 2;
					}
				}
			}
		}
	}

	// Create label bar
	if (LABELBAR_IMAGEWIDTH != 0) {
		size_t sLabelBarXStart = sMapOffsetX + sMapWidth;
		size_t sLabelBarYStart = sMapOffsetY;

		size_t sLabelBarWidth = LABELBAR_IMAGEWIDTH;
		size_t sLabelBarHeight = sMapHeight;

		size_t sLabelBarXEnd = sLabelBarXStart + sLabelBarWidth;
		size_t sLabelBarYEnd = sLabelBarYStart + sLabelBarHeight;

		size_t sLabelBarBoxHeight = sLabelBarHeight / (LABELBAR_BOXCOUNT + 2);
		size_t sLabelBarBoxHalfHeight = sLabelBarBoxHeight / 2;
		size_t sLabelBarBoxYBegin = sLabelBarBoxHeight;
		size_t sLabelBarBoxWidth = sLabelBarWidth / 8;
		size_t sLabelBarBoxHalfWidth = sLabelBarBoxWidth / 2;

		for (int j = sLabelBarYStart; j < sLabelBarYEnd; j++) {
			size_t jx = j;
			size_t ix = sLabelBarXStart;
			imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = 0;
			imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = 0;
			imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = 0;
		}

		for (size_t sBox = 0; sBox < LABELBAR_BOXCOUNT; sBox++) {
			unsigned char cR, cG, cB;

			double dColorValue = m_dDataRange[0]
				+ (m_dDataRange[1] - m_dDataRange[0])
					* (static_cast<double>(sBox) + 0.5)
					/ static_cast<double>(LABELBAR_BOXCOUNT);

			m_colormap.SampleWithScaling(
				dColorValue,
				m_dDataRange[0],
				m_dDataRange[1],
				m_dColorMapScalingFactor,
				cR, cG, cB);

			for (size_t j = 0; j < sLabelBarBoxHeight; j++) {
				size_t jx = sLabelBarYEnd - ((sBox+1) * sLabelBarBoxHeight + j) - 1;
				for (int i = 0; i < sLabelBarBoxWidth; i++) {
					size_t ix = sLabelBarXStart + sLabelBarBoxWidth + i;
					imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 0] = cR;
					imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 1] = cG;
					imagedata[NDIM * sPanelWidth * jx + NDIM * ix + 2] = cB;
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

			DrawString<NDIM>(
				m_sftLabelBar,
				strLabel,
				sLabelBarXStart + 2 * sLabelBarBoxWidth + sLabelBarBoxHalfWidth, 
				sLabelBarYEnd - ((sBox+1) * sLabelBarBoxHeight - LABELBAR_FONTHEIGHT/2 + 1) - 1,
				TextAlignment_Left,
				sPanelWidth,
				sPanelHeight,
				imagedata); 
		}
	}

	// Create title
	if (m_pncvisparent->GetPlotOptions().m_fShowTitle) {
		DrawString<NDIM>(
			m_sftTitleBar,
			m_pncvisparent->GetVarActiveTitle(),
			sMapOffsetX + TITLE_MARGINHEIGHT,
			sImageOffsetY + TITLE_FONTHEIGHT - 1,
			TextAlignment_Left,
			sPanelWidth,
			sPanelHeight,
			imagedata); 

		DrawString<NDIM>(
			m_sftTitleBar,
			m_pncvisparent->GetVarActiveUnits(),
			sMapOffsetX + sMapWidth,
			sImageOffsetY + TITLE_FONTHEIGHT - 1,
			TextAlignment_Right,
			sPanelWidth,
			sPanelHeight,
			imagedata); 
	}

	// Draw border around image
	if (plotopts.m_fShowTickmarkLabels) {

		{
			size_t jx0 = sMapOffsetY - 1;
			size_t jx1 = sMapOffsetY + sMapHeight;
			for (int i = 0; i < sMapWidth+2; i++) {
				size_t ix = sMapOffsetX + i - 1;

				imagedata[NDIM * sPanelWidth * jx0 + NDIM * ix + 0] = 0;
				imagedata[NDIM * sPanelWidth * jx0 + NDIM * ix + 1] = 0;
				imagedata[NDIM * sPanelWidth * jx0 + NDIM * ix + 2] = 0;

				imagedata[NDIM * sPanelWidth * jx1 + NDIM * ix + 0] = 0;
				imagedata[NDIM * sPanelWidth * jx1 + NDIM * ix + 1] = 0;
				imagedata[NDIM * sPanelWidth * jx1 + NDIM * ix + 2] = 0;
			}
		}

		{
			size_t ix0 = sMapOffsetX - 1;
			size_t ix1 = sMapOffsetX + sMapWidth;
			for (int j = 0; j < sMapHeight; j++) {
				size_t jx = sMapOffsetY + j;

				imagedata[NDIM * sPanelWidth * jx + NDIM * ix0 + 0] = 0;
				imagedata[NDIM * sPanelWidth * jx + NDIM * ix0 + 1] = 0;
				imagedata[NDIM * sPanelWidth * jx + NDIM * ix0 + 2] = 0;

				imagedata[NDIM * sPanelWidth * jx + NDIM * ix1 + 0] = 0;
				imagedata[NDIM * sPanelWidth * jx + NDIM * ix1 + 1] = 0;
				imagedata[NDIM * sPanelWidth * jx + NDIM * ix1 + 2] = 0;
			}
		}

	}


	//imagedata[NDIM * sPanelWidth * 1 + NDIM * 1 + 0] = 255;
	//imagedata[NDIM * sPanelWidth * 1 + NDIM * 1 + 1] = 0;
	//imagedata[NDIM * sPanelWidth * 1 + NDIM * 1 + 2] = 0;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::GenerateImageFromImageMap(
	bool fRedraw
) {
	_ASSERT(m_pncvisparent != NULL);

	if (!m_fEnableRedraw) {
		if (m_pncvisparent->IsVerbose()) {
			std::cout << "NOREDRAW" << std::endl;
		}
		return;
	}

	if (m_pncvisparent->IsVerbose()) {
		std::cout << "GENERATE IMAGE" << std::endl;
	}

	wxSize wxs = GetSize();

	size_t sPanelWidth = wxs.GetWidth();
	size_t sPanelHeight = wxs.GetHeight();

	m_image.Resize(wxs, wxPoint(0,0), 0, 0, 0);

	unsigned char * imagedata = m_image.GetData();

	// Generate image
	GenerateImageDataFromImageMap<3>(
		DISPLAY_BORDER,
		DISPLAY_BORDER,
		sPanelWidth - 2 * DISPLAY_BORDER,
		sPanelHeight - 2 * DISPLAY_BORDER,
		sPanelWidth,
		sPanelHeight,
		imagedata);

	// Draw border
	for (size_t j = 0; j < sPanelHeight; j++) {
		imagedata[3 * sPanelWidth * j + 0] = 0;
		imagedata[3 * sPanelWidth * j + 1] = 0;
		imagedata[3 * sPanelWidth * j + 2] = 0;

		imagedata[3 * (sPanelWidth * (j+1) - 1) + 0] = 0;
		imagedata[3 * (sPanelWidth * (j+1) - 1) + 1] = 0;
		imagedata[3 * (sPanelWidth * (j+1) - 1) + 2] = 0;
	}
	for (size_t i = 0; i < sPanelWidth; i++) {
		imagedata[3 * i + 0] = 0;
		imagedata[3 * i + 1] = 0;
		imagedata[3 * i + 2] = 0;

		imagedata[3 * (sPanelWidth * (sPanelHeight-1) + i) + 0] = 0;
		imagedata[3 * (sPanelWidth * (sPanelHeight-1) + i) + 1] = 0;
		imagedata[3 * (sPanelWidth * (sPanelHeight-1) + i) + 2] = 0;
	}

	// Redraw
	if (fRedraw) {
		PaintNow();
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetColorMap(
	const wxString & strColorMap,
	bool fRedraw
) {
	m_pncvisparent->GetColorMapLibrary().GenerateColorMap(strColorMap, m_colormap);

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetColorMapScalingFactor(
	float dColorMapScalingFactor,
	bool fRedraw
) {
	_ASSERT(dColorMapScalingFactor > 0.0);

	m_dColorMapScalingFactor = dColorMapScalingFactor;

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::ResampleData(
	bool fRedraw
) {
	m_imagemap.resize(m_dSampleX.size() * m_dSampleY.size());

	m_pncvisparent->SampleData(m_dSampleX, m_dSampleY, m_imagemap);

	if (fRedraw) {
		GenerateImageFromImageMap(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetCoordinateRange(
	size_t sMapWidth,
	size_t sMapHeight,
	double dX0,
	double dX1,
	double dY0,
	double dY1,
	bool fRedraw
) {
	m_dXrange[0] = dX0;
	m_dXrange[1] = dX1;
	m_dYrange[0] = dY0;
	m_dYrange[1] = dY1;

	m_dSampleX.resize(sMapWidth);
	for (size_t i = 0; i < sMapWidth; i++) {
		m_dSampleX[i] = m_dXrange[0] + (m_dXrange[1] - m_dXrange[0]) * (static_cast<double>(i) + 0.5) / static_cast<double>(sMapWidth);
	}

	m_dSampleY.resize(sMapHeight);
	for (size_t j = 0; j < sMapHeight; j++) {
		m_dSampleY[j] = m_dYrange[0] + (m_dYrange[1] - m_dYrange[0]) * (static_cast<double>(j) + 0.5) / static_cast<double>(sMapHeight);
	}

	m_pncvisparent->SetDisplayedBounds(m_dXrange[0], m_dXrange[1], m_dYrange[0], m_dYrange[1]);

	ResampleData(fRedraw);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::SetCoordinateRange(
	double dX0,
	double dX1,
	double dY0,
	double dY1,
	bool fRedraw
) {
	wxSize wxs;
	wxPosition wxp;
	GetMapPositionSize(wxs, wxp);

	_ASSERT(wxs.GetWidth() >= 0);
	_ASSERT(wxs.GetHeight() >= 0);

	SetCoordinateRange(
		wxs.GetWidth(),
		wxs.GetHeight(),
		dX0, dX1,
		dY0, dY1,
		fRedraw);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::ImposeImageSize(
	size_t sImageWidth,
	size_t sImageHeight
) {
	if ((sImageWidth - LABELBAR_IMAGEWIDTH == m_dSampleX.size()) &&
	    (sImageHeight == m_dSampleY.size())
	) {
		return;
	}

	m_fEnableRedraw = false;

	wxSize wxs;
	wxPosition wxp;
	GetMapPositionSize(wxs, wxp, sImageWidth, sImageHeight);

	_ASSERT(wxs.GetWidth() >= 0);
	_ASSERT(wxs.GetHeight() >= 0);

	SetCoordinateRange(
		wxs.GetWidth(),
		wxs.GetHeight(),
		m_dXrange[0], m_dXrange[1],
		m_dYrange[0], m_dYrange[1],
		false);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::ResetImageSize() {
	if (m_fEnableRedraw) {
		return;
	}

	SetCoordinateRange(
		m_dXrange[0], m_dXrange[1],
		m_dYrange[0], m_dYrange[1],
		false);

	m_fEnableRedraw = true;
}

////////////////////////////////////////////////////////////////////////////////

wxSize wxImagePanel::GetImageSize() {
	wxSize wxs = this->GetSize();
	wxs.DecBy(2 * DISPLAY_BORDER);
	return wxs;
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

void wxImagePanel::CalculateStringMinImageBufferSize(
	SFT & sft,
	const std::string & str,
	size_t & sMinWidth,
	size_t & sMinHeight,
	size_t & sBaseline
) {
	sMinWidth = 0;
	sMinHeight = 0;
	sBaseline = 0;

	for (int i = 0; i < str.length(); i++) {
		SFT_Glyph gid;
		if (sft_lookup(&sft, str[i], &gid) < 0) {
			std::cout << "FATAL ERROR IN SFT: " << str[i] << " missing" << std::endl;
			return;
		}

		SFT_GMetrics mtx;
		if (sft_gmetrics(&sft, gid, &mtx) < 0) {
			std::cout << "FATAL ERROR IN SFT: " << str[i] << " bad glyph metrics" << std::endl;
			return;
		}

		if (-mtx.yOffset > sBaseline) {
			sBaseline = -mtx.yOffset;
		}
		if (mtx.minHeight > sMinHeight) {
			sMinHeight = mtx.minHeight;
		}
		if (mtx.minHeight + sBaseline > sMinHeight) {
			sMinHeight = mtx.minHeight + sBaseline;
		}
		if (i != str.length()-1) {
			sMinWidth += mtx.advanceWidth;
		} else {
			sMinWidth += mtx.minWidth;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

template <int NDIM>
void wxImagePanel::DrawCharacter(
	SFT & sft,
	unsigned char c,
	size_t sX,
	size_t sY,
	size_t sCanvasWidth,
	size_t sCanvasHeight,
	unsigned char * imagedata,
	int * pwidth,
	int * pheight
) {
	SFT_Glyph gid;
	if (sft_lookup(&sft, c, &gid) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " missing" << std::endl;
		return;
	}

	SFT_GMetrics mtx;
	if (sft_gmetrics(&sft, gid, &mtx) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " bad glyph metrics" << std::endl;
		return;
	}

	SFT_Image img;
	img.width = (mtx.minWidth + 3) & ~3;
	img.height = mtx.minHeight;
	char * pixels = new char[img.width * img.height];
	img.pixels = pixels;
	if (sft_render(&sft, gid, img) < 0) {
		std::cout << "FATAL ERROR IN SFT: " << c << " not rendered" << std::endl;
		return;
	}

	if (pwidth != NULL) {
		(*pwidth) = mtx.advanceWidth;
	}
	if (pheight != NULL) {
		(*pheight) = mtx.minHeight;
	}

	int s = 0;
	for (int j = 0; j < img.height; j++) {
		size_t jx = sY + static_cast<size_t>(j) + mtx.yOffset;
		for (int i = 0; i < img.width; i++) {
			size_t ix = sX + static_cast<size_t>(i) + mtx.leftSideBearing;
			float dShadingFrac = static_cast<float>(255 - pixels[s]) / 255.0;
			imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 0] *= dShadingFrac;
			imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 1] *= dShadingFrac;
			imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 2] *= dShadingFrac;
			s++;
		}
	}

	delete[] pixels;
}

////////////////////////////////////////////////////////////////////////////////

template <int NDIM>
void wxImagePanel::DrawString(
	SFT & sft,
	const std::string & str,
	size_t sX,
	size_t sY,
	TextAlignment eAlign,
	size_t sCanvasWidth,
	size_t sCanvasHeight,
	unsigned char * imagedata,
	int * pwidth,
	int * pheight
) {
	int cumulative_width = 0;
	int cumulative_height = 0;
	int width = 0;
	int height = 0;

	// Render left-aligned text
	if (eAlign == TextAlignment_Left) {
		for (int i = 0; i < str.length(); i++) {
			DrawCharacter<NDIM>(sft, str[i], sX, sY, sCanvasWidth, sCanvasHeight, imagedata, &width, &height);
			sX += static_cast<size_t>(width);
			cumulative_width += width;
			if (height > cumulative_height) {
				cumulative_height = height;
			}
		}

	// Render right-aligned and center-aligned text using an image buffer
	} else if ((eAlign == TextAlignment_Right) || (eAlign == TextAlignment_Center)) {
		size_t sMinBufferWidth;
		size_t sMinBufferHeight;
		size_t sBaseline;

		CalculateStringMinImageBufferSize(sft, str, sMinBufferWidth, sMinBufferHeight, sBaseline);

		if (sMinBufferWidth == 0) {
			if (pwidth != NULL) {
				(*pwidth) = 0;
			}
			if (pheight != NULL) {
				(*pheight) = 0;
			}
			return;
		}

		cumulative_height = sMinBufferHeight;

		unsigned char * stringbuf = new unsigned char[NDIM * sMinBufferWidth * sMinBufferHeight];
		memset(stringbuf, 255, NDIM * sMinBufferWidth * sMinBufferHeight * sizeof(unsigned char));

		size_t sPenX = 0;

		for (int i = 0; i < str.length(); i++) {
			DrawCharacter<NDIM>(sft, str[i], sPenX, sBaseline, sMinBufferWidth, sMinBufferHeight, stringbuf, &width, &height);
			sPenX += static_cast<size_t>(width);
			cumulative_width += width;
		}

		size_t sPenBeginX = 0;
		if (eAlign == TextAlignment_Right) {
			sPenBeginX = sX - sPenX;
		} else if (eAlign == TextAlignment_Center) {
			sPenBeginX = sX - sPenX / 2;
		}

		for (int i = 0; i < sMinBufferWidth; i++) {
			int ix = sPenBeginX + i;
			for (int j = 0; j < sMinBufferHeight; j++) {
				int jx = sY - sBaseline + j;

				if (sCanvasWidth * jx + ix >= sCanvasWidth * sCanvasHeight) {
					continue;
				}
				if (j * sMinBufferWidth + i >= sMinBufferWidth * sMinBufferHeight) {
					continue;
				}
				float dShadingFrac = static_cast<float>(stringbuf[NDIM * (j * sMinBufferWidth + i) + 0]) / 255.0; 
				imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 0] *= dShadingFrac;
				imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 1] *= dShadingFrac;
				imagedata[NDIM * sCanvasWidth * jx + NDIM * ix + 2] *= dShadingFrac;
			}
		}

		delete[] stringbuf;
	}

	if (pwidth != NULL) {
		(*pwidth) = cumulative_width;
	}
	if (pheight != NULL) {
		(*pheight) = cumulative_height;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool wxImagePanel::ExportToPNG(
	const wxString & wxstrFilename,
	size_t sImageWidth,
	size_t sImageHeight
) {
	if ((sImageWidth == static_cast<size_t>(-1)) ||
	    (sImageHeight == static_cast<size_t>(-1))
	) {
		wxSize wxs = this->GetSize();

		if (sImageWidth == static_cast<size_t>(-1)) {
			sImageWidth = wxs.GetWidth() - 2 * DISPLAY_BORDER;
		}
		if (sImageHeight == static_cast<size_t>(-1)) {
			sImageHeight = wxs.GetHeight() - 2 * DISPLAY_BORDER;
		}
	}

	std::vector<unsigned char> pngimage(sImageWidth * sImageHeight * 4);

	GenerateImageDataFromImageMap<4>(
		0, 0,
		sImageWidth, sImageHeight,
		sImageWidth, sImageHeight,
		&(pngimage[0]));

	unsigned error = lodepng::encode(wxstrFilename.ToStdString(), pngimage, sImageWidth, sImageHeight);
	if (error) {
		std::cout << "PNG encoder error (" << error << "): " << lodepng_error_text(error) << std::endl;
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::PaintNow() {
	wxClientDC dc(this);
	Render(dc);
}

////////////////////////////////////////////////////////////////////////////////

void wxImagePanel::Render(wxDC & dc) {
	dc.DrawBitmap(m_image, 0, 0, false);
}

////////////////////////////////////////////////////////////////////////////////

