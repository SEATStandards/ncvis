#ifndef _WXNCVISLINEPLOT_H_
#define _WXNCVISLINEPLOT_H_

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <string>
#include <ctime>

class wxNcVisLinePlotPanel : public wxPanel {
public:
    wxNcVisLinePlotPanel(
        wxWindow * parent,
        const std::vector<double> & x,
        const std::vector<float>  & y,
        const wxString & xLabel, 
        time_t baseEpoch, 
        const wxString & yLabel,
        const wxString & title);

    void SetData(
        const std::vector<double> & x,
        const std::vector<float>  & y,
        const wxString & xLabel,     
        time_t baseEpoch,
        const wxString & yLabel,
        const wxString & title);

    wxString FormatXValue(double xv) const;
    bool ExportToPNG(const wxString & filename, int width, int height);

private:
    void DrawPlot(wxDC & dc, const wxSize & sz);
    void OnPaint(wxPaintEvent & evt);

    std::vector<double> m_x;
    std::vector<float>  m_y;

    wxString m_xLabel;
    time_t m_baseEpoch;

    wxString m_yLabel;
    wxString m_title;

    wxDECLARE_EVENT_TABLE();
};

class wxNcVisLinePlotFrame : public wxFrame {
public:
    wxNcVisLinePlotFrame(
        wxWindow * parent,
        const wxString & title,
        const std::vector<double> & x,
        const std::vector<float>  & y,
        const wxString & xLabel,
        time_t baseEpoch,
        const wxString & yLabel);

    void UpdatePlot(
        const wxString & title,
        const std::vector<double> & x,
        const std::vector<float>  & y,
        const wxString & xLabel,
        time_t baseEpoch,
        const wxString & yLabel);

private:
    void OnExportPng(wxCommandEvent & evt);
    wxNcVisLinePlotPanel * m_panel;
};

#endif
