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
