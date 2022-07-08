///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxImagePanel.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _WXIMAGEPANEL_H_
#define _WXIMAGEPANEL_H_

#include <wx/wx.h>
#include <wx/sizer.h>

#include "DataArray1D.h"
#include "CoordTransforms.h"
#include "GridDataSampler.h"
#include "ColorMap.h"
#include "schrift.h"

class wxNcVisFrame;

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A wxWidget that manages display of the data.
///	</summary>
class wxImagePanel : public wxPanel {
public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxImagePanel(
		wxNcVisFrame * parent
	);

public:
	///	<summary>
	///		Callback for when a paint event is triggered.
	///	</summary>
	void OnPaint(wxPaintEvent & evt);

	///	<summary>
	///		Callback for when a size event is triggered.
	///	</summary>
	void OnSize(wxSizeEvent & evt);

	///	<summary>
	///		Callback for when an idle event is triggered.
	///	</summary>
	void OnIdle(wxIdleEvent & evt);

	///	<summary>
	///		Callback for when the mouse is moved.
	///	</summary>
	void OnMouseMotion(wxMouseEvent & evt);

	///	<summary>
	///		Callback for when the mouse leaves the window.
	///	</summary>
	void OnMouseLeaveWindow(wxMouseEvent & evt);

	///	<summary>
	///		Callback for when the mouse is left-double-clicked.
	///	</summary>
	void OnMouseLeftDoubleClick(wxMouseEvent & evt);

public:
	///	<summary>
	///		Format a label bar label from a value.
	///	</summary>
	void FormatLabelBarLabel(
		double dValue,
		std::string & str
	);

	///	<summary>
	///		Generate the image from the image map.
	///	</summary>
	void GenerateImageFromImageMap(
		bool fRedraw = false
	);

	///	<summary>
	///		Set the color map.
	///	</summary>
	void SetColorMap(
		const std::string & strColorMap,
		bool fRedraw = false
	);

	///	<summary>
	///		Resample the coordinate range.
	///	</summary>
	void SetCoordinateRange(
		double dX0,
		double dX1,
		double dY0,
		double dY1,
		bool fRedraw = false
	);

	///	<summary>
	///		Set the data range.
	///	</summary>
	void SetDataRange(
		float dDataMin,
		float dDataMax,
		bool fRedraw = false
	);

public:
	///	<summary>
	///		Get the x coordinate range minimum.
	///	</summary>
	double GetXRangeMin() const {
		return m_dXrange[0];
	}

	///	<summary>
	///		Get the x coordinate range maximum.
	///	</summary>
	double GetXRangeMax() const {
		return m_dXrange[1];
	}

	///	<summary>
	///		Get the x coordinate range minimum.
	///	</summary>
	double GetYRangeMin() const {
		return m_dYrange[0];
	}

	///	<summary>
	///		Get the x coordinate range maximum.
	///	</summary>
	double GetYRangeMax() const {
		return m_dYrange[1];
	}

	///	<summary>
	///		Get the data range minimum.
	///	</summary>
	float GetDataRangeMin() const {
		return m_dDataRange[0];
	}

	///	<summary>
	///		Get the data range maximum.
	///	</summary>
	float GetDataRangeMax() const {
		return m_dDataRange[1];
	}

public:
	///	<summary>
	///		Draw the specified character at the specified location on the image,
	///		using font information from m_sft.  The coordinate (x,y) indicates
	///		the top-left corner of the character.
	///	</summary>
	void DrawCharacter(
		unsigned char c,
		size_t x,
		size_t y,
		int * pwidth = NULL,
		int * pheight = NULL);

	///	<summary>
	///		Draw the specified string at the specified location on the image.
	///		using font information from m_sft.  The coordinate (x,y) indicates
	///		the top-left corner of the string.
	///	</summary>
	void DrawString(
		const std::string & str,
		size_t x,
		size_t y,
		int * pwidth = NULL,
		int * pheight = NULL);

	///	<summary>
	///		Repaint the panel now.
	///	</summary>
	void PaintNow();

	///	<summary>
	///		Render the device context.
	///	</summary>
	void Render(wxDC & dc);

private:
	///	<summary>
	///		Pointer to parent frame.
	///	</summary>
	wxNcVisFrame * m_pncvisparent;

	///	<summary>
	///		Colormap.
	///	</summary>
	ColorMap m_colormap;

	///	<summary>
	///		Font information for label bar.
	///	</summary>
	SFT m_sft;

	///	<summary>
	///		Longitude region displayed in plot.
	///	</summary>
	double m_dXrange[2];

	///	<summary>
	///		Latitude region displayed in plot.
	///	</summary>
	double m_dYrange[2];

	///	<summary>
	///		Data range displayed in plot.
	///	</summary>
	float m_dDataRange[2];

	///	<summary>
	///		Array of sample points in X direction.
	///	</summary>
	DataArray1D<double> m_dSampleX;

	///	<summary>
	///		Array of sample points in Y direction.
	///	</summary>
	DataArray1D<double> m_dSampleY;

	///	<summary>
	///		Image map.
	///	</summary>
	DataArray1D<int> m_imagemap;

	///	<summary>
	///		Image bitmap data.
	///	</summary>
	wxImage m_image;

	///	<summary>
	///		A flag indicating the window has been resized.
	///	</summary>
	bool m_fResize;

	DECLARE_EVENT_TABLE()
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXIMAGEPANEL_H_

