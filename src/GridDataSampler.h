///////////////////////////////////////////////////////////////////////////////
///
///	\file    GridDataSampler.h
///	\author  Paul Ullrich
///	\version July 13, 2022
///

#ifndef _GRIDDATASAMPLER_H_
#define _GRIDDATASAMPLER_H_

#include "Announce.h"
#include "QuadTree.h"
#include "kdtree.h"
#include "netcdfcpp.h"

#include <vector>

///////////////////////////////////////////////////////////////////////////////

class GridDataSampler {
public:
	///	<summary>
	///		Constructor.
	///	</summary>
	GridDataSampler() :
		m_fIsInitialized(false)
	{ }

	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const std::vector<double> & dLon,
		const std::vector<double> & dLat
	);

	///	<summary>
	///		Sample.
	///	</summary>
	virtual void Sample(
		const std::vector<double> & dSampleLon,
		const std::vector<double> & dSampleLat,
		std::vector<int> & dImageMap
	) const = 0;

	///	<summary>
	///		Check if initialized.
	///	</summary>
	bool IsInitialized() const {
		return m_fIsInitialized;
	}

private:
	///	<summary>
	///		Flag indicating this GridDataSampler is initialized.
	///	</summary>
	bool m_fIsInitialized;
};

///////////////////////////////////////////////////////////////////////////////

class GridDataSamplerUsingCubedSphereQuadTree : public GridDataSampler {
public:
	///	<summary>
	///		Convert a RLL coordinate to an equiangular cubed-sphere ABP coordinate.
	///	</summary>
	static void ABPFromRLL(
		double dLon,
		double dLat,
		double & dA,
		double & dB,
		int & nP
	);

	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const std::vector<double> & dLon,
		const std::vector<double> & dLat,
		double dFillValue
	);

	///	<summary>
	///		Generate
	///	</summary>
	virtual void Sample(
		const std::vector<double> & dSampleLon,
		const std::vector<double> & dSampleLat,
		std::vector<int> & dImageMap
	) const;

public:
	///	<summary>
	///		QuadTree root node.
	///	</summary>
	std::vector<QuadTreeNode> m_vecquadtree;

	///	<summary>
	///		Apply distance criteria for filtering sample points.
	///	</summary>
	bool m_fDistanceFilter;

	///	<summary>
	///		Distance criteria for filtering sample points.
	///	</summary>
	double m_dMaxDist;
};

///////////////////////////////////////////////////////////////////////////////

class GridDataSamplerUsingQuadTree : public GridDataSampler {
public:
	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const std::vector<double> & dLon,
		const std::vector<double> & dLat,
		double dFillValue
	);

	///	<summary>
	///		Generate
	///	</summary>
	virtual void Sample(
		const std::vector<double> & dSampleLon,
		const std::vector<double> & dSampleLat,
		std::vector<int> & dImageMap
	) const;

public:
	///	<summary>
	///		QuadTree root node.
	///	</summary>
	QuadTreeNode m_quadtree;

	///	<summary>
	///		Apply distance criteria for filtering sample points.
	///	</summary>
	bool m_fDistanceFilter;

	///	<summary>
	///		Distance criteria for filtering sample points.
	///	</summary>
	double m_dMaxDist;
};

///////////////////////////////////////////////////////////////////////////////

class GridDataSamplerUsingKDTree : public GridDataSampler {
public:
	///	<summary>
	///		Constructor.
	///	</summary>
	GridDataSamplerUsingKDTree() :
		m_kdtree(NULL)
	{ }

	///	<summary>
	///		Destructor.
	///	</summary>
	~GridDataSamplerUsingKDTree();

	///	<summary>
	///		Initialize.
	///	</summary>
	virtual void Initialize(
		const std::vector<double> & dLon,
		const std::vector<double> & dLat,
		double dFillValue
	);

	///	<summary>
	///		Generate
	///	</summary>
	virtual void Sample(
		const std::vector<double> & dSampleLon,
		const std::vector<double> & dSampleLat,
		std::vector<int> & dImageMap
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
