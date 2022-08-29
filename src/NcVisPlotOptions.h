///////////////////////////////////////////////////////////////////////////////
///
///	\file    NcVisPlotOptions.h
///	\author  Paul Ullrich
///	\version August 25, 2022
///

#ifndef _NCVISPLOTOPTIONS_H_
#define _NCVISPLOTOPTIONS_H_

///////////////////////////////////////////////////////////////////////////////

class NcVisPlotOptions {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	NcVisPlotOptions() :
		m_fShowTitle(true),
		m_fShowTickmarkLabels(true),
		m_fShowGrid(false)
	{ }

public:
	///	<summary>
	///		Equality operator.
	///	</summary>
	bool operator==(const NcVisPlotOptions & plotopts) const {
		return (
			(m_fShowTitle == plotopts.m_fShowTitle) &&
			(m_fShowTickmarkLabels == plotopts.m_fShowTickmarkLabels) &&
			(m_fShowGrid == plotopts.m_fShowGrid));
	}

	///	<summary>
	///		Inequality operator.
	///	</summary>
	bool operator!=(const NcVisPlotOptions & plotopts) const {
		return !((*this) == plotopts);
	}

public:
	///	<summary>
	///		Show the title in the image.
	///	</summary>
	bool m_fShowTitle;

	///	<summary>
	///		Show tickmark labels in the image.
	///	</summary>
	bool m_fShowTickmarkLabels;

	///	<summary>
	///		Show grid in the image.
	///	</summary>
	bool m_fShowGrid;

};

///////////////////////////////////////////////////////////////////////////////

#endif // _NCVISPLOTOPTIONS_H_

