///////////////////////////////////////////////////////////////////////////////
///
///	\file    GridDataSampler.cpp
///	\author  Paul Ullrich
///	\version July 13, 2022
///

#include "GridDataSampler.h"
#include "CoordTransforms.h"
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// GridDataSampler
///////////////////////////////////////////////////////////////////////////////

void GridDataSampler::Initialize(
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	m_fIsInitialized = true;
}

///////////////////////////////////////////////////////////////////////////////
// GridDataSamplerUsingCubedSphereQuadTree
///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingCubedSphereQuadTree::ABPFromRLL(
	double dLonDeg,
	double dLatDeg,
	double & dA,
	double & dB,
	int & nP
) {
	const double dLonRad = DegToRad(dLonDeg);
	const double dLatRad = DegToRad(dLatDeg);

	// Default panel to unattainable value
	nP = 6;

	// Translate from RLL coordinates to XYZ space
	double xx, yy, zz, pm;

	xx = cos(dLonRad) * cos(dLatRad);
	yy = sin(dLonRad) * cos(dLatRad);
	zz = sin(dLatRad);

	pm = std::max(fabs(xx), std::max(fabs(yy), fabs(zz)));

	// Check maxmality of the x coordinate
	if (pm == fabs(xx)) {
		if (xx > 0) {
			nP = 0;
		} else {
			nP = 2;
		}
	}

	// Check maximality of the y coordinate
	if (pm == fabs(yy)) {
		if (yy > 0) {
			nP = 1;
		} else {
			nP = 3;
		}
	}

	// Check maximality of the z coordinate
	if (pm == fabs(zz)) {
		if (zz > 0) {
			nP = 4;
		} else {
			nP = 5;
		}
	}

	// Panel assignments
	double sx, sy, sz;
	if (nP == 0) {
		sx = yy;
		sy = zz;
		sz = xx;

	} else if (nP == 1) {
		sx = -xx;
		sy = zz;
		sz = yy;

	} else if (nP == 2) {
		sx = -yy;
		sy = zz;
		sz = -xx;

	} else if (nP == 3) {
		sx = xx;
		sy = zz;
		sz = -yy;

	} else if (nP == 4) {
		sx = yy;
		sy = -xx;
		sz = zz;

	} else if (nP == 5) {
		sx = yy;
		sy = xx;
		sz = -zz;

	} else {
		_EXCEPTIONT("Logic error.");
	}

	// Convert to equiangular coordinates
	dA = atan(sx / sz);
	dB = atan(sy / sz);
}

////////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingCubedSphereQuadTree::Initialize(
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	GridDataSampler::Initialize(dLon, dLat);

	_ASSERT(dLon.GetRows() == dLat.GetRows());

	AnnounceStartBlock("Generating quadtree from lat/lon arrays");

	m_vecquadtree.resize(6, QuadTreeNode(-0.5*M_PI, 0.5*M_PI, -0.5*M_PI, 0.5*M_PI));

	long iReportSize = static_cast<long>(dLon.GetRows()) / 100;
	for (long i = 0; i < dLon.GetRows(); i++) {

		double dA;
		double dB;
		int nP;

		ABPFromRLL(dLon[i], dLat[i], dA, dB, nP);

		_ASSERT((nP >= 0) && (nP <= 5));

		m_vecquadtree[nP].insert(dA, dB, i);

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingCubedSphereQuadTree::Sample(
	const DataArray1D<double> & dSampleLon,
	const DataArray1D<double> & dSampleLat,
	DataArray1D<int> & dImageMap
) const {
	dImageMap.Allocate(dSampleLon.GetRows() * dSampleLat.GetRows());

	int s = 0;
	for (int j = 0; j < dSampleLat.GetRows(); j++) {
	for (int i = 0; i < dSampleLon.GetRows(); i++) {

		double dA;
		double dB;
		int nP;

		ABPFromRLL(dSampleLon[i], dSampleLat[j], dA, dB, nP);

		_ASSERT((nP >= 0) && (nP <= 5));

		size_t sI = m_vecquadtree[nP].find_inexact(dA, dB);

		if (sI == static_cast<size_t>(-1)) {
			dImageMap[s] = 0;
		} else {
			dImageMap[s] = sI;
		}

		s++;
	}
	}
}

///////////////////////////////////////////////////////////////////////////////
// GridDataSamplerUsingQuadTree
///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Initialize(
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	GridDataSampler::Initialize(dLon, dLat);

	_ASSERT(dLon.GetRows() == dLat.GetRows());

	AnnounceStartBlock("Generating quadtree from lat/lon arrays");

	long iReportSize = static_cast<long>(dLon.GetRows()) / 100;
	for (long i = 0; i < dLon.GetRows(); i++) {

		double dLonStandard = LonDegToStandardRange(dLon[i]);

		m_quadtree.insert(dLonStandard, dLat[i], i);

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Sample(
	const DataArray1D<double> & dSampleLon,
	const DataArray1D<double> & dSampleLat,
	DataArray1D<int> & dImageMap
) const {
	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.GetRows()) * static_cast<long>(dSampleLon.GetRows()) / 100;
	//AnnounceStartBlock("Querying data points within the quadtree");

	dImageMap.Allocate(dSampleLon.GetRows() * dSampleLat.GetRows());

	int s = 0;
	for (int j = 0; j < dSampleLat.GetRows(); j++) {
	for (int i = 0; i < dSampleLon.GetRows(); i++) {
		size_t sI = m_quadtree.find_inexact(LonDegToStandardRange(dSampleLon[i]), dSampleLat[j]);

		if (sI == static_cast<size_t>(-1)) {
			dImageMap[s] = 0;//_EXCEPTION();
		} else {
			dImageMap[s] = sI;
		}

		//lTotal++;
		//if (lTotal % lReportSize == 0) {
		//	Announce("%li%% complete", lTotal / lReportSize);
		//}

		s++;
	}
	}
	//AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////
// GridDataSamplerUsingKDTree
///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Initialize(
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	GridDataSampler::Initialize(dLon, dLat);

	AnnounceStartBlock("Generating kdtree from from lat/lon arrays");

	m_kdtree = kd_create(3);
	if (m_kdtree == NULL) {
		_EXCEPTIONT("kd_create(3) failed");
	}

	long iReportSize = static_cast<long>(dLon.GetRows()) / 100;
	for (long i = 0; i < static_cast<long>(dLon.GetRows()); i++) {

		double dX;
		double dY;
		double dZ;

		RLLtoXYZ_Deg(dLon[i], dLat[i], dX, dY, dZ);

		kd_insert3(m_kdtree, dX, dY, dZ, (void*)(&m_iRef + i));

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Sample(
	const DataArray1D<double> & dSampleLon,
	const DataArray1D<double> & dSampleLat,
	DataArray1D<int> & dImageMap
) const {

	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.GetRows()) * static_cast<long>(dSampleLon.GetRows()) / 100;
	//AnnounceStartBlock("Querying data points within the kdtree");

	dImageMap.Allocate(dSampleLon.GetRows() * dSampleLat.GetRows());

	int s = 0;
	for (int j = 0; j < dSampleLat.GetRows(); j++) {
	for (int i = 0; i < dSampleLon.GetRows(); i++) {

		double dX;
		double dY;
		double dZ;

		RLLtoXYZ_Deg(dSampleLon[i], dSampleLat[j], dX, dY, dZ);

		kdres * kdres = kd_nearest3(m_kdtree, dX, dY, dZ);
		if (kdres == NULL) {
			_EXCEPTIONT("Unexpected query failure in kdtree");
		}

		dImageMap[s] = static_cast<size_t>(reinterpret_cast<int*>(kd_res_item_data(kdres)) - &m_iRef);

		//lTotal++;
		//if (lTotal % lReportSize == 0) {
		//	Announce("%li%% complete", lTotal / lReportSize);
		//}

		s++;
	}
	}
	//AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

