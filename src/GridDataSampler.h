///////////////////////////////////////////////////////////////////////////////
///
///	\file    GridDataSampler.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _GRIDDATASAMPLER_H_
#define _GRIDDATASAMPLER_H_

#include "Announce.h"
#include "QuadTree.h"
#include "kdtree.h"
#include "DataArray1D.h"
#include "netcdfcpp.h"

///////////////////////////////////////////////////////////////////////////////

class GridDataSampler {
public:
	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const DataArray1D<double> & dLon,
		const DataArray1D<double> & dLat
	) = 0;

	///	<summary>
	///		Sample.
	///	</summary>
	virtual void Sample(
		const DataArray1D<double> & dSampleLon,
		const DataArray1D<double> & dSampleLat,
		DataArray1D<int> & dImageMap
	) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class GridDataSamplerUsingQuadTree : public GridDataSampler {
public:
	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const DataArray1D<double> & dLon,
		const DataArray1D<double> & dLat
	);

	///	<summary>
	///		Generate
	///	</summary>
	virtual void Sample(
		const DataArray1D<double> & dSampleLon,
		const DataArray1D<double> & dSampleLat,
		DataArray1D<int> & dImageMap
	) const;

public:
	///	<summary>
	///		QuadTree root node.
	///	</summary>
	QuadTreeNode m_quadtree;
};

///////////////////////////////////////////////////////////////////////////////

class GridDataSamplerUsingKDTree : public GridDataSampler {
public:
	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const DataArray1D<double> & dLon,
		const DataArray1D<double> & dLat
	);

	///	<summary>
	///		Generate
	///	</summary>
	virtual void Sample(
		const DataArray1D<double> & dSampleLon,
		const DataArray1D<double> & dSampleLat,
		DataArray1D<int> & dImageMap
	) const;

public:
	///	<summary>
	///		Reference address.
	///	</summary>
	int m_iRef;

	///	<summary>
	///		QuadTree root node.
	///	</summary>
	kdtree * m_kdtree;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _GRIDDATASAMPLER_H_
