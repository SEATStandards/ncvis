# SCRIP file-format support in ncvis (centers-only)

**Date:** 2026-06-05
**Status:** Approved (design)
**Scope:** Minimal — read and visualize SCRIP-convention NetCDF files using the existing nearest-cell sampling pipeline. No new sampler, no polygon rasterization.

---

## 1. Motivation

SCRIP is the de-facto exchange format for unstructured climate-model grids (CESM, E3SM, ESMF). A representative example is
`/Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc`:

```
dimensions:
    grid_size = 15970 ;
    grid_corners = 7 ;
    grid_rank = 1 ;
variables:
    double grid_center_lat(grid_size) ;
    double grid_center_lon(grid_size) ;
    double grid_corner_lat(grid_size, grid_corners) ;
    double grid_corner_lon(grid_size, grid_corners) ;
    int    grid_imask(grid_size) ;
    int    grid_dims(grid_rank) ;
    double htopo(grid_size) ;
    double landfract(grid_size) ;
```

Today ncvis cannot open this file as a map because:

1. **Lat/lon auto-detection misses SCRIP names.** `OpenFiles` (wxNcVisFrame.cpp:401–411) matches only `lon | longitude | lonCell | mesh_node_x` (and the lat equivalents) or files with `standard_name`/`long_name` attributes set. SCRIP's `grid_center_lon` / `grid_center_lat` have neither.
2. **Units are not auto-converted.** SCRIP coordinates may be `degrees` (this file) or `radians` (also legal per the convention and common in practice). The downstream bounds-snapping logic in `InitializeGridDataSampler` (wxNcVisFrame.cpp:331–356) and the samplers (`LonDegToStandardRange`, etc.) are degree-based.
3. **`grid_imask` is ignored.** Inactive SCRIP cells (`grid_imask == 0`) would render as legitimate-looking zero values.

## 2. Goal

Open the example file with no command-line flags; render `htopo` and `landfract` as filled unstructured maps using the existing samplers.

## 3. Non-goals

- Polygon rasterization from `grid_corner_lon` / `grid_corner_lat`.
- Mesh-edge overlay drawn from `grid_corner_*`.
- Logical reshape of SCRIP rank-2 files (`grid_rank == 2`) into structured `i,j` grids — they will be handled as unstructured.
- Writing SCRIP output.
- A dedicated `-scrip` command-line flag.

## 4. Design

### 4.1 Lat/lon name detection

In `wxNcVisFrame::OpenFiles` (wxNcVisFrame.cpp:401–411), extend the existing common-name lists:

- `vecCommonLonVarNames` += `"grid_center_lon"`
- `vecCommonLatVarNames` += `"grid_center_lat"`

This is the minimum-delta change to the existing pattern. Detection then flows through the existing path that:
- Records `grid_size` as `m_strDefaultUnstructDimName` (wxNcVisFrame.cpp:514–518).
- Promotes it to `m_strUnstructDimName` (line 663).
- Causes the active variable to be flagged unstructured in `LoadData` when its sole dim matches (lines 913–914).

No other code paths in detection need to change.

### 4.2 Units autodetect (degrees ↔ radians)

In `InitializeGridDataSampler` (wxNcVisFrame.cpp:204), after reading `dLon` and `dLat` from the NetCDF variables (the 1D branch around lines 226–245 and the multidim branch around lines 261–297), inspect the `units` attribute on each coordinate variable.

Rule:
- If `units` (lowercased, trimmed) is one of `radian`, `radians`, `rad`, multiply every non-fill, non-NaN entry by `180.0 / M_PI` in place.
- Otherwise (units missing, `degrees`, `degrees_east`, `degree_north`, etc.), no conversion — preserves all current behavior.

Apply independently to lon and lat (mixed units are pathological but the per-variable decision costs nothing extra).

The conversion happens *before* the bounds-snapping logic at lines 331–356 so that snapping continues to work in degrees.

Data variables (e.g., `grid_area` in `radians^2`) are not coordinate variables and are untouched.

### 4.3 `grid_imask` masking

In `LoadData` (wxNcVisFrame.cpp:893+), after the 1D-unstructured branch finishes populating `m_data`:

If all of the following hold:
- `m_fIsVarActiveUnstructured == true`
- A variable named `grid_imask` exists in the same file as the active variable (look up via `m_mapVarNames[1]`).
- `grid_imask`'s single dimension name equals `m_strUnstructDimName`.
- `grid_imask`'s length equals `m_data.size()`.

Then:
- Read `grid_imask` (as `int`).
- If `m_fDataHasMissingValue` is false, set `m_dMissingValueFloat = std::numeric_limits<float>::quiet_NaN()` and `m_fDataHasMissingValue = true`. NaN is already treated as missing by the rendering pipeline (wxNcVisFrame.cpp:1399, 1423, 1432).
- For every `i` where `grid_imask[i] == 0`, set `m_data[i] = m_dMissingValueFloat`.

If `grid_imask` is absent, no behavior change.

This is the documented SCRIP semantics: a cell is active iff `imask != 0`.

### 4.4 Nothing else

The existing unstructured pipeline — sampler initialization, image-panel rendering, mouse-hover readout, PNG export — needs no SCRIP-specific changes once §4.1–§4.3 have run.

## 5. Verification plan

### 5.1 Primary file under test

`/Users/mahadevan/Code/sigma/topography-tool/EC30to60E2r2_smoothed.nc` — open with no flags. Expect:
- `htopo` and `landfract` selectable, render as filled global unstructured maps.
- Bounds snap to `(−180, 180) × (−90, 90)` (the file is degrees, global).
- Inactive cells (if any have `grid_imask == 0`) render as missing.

### 5.2 Radians-units path

The primary file declares `units = "degrees"` and therefore does **not** exercise the radians conversion. To verify §4.2 end-to-end, build a small synthetic SCRIP file (Python + xarray, ~30 lines: one ring of cells, `units = "radians"`, a single data variable) and confirm:
- Detected as global (`(−π, π)` becomes `(−180, 180)` after conversion, then snaps).
- Rendered map looks identical to a degrees-units equivalent.

The synthetic file should live under `tests/data/` or equivalent (location to be picked during implementation).

### 5.3 Regression files

- One CESM-style file with `lon` / `lat` 1D coordinates (degrees, global) — confirm no behavior change.
- One MPAS-style file with `lonCell` / `latCell` if available — confirm no behavior change.

If only one of these is locally available, document the gap.

### 5.4 Acceptance criteria

- Example SCRIP file opens, both data variables render as recognizable global topography / land-fraction maps.
- Synthetic radians SCRIP file renders correctly.
- All previously working files continue to work (manual smoke test).

## 6. Implementation risks

| Risk | Mitigation |
|---|---|
| `grid_corner_lat` / `grid_corner_lon` and `grid_imask` themselves are 1D or 2D variables that could be picked up as data variables in the variable dropdown, cluttering the UI. | Acceptable for v1 — they appear in the dropdown but render meaningfully (corner arrays as 2D, imask as a 0/1 map). Document but do not filter. |
| A SCRIP file with `grid_rank == 2` (logically structured) treated as unstructured looks the same to the user as rank-1. | Acceptable — the centers-only path produces a correct map either way. Reshape support is explicitly out of scope. |
| Some SCRIP producers write `units = "radian"` (singular), `"rad"`, or even an unknown spelling. | The radians check accepts `radian`, `radians`, `rad`. Anything else falls through as "not radians" (existing behavior — usually fine if values are in `[−180, 180]`). |
| A file mixes a radians `grid_center_lon` with a degrees `grid_center_lat` (or vice versa). | Handled — each is converted independently based on its own `units` attribute. |

## 7. Out-of-scope follow-ups (for future work)

- A `GridDataSamplerUsingPolygons` that rasterizes each cell from its corners. Most accurate, large change.
- A togglable "mesh edges" overlay built from `grid_corner_*` (similar shape to the existing shapefile overlay).
- Detection / reshape of `grid_rank == 2` SCRIP files into structured grids.
