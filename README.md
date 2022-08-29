# ncvis
 NetCDF Visualizer

Developed by Paul A. Ullrich

Significant Contributions by Travis O'Brien and Vijay Mahadevan

Funding for the development of ncvis is provided by the United States Department of Energy Office of Science under the Regional and Global Model Analysis project "SEATS: Simplifying ESM Analysis Through Standards."

ncvis is inspired by David Pierce's most excellent ncview utility (http://meteora.ucsd.edu/~pierce/ncview_home_page.html)

## Configuration

In order for ncvis to run the `resources` directory must be in the same folder as `ncvis` or the `NCVIS_RESOURCE_DIR` environment variable must be set to the path of the ncvis resources folder. This directory is where ncvis stores fonts, colormaps and shapefiles.

On Unix of Linux-based systems, ncvis uses GDK (via wxWidgets) for rendering of the interface.  The font size displayed in GDK can be adjusted by setting the `GDK_DPI_SCALE` environment variable, e.g. via `export GDK_DPI_SCALE=0.5`.

## Colormaps

Some default colormaps from the [cmocean](https://github.com/matplotlib/cmocean) library are pre-installed in the `resources` subdirectory: files with the `.rgb` extension.  Additional colormaps can be added by adding new `.rgb` files to this directory.  The files must have 256 lines and three columns (separated by a single space) with each column corresponding to integer RGB values between 0-255. The python code used to generate the maps follows: 

```python   
import numpy as np
import cmocean
import matplotlib as mpl

# loop over colormaps
N = 256
for cmap in cmocean.cm.cmapnames:
    # get the colormap object
    cm = mpl.cm.get_cmap(f"cmo.{cmap}", N)
    # convert the colormap to an integer numpy array with values from 0-255
    color_table = np.around(cm.colors * 255,0).astype(int)[:,:3]
    # save the file to disk
    np.savetxt(f"{cmap}.rgb", color_table, fmt = "%i")
```

## Screenshot (v2022.08.28)

<img width="922" alt="ncvis_2022_08_28_era5_topo" src="https://user-images.githubusercontent.com/5330916/187129223-b9d47718-fff3-4fd9-8efb-4f71bd86d3e2.png">

## Screenshot (v2022.08.18)

<img width="842" alt="ncvis_screenshot_2022-08-18" src="https://user-images.githubusercontent.com/5330916/185477195-0381f475-10d4-4aa4-acdf-c352c87824b2.png">
