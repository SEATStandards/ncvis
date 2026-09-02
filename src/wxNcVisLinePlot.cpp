#include "wxNcVisLinePlot.h"
#include <algorithm>
#include <cmath>
#include <wx/settings.h>
#include <limits>
#include <ctime>

enum {
    ID_LinePlot_ExportPng = wxID_HIGHEST + 1
};

wxBEGIN_EVENT_TABLE(wxNcVisLinePlotPanel, wxPanel)
    EVT_PAINT(wxNcVisLinePlotPanel::OnPaint)
wxEND_EVENT_TABLE()

wxNcVisLinePlotPanel::wxNcVisLinePlotPanel(
    wxWindow * parent,
    const std::vector<double> & x,
    const std::vector<float>  & y,
    const wxString & xLabel,
    time_t baseEpoch,
    const wxString & yLabel,
    const wxString & title
) : wxPanel(parent),
    m_x(x), m_y(y),
    m_xLabel(xLabel),
    m_baseEpoch(baseEpoch),
    m_yLabel(yLabel),
    m_title(title)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

wxString wxNcVisLinePlotPanel::FormatXValue(double xv) const {
    if (m_baseEpoch == 0) {
        return wxString::Format("%.4g", xv);
    }

    time_t t = m_baseEpoch + static_cast<time_t>(std::llround(xv * 86400.0));
    wxDateTime dt(t);

    if (!dt.IsValid()) {
        return wxString::Format("%.4g", xv);
    }

    return dt.Format("%Y-%m-%d");
}

void wxNcVisLinePlotPanel::SetData(
    const std::vector<double> & x,
    const std::vector<float>  & y,
    const wxString & xLabel,
    time_t baseEpoch,
    const wxString & yLabel,
    const wxString & title
) {
    m_x = x;
    m_y = y;
    m_xLabel = xLabel;
    m_baseEpoch = baseEpoch;
    m_yLabel = yLabel;
    m_title = title;
    Refresh();
}

void wxNcVisLinePlotPanel::OnPaint(wxPaintEvent &) {
    wxAutoBufferedPaintDC dc(this);
    DrawPlot(dc, GetClientSize());
}

void wxNcVisLinePlotPanel::DrawPlot(wxDC & dc, const wxSize & sz) {
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    if (sz.x < 80 || sz.y < 80) return;
    if (m_x.size() < 2 || m_y.size() < 2) return;

    // Padding tuned for titles + range labels
    const int padL = 85, padR = 60, padT = 35, padB = 85;
    const int w = sz.x - padL - padR;
    const int h = sz.y - padT - padB;
    if (w <= 10 || h <= 10) return;

    // Plot rectangle (white box)
    wxRect plotRect(padL, padT, w, h);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(plotRect);

    // Bounds
    auto mm = std::minmax_element(m_x.begin(), m_x.end());
    double xmin = *(mm.first);
    double xmax = *(mm.second);

    float ymin = m_y[0], ymax = m_y[0];
    for (float v : m_y) {
        ymin = std::min(ymin, v);
        ymax = std::max(ymax, v);
    }

    // Avoid zero ranges
    if (xmax == xmin) xmax = xmin + 1.0;
    if (ymax == ymin) ymax = ymin + 1.0f;

    auto X = [&](double xv) -> int {
        return padL + (int)std::lround((xv - xmin) * (double)w / (xmax - xmin));
    };
    auto Y = [&](float yv) -> int {
        return padT + (int)std::lround((double)(ymax - yv) * (double)h / (double)(ymax - ymin));
    };

    const int nXTicks = 6;
    const int nYTicks = 6;

    // Grid lines
    dc.SetPen(wxPen(wxColour(180,180,180), 1, wxPENSTYLE_DOT));

    for (int i = 0; i < nXTicks; ++i) {
        double xv = xmin + (xmax - xmin) * static_cast<double>(i) / static_cast<double>(nXTicks - 1);
        int px = X(xv);
        dc.DrawLine(px, padT, px, padT + h);
    }

    for (int i = 0; i < nYTicks; ++i) {
        double yv = ymin + (ymax - ymin) * static_cast<double>(i) / static_cast<double>(nYTicks - 1);
        int py = Y(static_cast<float>(yv));
        dc.DrawLine(padL, py, padL + w, py);
    }

    // Redraw border on top of grid
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(plotRect);

    // Title (top)
    wxFont fontTitle = dc.GetFont();
    fontTitle.SetWeight(wxFONTWEIGHT_BOLD);
    fontTitle.SetPointSize(fontTitle.GetPointSize() + 2);
    dc.SetFont(fontTitle);

    wxSize titleSz = dc.GetTextExtent(m_title);
    dc.DrawText(m_title, padL + (w - titleSz.x) / 2, 8);

    wxFont fontNormal = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    dc.SetFont(fontNormal);

    // Axis titles
    wxSize xLabSz = dc.GetTextExtent(m_xLabel);
    dc.DrawText(m_xLabel, padL + (w - xLabSz.x) / 2, padT + h + 40);

    // Rotated Y label on left
    // (90 degrees puts it reading bottom-to-top; use 270 if you prefer top-to-bottom)
    dc.DrawRotatedText(m_yLabel, 15, padT + h/2 + dc.GetTextExtent(m_yLabel).x/2, 90.0);

    // Axis range labels (min/max)
    for (int i = 0; i < nXTicks; ++i) {
        double xv = xmin + (xmax - xmin) * static_cast<double>(i) / static_cast<double>(nXTicks - 1);
        int px = X(xv);

        // tick mark
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(px, padT + h, px, padT + h + 5);

        // label
        wxString s = FormatXValue(xv);
        wxSize tsz = dc.GetTextExtent(s);
        dc.DrawText(s, px - tsz.x / 2, padT + h + 12);
    }

    for (int i = 0; i < nYTicks; ++i) {
        double yv = ymin + (ymax - ymin) * static_cast<double>(i) / static_cast<double>(nYTicks - 1);
        int py = Y(static_cast<float>(yv));

        // tick mark
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(padL - 5, py, padL, py);

        // label
        wxString s = wxString::Format("%.4g", yv);
        wxSize tsz = dc.GetTextExtent(s);
        dc.DrawText(s, padL - 10 - tsz.x, py - tsz.y / 2);
    }

    // Line plot
    dc.SetPen(*wxBLACK_PEN);
    wxPoint prev(X(m_x[0]), Y(m_y[0]));
    for (size_t i = 1; i < m_x.size(); ++i) {
        wxPoint cur(X(m_x[i]), Y(m_y[i]));
        dc.DrawLine(prev, cur);
        prev = cur;
    }

    // Dots at each point
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxBLACK_BRUSH);
    const int r = 2;
    for (size_t i = 0; i < m_x.size(); ++i) {
        dc.DrawCircle(X(m_x[i]), Y(m_y[i]), r);
    }
}

bool wxNcVisLinePlotPanel::ExportToPNG(const wxString & filename, int width, int height) {
    wxBitmap bmp(width, height);
    wxMemoryDC dc(bmp);

    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.Clear();

    wxSize oldSize = GetClientSize();
    SetSize(width, height);

    DrawPlot(dc, wxSize(width, height));

    SetSize(oldSize);

    dc.SelectObject(wxNullBitmap);

    wxImage img = bmp.ConvertToImage();
    if (!img.IsOk()) {
        return false;
    }

    return img.SaveFile(filename, wxBITMAP_TYPE_PNG);
}

wxNcVisLinePlotFrame::wxNcVisLinePlotFrame(
    wxWindow * parent,
    const wxString & title,
    const std::vector<double> & x,
    const std::vector<float>  & y,
    const wxString & xLabel,
    time_t baseEpoch,
    const wxString & yLabel
) : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(750, 450))
{
    wxBoxSizer * pTopSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer * pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton * pExportBtn = new wxButton(this, ID_LinePlot_ExportPng, "Export PNG");
    pExportBtn->Bind(wxEVT_BUTTON, &wxNcVisLinePlotFrame::OnExportPng, this);
    pButtonSizer->Add(pExportBtn, 0, wxALL, 5);

    m_panel = new wxNcVisLinePlotPanel(this, x, y, xLabel, baseEpoch, yLabel, title);

    pTopSizer->Add(pButtonSizer, 0, wxEXPAND);
    pTopSizer->Add(m_panel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 1);

    SetSizer(pTopSizer);
    //SetBackgroundColour(*wxBLACK);
}

void wxNcVisLinePlotFrame::UpdatePlot(
    const wxString & title,
    const std::vector<double> & x,
    const std::vector<float>  & y,
    const wxString & xLabel,
    time_t baseEpoch,
    const wxString & yLabel
) {
    SetTitle(title);
    m_panel->SetData(x, y, xLabel, baseEpoch, yLabel, title);
    Raise();
}

void wxNcVisLinePlotFrame::OnExportPng(wxCommandEvent & evt) {
    wxFileDialog dlg(
        this,
        "Save plot as PNG",
        "",
        "lineplot.png",
        "PNG files (*.png)|*.png",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    wxSize sz = m_panel->GetClientSize();
    if (sz.x <= 0 || sz.y <= 0) {
        wxMessageBox("Invalid plot size.", "Export PNG");
        return;
    }

    wxBitmap bmp(sz.x, sz.y);
    wxMemoryDC memdc(bmp);
    memdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    memdc.Clear();

    m_panel->Refresh();
    m_panel->Update();

    wxClientDC srcdc(m_panel);
    memdc.Blit(0, 0, sz.x, sz.y, &srcdc, 0, 0);

    memdc.SelectObject(wxNullBitmap);

    if (!m_panel->ExportToPNG(dlg.GetPath(), m_panel->GetClientSize().x, m_panel->GetClientSize().y)) {
        wxMessageBox("Failed to save PNG.", "Export PNG");
        return;
    }

    wxMessageBox("PNG saved successfully.", "Export PNG");
}
