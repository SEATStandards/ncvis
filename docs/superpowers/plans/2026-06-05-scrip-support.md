# SCRIP File-Format Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Open SCRIP-convention NetCDF files (e.g., `EC30to60E2r2_smoothed.nc`) and render data variables on `grid_size` as filled unstructured maps using ncvis's existing samplers.

**Architecture:** Three localized changes in `src/wxNcVisFrame.cpp`:
1. Add `grid_center_lon` / `grid_center_lat` to the auto-detection lists in `OpenFiles`.
2. In `InitializeGridDataSampler`, read each coordinate variable's `units` attribute and convert radians → degrees in place when applicable.
3. In `LoadData`, after the 1D-unstructured branch, mask cells where a companion `grid_imask` variable equals 0.

No new files, no new samplers, no new build dependencies.

**Tech Stack:** C++11 · wxWidgets · the in-tree `netcdfcpp.h` C++ wrapper · `mpicxx` via `build.sh`

**Spec:** `docs/superpowers/specs/2026-06-05-scrip-support-design.md`

---

## Pre-flight notes for the implementing engineer

- **The repo has no automated test framework.** ncvis is a wxWidgets GUI app verified manually by opening a file and observing the rendered output. Each task's "test" therefore takes the form of `build → run binary on a specific file → visually confirm`. Do not introduce a test framework — that is explicitly out of scope.
- **Uncommitted local changes exist** on `src/GridDataSampler.cpp` and `build.sh`, plus untracked `build/`, `ncvis.dSYM/`, `.DS_Store`, `.omc/`. **Do not `git add -A`.** Stage files by exact path in every commit step.
- **Build invocation:** `./build.sh` from repo root. Produces the `ncvis` binary at repo root.
- **Run invocation:** `./ncvis <path-to-file.nc>`. The binary needs `resources/` co-located (already there) or `NCVIS_RESOURCE_DIR` set.
- **Primary test file:** `/Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc`.
- **NetCDF C++ API** (from `src/netcdfcpp.h`, already used throughout the codebase):
  - `var->get_att("name")` → `NcAtt*` (returns `NULL` when absent)
  - `att->as_string(0)` → `const char*` (the first string value)
  - `var->get(buf, n)` → reads `n` values starting at the current cursor
  - `var->get_dim(0)->size()` → dim length
  - `var->get_dim(0)->name()` → dim name

---

## File Structure

**Modified files (all in one source file):**
- `src/wxNcVisFrame.cpp` — three hunks:
  - hunk A: name-list extension near line 402–411 (in `OpenFiles`)
  - hunk B: units-autodetect block inside `InitializeGridDataSampler` (after lat/lon load, before bounds-snapping; around line 245 for the 1D branch and around line 297 for the multidim branch)
  - hunk C: grid_imask post-process at the end of the 1D-unstructured branch of `LoadData` (around line 937)

**New files (for verification):**
- `tests/data/scrip_radians_sample.nc` — synthetic SCRIP file in radians, ~24 cells.
- `tests/scripts/make_scrip_radians_sample.py` — Python script that produces it (committed for reproducibility).

---

## Task 1: Add SCRIP coordinate names to auto-detection lists

**Files:**
- Modify: `src/wxNcVisFrame.cpp:401-411`

- [ ] **Step 1: Identify the exact insertion site**

Open `src/wxNcVisFrame.cpp` and locate this block (around line 401–411 of the pre-change file):

```cpp
std::vector<std::string> vecCommonLonVarNames;
vecCommonLonVarNames.push_back("lon");
vecCommonLonVarNames.push_back("longitude");
vecCommonLonVarNames.push_back("lonCell");
vecCommonLonVarNames.push_back("mesh_node_x");

std::vector<std::string> vecCommonLatVarNames;
vecCommonLatVarNames.push_back("lat");
vecCommonLatVarNames.push_back("latitude");
vecCommonLatVarNames.push_back("latCell");
vecCommonLatVarNames.push_back("mesh_node_y");
```

- [ ] **Step 2: Add the two SCRIP names**

Replace the block with:

```cpp
std::vector<std::string> vecCommonLonVarNames;
vecCommonLonVarNames.push_back("lon");
vecCommonLonVarNames.push_back("longitude");
vecCommonLonVarNames.push_back("lonCell");
vecCommonLonVarNames.push_back("mesh_node_x");
vecCommonLonVarNames.push_back("grid_center_lon");

std::vector<std::string> vecCommonLatVarNames;
vecCommonLatVarNames.push_back("lat");
vecCommonLatVarNames.push_back("latitude");
vecCommonLatVarNames.push_back("latCell");
vecCommonLatVarNames.push_back("mesh_node_y");
vecCommonLatVarNames.push_back("grid_center_lat");
```

- [ ] **Step 3: Build**

Run from repo root:

```
./build.sh
```

Expected: compile completes with no errors; `ncvis` binary present at repo root with a new mtime.

- [ ] **Step 4: Manual verification — primary SCRIP file opens with coordinates detected**

Run:

```
./ncvis /Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc
```

Expected:
- The window opens without errors. The console line "Generating quadtree from lat/lon arrays" appears (proves `InitializeGridDataSampler` ran, which only happens once `m_strUnstructDimName` is populated).
- The variable dropdown for 1D variables contains `htopo`, `landfract`, `grid_imask` (and possibly `grid_area`).
- Selecting `htopo` renders a global topography-shaped image with continents visible (oceans appear as ~0; inactive cells will look spurious — that gets fixed in Task 3, do not worry now).
- Selecting `landfract` renders a global land-fraction image.

If the window opens but the variable renders as a 1D x-axis plot (a single horizontal strip), detection did NOT work — re-check that `grid_size` is being recorded as the unstructured dim (add a `std::cout` of `m_strDefaultUnstructDimName` after line 518 to diagnose; remove the debug print before committing).

- [ ] **Step 5: Commit**

```
git add src/wxNcVisFrame.cpp
git commit -m "Detect SCRIP grid_center_lon/lat as map coordinates"
```

Note: do NOT use `git add -A` — there are unrelated uncommitted edits on `build.sh` and `src/GridDataSampler.cpp` that must stay out of this commit.

---

## Task 2: Auto-convert radians to degrees in `InitializeGridDataSampler`

**Files:**
- Modify: `src/wxNcVisFrame.cpp` — inside `InitializeGridDataSampler` (function begins at line 204), two insertion sites (1D branch ~line 245, multidim branch ~line 297).

- [ ] **Step 1: Add a small helper at the top of `InitializeGridDataSampler`**

Find the function start:

```cpp
void wxNcVisFrame::InitializeGridDataSampler() {

	NcError error(NcError::silent_nonfatal);

	std::vector<double> dLon;
	std::vector<double> dLat;

	double dFillValue = std::numeric_limits<double>::max();
```

Insert a local lambda immediately after the local-variable declarations and before the first `if (m_strVarActiveMultidimLon == "") {` block. Replace the block above with:

```cpp
void wxNcVisFrame::InitializeGridDataSampler() {

	NcError error(NcError::silent_nonfatal);

	std::vector<double> dLon;
	std::vector<double> dLat;

	double dFillValue = std::numeric_limits<double>::max();

	// Local helper: if varCoord declares units of "radian"/"radians"/"rad"
	// (case-insensitive), convert every non-fill, non-NaN entry of vecCoord
	// from radians to degrees in place.  Coordinate variables that declare
	// degrees, or that have no "units" attribute, are left unchanged.
	auto convertRadiansToDegreesIfNeeded =
		[&dFillValue](NcVar * varCoord, std::vector<double> & vecCoord) {
			if (varCoord == NULL) {
				return;
			}
			NcAtt * attUnits = varCoord->get_att("units");
			if (attUnits == NULL) {
				return;
			}
			std::string strUnits = attUnits->as_string(0);
			std::string strLower;
			strLower.reserve(strUnits.size());
			for (size_t k = 0; k < strUnits.size(); k++) {
				char c = strUnits[k];
				if ((c >= 'A') && (c <= 'Z')) {
					c = static_cast<char>(c - 'A' + 'a');
				}
				strLower.push_back(c);
			}
			// Trim ASCII whitespace from both ends.
			size_t sBegin = 0;
			while ((sBegin < strLower.size()) &&
			       ((strLower[sBegin] == ' ') || (strLower[sBegin] == '\t'))) {
				sBegin++;
			}
			size_t sEnd = strLower.size();
			while ((sEnd > sBegin) &&
			       ((strLower[sEnd-1] == ' ') || (strLower[sEnd-1] == '\t'))) {
				sEnd--;
			}
			strLower = strLower.substr(sBegin, sEnd - sBegin);
			if ((strLower != "radian") &&
			    (strLower != "radians") &&
			    (strLower != "rad")) {
				return;
			}
			const double dRadToDeg = 180.0 / M_PI;
			for (size_t i = 0; i < vecCoord.size(); i++) {
				if (std::isnan(vecCoord[i])) {
					continue;
				}
				if (vecCoord[i] == dFillValue) {
					continue;
				}
				vecCoord[i] *= dRadToDeg;
			}
		};
```

- [ ] **Step 2: Call the helper in the 1D branch**

In the 1D branch of `InitializeGridDataSampler`, find this block (around line 240–246):

```cpp
		varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
		varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

		NcAtt * attFillValue = varLon->get_att("_FillValue");
		if (attFillValue != NULL) {
			dFillValue = attFillValue->as_double(0);
		}
```

Append two helper calls so the block becomes:

```cpp
		varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
		varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

		NcAtt * attFillValue = varLon->get_att("_FillValue");
		if (attFillValue != NULL) {
			dFillValue = attFillValue->as_double(0);
		}

		convertRadiansToDegreesIfNeeded(varLon, dLon);
		convertRadiansToDegreesIfNeeded(varLat, dLat);
```

- [ ] **Step 3: Call the helper in the multidim branch**

In the multidim branch of the same function, find the block ending with the two `varLon->gets` / `varLat->gets` calls (around lines 280–297):

```cpp
		varLon->set_cur(&(m_lVarActiveDims[0]));
		varLat->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == varLon->num_dims()-1) {
			varLon->get(&(dLon[0]), &(vecSize[0]));
			varLat->get(&(dLat[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(varLon->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < varLon->num_dims(); d++) {
				vecStride[d] = varLon->get_dim(d)->size();
			}

			varLon->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
			varLat->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
		}
	}
```

Append the two helper calls immediately after this block, right before the line `// Initialize the GridDataSampler`. The closing `}` shown above closes the `else` branch of `if (m_strVarActiveMultidimLon == "")`. Modify it to:

```cpp
		varLon->set_cur(&(m_lVarActiveDims[0]));
		varLat->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == varLon->num_dims()-1) {
			varLon->get(&(dLon[0]), &(vecSize[0]));
			varLat->get(&(dLat[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(varLon->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < varLon->num_dims(); d++) {
				vecStride[d] = varLon->get_dim(d)->size();
			}

			varLon->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
			varLat->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
		}

		convertRadiansToDegreesIfNeeded(varLon, dLon);
		convertRadiansToDegreesIfNeeded(varLat, dLat);
	}
```

- [ ] **Step 4: Build**

```
./build.sh
```

Expected: compile completes with no errors.

- [ ] **Step 5: Manual verification — degrees file still renders unchanged**

Run:

```
./ncvis /Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc
```

Expected: behavior identical to Task 1 Step 4. (`htopo`, `landfract` still render as global maps; this file is degrees so the new code path is not exercised — that comes in Task 4. This run only confirms the helper does not regress the degrees path.)

Also confirm bounds detection still snaps correctly: the longitude bounds text box should read close to `0` and `360` (or `-180` and `180`), the latitude bounds should read close to `-90` and `90`.

- [ ] **Step 6: Commit**

```
git add src/wxNcVisFrame.cpp
git commit -m "Autodetect radians vs degrees for lat/lon coordinates"
```

---

## Task 3: Honor `grid_imask` as a per-cell mask in `LoadData`

**Files:**
- Modify: `src/wxNcVisFrame.cpp:893-937` — append a post-process block at the end of the 1D-unstructured branch of `LoadData`.

- [ ] **Step 1: Identify the insertion site**

In `LoadData` (starts at line 893), the 1D branch ends at line 937 with this closing structure:

```cpp
		// Load data
		m_varActive->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == m_varActive->num_dims()-1) {
			m_varActive->get(&(m_data[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(m_varActive->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < m_varActive->num_dims(); d++) {
				vecStride[d] = m_varActive->get_dim(d)->size();
			}

			m_varActive->gets(&(m_data[0]), &(vecSize[0]), &(vecStride[0]));
		}

	// 2D data
	} else {
```

The new code goes just before `// 2D data`, inside the 1D-branch's closing `}`.

- [ ] **Step 2: Insert the imask post-process**

Replace the block above with:

```cpp
		// Load data
		m_varActive->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == m_varActive->num_dims()-1) {
			m_varActive->get(&(m_data[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(m_varActive->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < m_varActive->num_dims(); d++) {
				vecStride[d] = m_varActive->get_dim(d)->size();
			}

			m_varActive->gets(&(m_data[0]), &(vecSize[0]), &(vecStride[0]));
		}

		// SCRIP grid_imask post-process.  When the active variable is on the
		// unstructured cell-center dimension and a sibling 1D variable named
		// "grid_imask" exists on the same dimension, treat cells with
		// imask == 0 as missing data (SCRIP convention: cell active iff
		// imask != 0).
		if (m_fIsVarActiveUnstructured) {
			auto itImask = m_mapVarNames[1].find("grid_imask");
			if (itImask != m_mapVarNames[1].end()) {
				NcVar * varImask = NULL;
				for (size_t f = 0; f < itImask->second.size(); f++) {
					NcVar * varCandidate =
						m_vecpncfiles[itImask->second[f]]->get_var("grid_imask");
					if (varCandidate == NULL) {
						continue;
					}
					if (varCandidate->num_dims() != 1) {
						continue;
					}
					if (varCandidate->get_dim(0)->name() != m_strUnstructDimName) {
						continue;
					}
					if (static_cast<size_t>(varCandidate->get_dim(0)->size())
					    != m_data.size()) {
						continue;
					}
					varImask = varCandidate;
					break;
				}
				if (varImask != NULL) {
					std::vector<int> vecImask(m_data.size());
					varImask->get(&(vecImask[0]), varImask->get_dim(0)->size());
					if (!m_fDataHasMissingValue) {
						m_dMissingValueFloat =
							std::numeric_limits<float>::quiet_NaN();
						m_fDataHasMissingValue = true;
					}
					for (size_t i = 0; i < m_data.size(); i++) {
						if (vecImask[i] == 0) {
							m_data[i] = m_dMissingValueFloat;
						}
					}
				}
			}
		}

	// 2D data
	} else {
```

- [ ] **Step 3: Build**

```
./build.sh
```

Expected: compile completes with no errors.

- [ ] **Step 4: Manual verification — inactive cells render as missing**

Run:

```
./ncvis /Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc
```

Expected:
- Selecting `htopo`: same overall map as before. Any cells with `grid_imask == 0` now render as the colormap's "missing" color (typically transparent/grey, depending on the colormap) instead of as the colormap's value-for-zero.
- Selecting `landfract`: same global pattern as before, with inactive cells masked out.
- Selecting `grid_imask` itself: renders an indicator field (0/1).
- The dropdown that shows `_FillValue` / "missing" state should reflect that the variable now has missing values (visible by attempting export or by inspecting the range readouts — values of NaN will be excluded from auto-range).

If `htopo` looks identical to Task 1 (no change in masked cells' appearance), one of two things happened: either no cells in this file have `imask == 0`, or the post-process didn't run. Check by adding a one-shot `std::cout << "Applied grid_imask mask to " << countZeros << " cells\n";` after the for-loop to confirm; remove the debug print before committing.

- [ ] **Step 5: Commit**

```
git add src/wxNcVisFrame.cpp
git commit -m "Mask inactive SCRIP cells using grid_imask"
```

---

## Task 4: Create a synthetic SCRIP file in radians for §4.2 verification

**Files:**
- Create: `tests/scripts/make_scrip_radians_sample.py`
- Create: `tests/data/scrip_radians_sample.nc` (committed binary, ~5 KB, generated by the script above)

- [ ] **Step 1: Create the tests directory tree**

```
mkdir -p tests/scripts tests/data
```

- [ ] **Step 2: Write the generator script**

Create `tests/scripts/make_scrip_radians_sample.py` with the following content:

```python
"""Generate a tiny SCRIP-convention NetCDF file in radians for ncvis testing.

The file describes a 4x6 = 24-cell global lat/lon grid (logically structured
but emitted as unstructured rank-1 SCRIP). All coordinate values are in
radians; the units attribute is set to "radians" so ncvis must auto-convert.
A single data variable `band` is set to a smooth function of latitude so the
rendered map is visually distinctive.
"""

import math
import os

import netCDF4 as nc
import numpy as np


def main() -> None:
    n_lon = 6
    n_lat = 4
    n_cells = n_lon * n_lat

    # Cell centers on a regular lat/lon grid, in radians.
    lon_centers_deg = (np.arange(n_lon) + 0.5) * (360.0 / n_lon) - 180.0
    lat_centers_deg = (np.arange(n_lat) + 0.5) * (180.0 / n_lat) - 90.0
    lon2d, lat2d = np.meshgrid(lon_centers_deg, lat_centers_deg)
    centers_lon_rad = np.deg2rad(lon2d.ravel())
    centers_lat_rad = np.deg2rad(lat2d.ravel())

    # Cell corners (4 per cell) in radians, ordered CCW.
    dlon = (360.0 / n_lon) / 2.0
    dlat = (180.0 / n_lat) / 2.0
    corners_lon_rad = np.zeros((n_cells, 4), dtype=np.float64)
    corners_lat_rad = np.zeros((n_cells, 4), dtype=np.float64)
    for k in range(n_cells):
        lon_c = lon2d.ravel()[k]
        lat_c = lat2d.ravel()[k]
        corners_lon_rad[k] = np.deg2rad(
            [lon_c - dlon, lon_c + dlon, lon_c + dlon, lon_c - dlon])
        corners_lat_rad[k] = np.deg2rad(
            [lat_c - dlat, lat_c - dlat, lat_c + dlat, lat_c + dlat])

    # Mask: deactivate one cell to exercise grid_imask handling.
    imask = np.ones(n_cells, dtype=np.int32)
    imask[0] = 0

    # Data: smooth function of latitude so a band pattern is visible.
    band = np.sin(2.0 * centers_lat_rad).astype(np.float64)

    out_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "data")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "scrip_radians_sample.nc")

    with nc.Dataset(out_path, "w", format="NETCDF3_64BIT_OFFSET") as ds:
        ds.createDimension("grid_size", n_cells)
        ds.createDimension("grid_corners", 4)
        ds.createDimension("grid_rank", 1)

        v_clat = ds.createVariable(
            "grid_center_lat", "f8", ("grid_size",))
        v_clat.units = "radians"
        v_clat[:] = centers_lat_rad

        v_clon = ds.createVariable(
            "grid_center_lon", "f8", ("grid_size",))
        v_clon.units = "radians"
        v_clon[:] = centers_lon_rad

        v_xlat = ds.createVariable(
            "grid_corner_lat", "f8", ("grid_size", "grid_corners"))
        v_xlat.units = "radians"
        v_xlat[:] = corners_lat_rad

        v_xlon = ds.createVariable(
            "grid_corner_lon", "f8", ("grid_size", "grid_corners"))
        v_xlon.units = "radians"
        v_xlon[:] = corners_lon_rad

        v_imask = ds.createVariable("grid_imask", "i4", ("grid_size",))
        v_imask[:] = imask

        v_dims = ds.createVariable("grid_dims", "i4", ("grid_rank",))
        v_dims[:] = np.array([n_cells], dtype=np.int32)

        v_band = ds.createVariable("band", "f8", ("grid_size",))
        v_band[:] = band

    print(f"wrote {out_path} ({os.path.getsize(out_path)} bytes)")


if __name__ == "__main__":
    main()
```

- [ ] **Step 3: Run the generator**

The script needs `netCDF4` and `numpy`. If they are not already importable in the engineer's Python environment:

```
python3 -m pip install --user netCDF4 numpy
```

Then run:

```
python3 tests/scripts/make_scrip_radians_sample.py
```

Expected stdout (path may differ slightly):

```
wrote /Users/.../ncvis/tests/data/scrip_radians_sample.nc (XXXX bytes)
```

Confirm the file exists and is readable:

```
/opt/netcdf/bin/ncdump -h tests/data/scrip_radians_sample.nc
```

Expected: a SCRIP-shaped header where `grid_center_lat:units = "radians"` and the data variable `band(grid_size)` is present.

- [ ] **Step 4: Manual verification — radians file renders identically to its degrees twin**

Run:

```
./ncvis tests/data/scrip_radians_sample.nc
```

Expected:
- Window opens. Variable `band` is selectable.
- Selecting `band`: a smooth latitude-banded map appears (low values near the poles where `sin(2·lat) → 0`, peaks near ±45°). The map fills the full global extent — longitude bounds close to `(-180, 180)` and latitude bounds close to `(-90, 90)`. If the bounds show as small radian-valued numbers (e.g., `~-3.14` and `~3.14`), the conversion in Task 2 did NOT fire — re-check the `units` attribute path.
- One cell (cell index 0, near the south-west corner of the grid) renders as missing — the grid_imask handling from Task 3.

- [ ] **Step 5: Commit**

```
git add tests/scripts/make_scrip_radians_sample.py tests/data/scrip_radians_sample.nc
git commit -m "Add synthetic radians-units SCRIP sample for testing"
```

---

## Task 5: Regression smoke-test on existing files

**Files:** none modified.

- [ ] **Step 1: Open the in-repo ROMS file**

```
./ncvis roms_his_0_chesapeake.nc
```

Expected: opens as before the SCRIP changes — pick any 2D variable from the dropdown and confirm it renders. (This file uses structured lat/lon, so the unstructured codepath is untouched.)

- [ ] **Step 2: If a CESM-style or MPAS-style file is locally available, verify it too**

If the engineer has access to a file with `lon`/`lat` (CESM) or `lonCell`/`latCell` (MPAS), open it and confirm map rendering is unchanged. If not available, note the gap in the commit message of Task 4 or in a follow-up note — do not block on this.

- [ ] **Step 3: No commit needed unless a regression was found**

If everything works as expected, the regression test is complete. If a regression was found, file the symptom in the spec's "Implementation risks" table and stop — do not commit a broken pipeline.

---

## Self-review checklist (run after all five tasks are committed)

Run these checks on your final repo state:

```
git log --oneline -5
git status --short
```

Expected:
- Top of `git log`: five new commits matching the messages from Tasks 1, 2, 3, and 4 (Task 5 leaves no commit unless a regression was logged).
- `git status` shows the same uncommitted-from-the-start files (`build.sh`, `src/GridDataSampler.cpp`) plus the untracked dirs (`build/`, `ncvis.dSYM/`, `.omc/`, `.DS_Store`) — and nothing else. If anything else is uncommitted or untracked, investigate before declaring done.

Final manual checks:
- `EC30to60E2r2_smoothed.nc` (degrees, real SCRIP): opens, `htopo` and `landfract` render globally, inactive cells masked.
- `scrip_radians_sample.nc` (synthetic radians): opens, `band` renders globally with one masked cell.
- `roms_his_0_chesapeake.nc` (structured, regression): opens and renders unchanged.
