///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxImagePanel.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxImagePanel.h"
#include "wxNcVisFrame.h"

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

wxImagePanel::wxImagePanel(
	wxFrame * parent,
	const GridDataSampler * pgds,
	const DataArray1D<float> * pdata
) :
	wxPanel(parent),
	m_pgds(pgds),
	m_pdata(pdata),
	m_image(720, 360),
	m_fResize(false)
{
	ColorMapLibrary colormaplib;
	SetColorMap(colormaplib.GetColorMapName(0));

	SetSize(wxSize(720,360));
	SetMinSize(wxSize(720,360));
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

	if ((pos.x < 0) || (pos.x >= m_dSampleLon.GetRows())) {
		return;
	}
	if ((pos.y < 0) || (pos.y >= m_dSampleLat.GetRows())) {
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

	wxNcVisFrame * ncvisparent = dynamic_cast<wxNcVisFrame *>(GetParent());
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

void wxImagePanel::GenerateImageFromImageMap(
	bool fRedraw
) {

	wxSize wxs = this->GetSize();

	size_t sWidth = wxs.GetWidth();
	size_t sHeight = wxs.GetHeight();

	m_image.Resize(wxs, wxPoint(0,0), 255, 255, 255);

	unsigned char * imagedata = m_image.GetData();
	int s = 0;
	for (size_t j = 0; j < sHeight; j++) {
	for (size_t i = 0; i < sWidth; i++) {
		size_t jx = sHeight - j - 1;

		m_colormap.Sample(
			(*m_pdata)[m_imagemap[s]],
			m_dDataRange[0],
			m_dDataRange[1],
			imagedata[3 * sWidth * jx + 3 * i + 0],
			imagedata[3 * sWidth * jx + 3 * i + 1],
			imagedata[3 * sWidth * jx + 3 * i + 2]);

		s++;
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

	m_dXrange[0] = dX0;
	m_dXrange[1] = dX1;
	m_dYrange[0] = dY0;
	m_dYrange[1] = dY1;

	m_dSampleLon.Allocate(sWidth);
	for (size_t i = 0; i < sWidth; i++) {
		m_dSampleLon[i] = m_dXrange[0] + (m_dXrange[1] - m_dXrange[0]) * (static_cast<double>(i) + 0.5) / static_cast<double>(sWidth);
		m_dSampleLon[i] = LonDegToStandardRange(m_dSampleLon[i]);
	}

	m_dSampleLat.Allocate(sHeight);
	for (size_t j = 0; j < sHeight; j++) {
		m_dSampleLat[j] = m_dYrange[0] + (m_dYrange[1] - m_dYrange[0]) * (static_cast<double>(j) + 0.5) / static_cast<double>(sHeight);
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

