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
	const std::vector<double> & dLon,
	const std::vector<double> & dLat
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
	const std::vector<double> & dLon,
	const std::vector<double> & dLat,
	double dFillValue
) {
	GridDataSampler::Initialize(dLon, dLat);

	_ASSERT(dLon.size() == dLat.size());

	for (int i = 0; i < m_vecquadtree.size(); i++) {
		m_vecquadtree[i].clear();
	}
	m_vecquadtree.clear();

	AnnounceStartBlock("Generating quadtree from lat/lon arrays");

	m_vecquadtree.resize(6, QuadTreeNode(-0.5*M_PI, 0.5*M_PI, -0.5*M_PI, 0.5*M_PI));

	m_fDistanceFilter = false;
	int iMaxLevel = 0;

	long iReportSize = static_cast<long>(dLon.size()) / 100;
	for (long i = 0; i < dLon.size(); i++) {

		if ((dLon[i] != dFillValue) && (dLat[i] != dFillValue)) {

			double dA;
			double dB;
			int nP;

			ABPFromRLL(dLon[i], dLat[i], dA, dB, nP);

			_ASSERT((nP >= 0) && (nP <= 5));

			int iLevel = m_vecquadtree[nP].insert(dA, dB, i);
			if (iLevel > iMaxLevel) {
				iMaxLevel = iLevel;
			}

		} else {
			m_fDistanceFilter = true;
		}

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	m_dMaxDist = M_PI * pow(0.5, static_cast<double>(iMaxLevel));

	if (m_fDistanceFilter) {
		Announce("Maximum render distance: %1.5e (%i)", m_dMaxDist, iMaxLevel);
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingCubedSphereQuadTree::Sample(
	const std::vector<double> & dSampleLon,
	const std::vector<double> & dSampleLat,
	std::vector<int> & dImageMap
) const {
	dImageMap.resize(dSampleLon.size() * dSampleLat.size());

	int s = 0;
	for (int j = 0; j < dSampleLat.size(); j++) {
	for (int i = 0; i < dSampleLon.size(); i++) {

		double dA;
		double dB;
		int nP;

		double dAref;
		double dBref;

		ABPFromRLL(dSampleLon[i], dSampleLat[j], dA, dB, nP);

		_ASSERT((nP >= 0) && (nP <= 5));

		size_t sI = m_vecquadtree[nP].find_inexact(dA, dB, dAref, dBref);

		if (m_fDistanceFilter) {
			if (fabs(dA - dAref) > m_dMaxDist) {
				sI = static_cast<size_t>(-1);
			}
			if (fabs(dB - dBref) > m_dMaxDist) {
				sI = static_cast<size_t>(-1);
			}
		}

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

void GridDataSamplerUsingQuadTree::SetRegionalBounds(
	double dLonBounds0,
	double dLonBounds1,
	double dLatBounds0,
	double dLatBounds1
) {
	m_quadtree.clear();
	m_quadtree = QuadTreeNode(dLonBounds0, dLonBounds1, dLatBounds0, dLatBounds1);
	m_fRegional = true;
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Initialize(
	const std::vector<double> & dLon,
	const std::vector<double> & dLat,
	double dFillValue
) {
	GridDataSampler::Initialize(dLon, dLat);

	_ASSERT(dLon.size() == dLat.size());

	m_quadtree.clear();

	AnnounceStartBlock("Generating quadtree from lat/lon arrays");

	m_fDistanceFilter = false;
	int iMaxLevel = 0;

	long iReportSize = static_cast<long>(dLon.size()) / 100;
	for (long i = 0; i < dLon.size(); i++) {

		if ((dLon[i] != dFillValue) && (dLat[i] != dFillValue)) {
			double dStandardLonDeg;
			if (!m_fRegional) {
				dStandardLonDeg = LonDegToStandardRange(dLon[i]);
			} else {
				dStandardLonDeg = dLon[i];
			}

			int iLevel = m_quadtree.insert(dStandardLonDeg, dLat[i], i);
			if (iLevel > iMaxLevel) {
				iMaxLevel = iLevel;
			}

		} else {
			m_fDistanceFilter = true;
		}

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	m_dMaxDist = 2.0 * 360.0 * pow(0.5, static_cast<double>(iMaxLevel));

	if (m_fDistanceFilter) {
		if (m_fRegional) {
			_EXCEPTIONT("DistanceFilter cannot be combined with regional bounds");
		}
		Announce("Maximum render distance: %1.5e (%i)", m_dMaxDist, iMaxLevel);
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Sample(
	const std::vector<double> & dSampleLon,
	const std::vector<double> & dSampleLat,
	std::vector<int> & dImageMap
) const {
	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.size()) * static_cast<long>(dSampleLon.size()) / 100;
	AnnounceStartBlock("Querying data points within the quadtree");

	dImageMap.resize(dSampleLon.size() * dSampleLat.size());

	double dLonRef;
	double dLatRef;

	double dMaxDeltaLon = 2.0 * fabs(dSampleLon[1] - dSampleLon[0]);
	double dMaxDeltaLat = 2.0 * fabs(dSampleLat[1] - dSampleLat[0]);

	int s = 0;
	for (int j = 0; j < dSampleLat.size(); j++) {
	for (int i = 0; i < dSampleLon.size(); i++) {
		double dSampleStandardLonDeg;
		if (!m_fRegional) {
			dSampleStandardLonDeg = LonDegToStandardRange(dSampleLon[i]);
		} else {
			dSampleStandardLonDeg = dSampleLon[i];
		}

		size_t sI =
			m_quadtree.find_inexact(
				dSampleStandardLonDeg,
				dSampleLat[j],
				dLonRef,
				dLatRef);

		if (m_fDistanceFilter) {
			double dDeltaLon = LonDegToStandardRange(dSampleLon[i] - dLonRef);
			if (dDeltaLon > 180.0) {
				dDeltaLon -= 360.0;
			}

			if (fabs(dDeltaLon * cos(dSampleLat[j] / 180.0 * M_PI)) > m_dMaxDist) {
				sI = static_cast<size_t>(-1);
			}
			if (fabs(dSampleLat[j] - dLatRef) > m_dMaxDist) {
				sI = static_cast<size_t>(-1);
			}
		}

		if (sI == static_cast<size_t>(-1)) {
			dImageMap[s] = 0;
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
	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////
// GridDataSamplerUsingKDTree
///////////////////////////////////////////////////////////////////////////////

GridDataSamplerUsingKDTree::~GridDataSamplerUsingKDTree() {
	if (m_kdtree != NULL) {
		kd_free(m_kdtree);
	}
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Initialize(
	const std::vector<double> & dLon,
	const std::vector<double> & dLat,
	double dFillValue
) {
	GridDataSampler::Initialize(dLon, dLat);

	if (m_kdtree != NULL) {
		kd_free(m_kdtree);
	}

	AnnounceStartBlock("Generating kdtree from from lat/lon arrays");

	m_kdtree = kd_create(3);
	if (m_kdtree == NULL) {
		_EXCEPTIONT("kd_create(3) failed");
	}

	long iReportSize = static_cast<long>(dLon.size()) / 100;
	for (long i = 0; i < static_cast<long>(dLon.size()); i++) {

		if ((dLon[i] != dFillValue) && (dLat[i] != dFillValue)) {

			double dX;
			double dY;
			double dZ;

			RLLtoXYZ_Deg(dLon[i], dLat[i], dX, dY, dZ);

			kd_insert3(m_kdtree, dX, dY, dZ, (void*)(&m_iRef + i));
		}

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Sample(
	const std::vector<double> & dSampleLon,
	const std::vector<double> & dSampleLat,
	std::vector<int> & dImageMap
) const {

	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.size()) * static_cast<long>(dSampleLon.size()) / 100;
	//AnnounceStartBlock("Querying data points within the kdtree");

	dImageMap.resize(dSampleLon.size() * dSampleLat.size());

	int s = 0;
	for (int j = 0; j < dSampleLat.size(); j++) {
	for (int i = 0; i < dSampleLon.size(); i++) {

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

