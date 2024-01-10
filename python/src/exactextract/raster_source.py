#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os

import numpy as np
from _exactextract import RasterSource


class GDALRasterSource(RasterSource):
    def __init__(self, ds, band_idx: int = 1, *, name=None):
        super().__init__()
        if isinstance(ds, (str, os.PathLike)):
            from osgeo import gdal

            ds = gdal.Open(ds)
        self.ds = ds

        # Sanity check inputs
        if band_idx is not None and band_idx <= 0:
            raise ValueError("Raster band index starts from 1!")

        # Check for axis-aligned grid
        gt = self.ds.GetGeoTransform()
        if gt[2] != 0 or gt[4] != 0:
            raise ValueError("Rotated rasters are not supported.")

        self.band = self.ds.GetRasterBand(band_idx)

        if name:
            self.set_name(name)

    def res(self):
        gt = self.ds.GetGeoTransform()
        return gt[1], abs(gt[5])

    def extent(self):
        gt = self.ds.GetGeoTransform()

        dx, dy = self.res()

        left = gt[0]
        right = left + dx * self.ds.RasterXSize
        top = gt[3]
        bottom = gt[3] - dy * self.ds.RasterYSize

        return (left, bottom, right, top)

    def nodata_value(self):
        return self.band.GetNoDataValue()

    def read_window(self, x0, y0, nx, ny):
        arr = self.band.ReadAsArray(xoff=x0, yoff=y0, win_xsize=nx, win_ysize=ny)

        if self.band.GetScale() is not None:
            if issubclass(arr.dtype.type, np.integer):
                arr = arr.astype(np.float64)
            arr *= self.band.GetScale()

        if self.band.GetOffset() is not None:
            if issubclass(arr.dtype.type, np.integer):
                arr = arr.astype(np.float64)
            arr += self.band.GetOffset()

        return arr

    def srs_wkt(self):
        crs = self.ds.GetSpatialRef()
        if crs:
            return crs.ExportToWkt()


class NumPyRasterSource(RasterSource):
    def __init__(
        self,
        mat,
        xmin=None,
        ymin=None,
        xmax=None,
        ymax=None,
        *,
        nodata=None,
        name=None,
        srs_wkt=None
    ):
        super().__init__()
        self.mat = mat
        self.nodata = nodata

        assert (xmin is None) == (ymin is None) == (xmax is None) == (ymax is None)
        if xmin is None:
            self.ext = (0, 0, self.mat.shape[1], self.mat.shape[0])
        else:
            self.ext = (xmin, ymin, xmax, ymax)

        if name:
            self.set_name(name)

        self.srs_wkt_str = srs_wkt

    def res(self):
        ny, nx = self.mat.shape
        dy = (self.ext[3] - self.ext[1]) / ny
        dx = (self.ext[2] - self.ext[0]) / nx

        return (dx, dy)

    def srs_wkt(self):
        return self.srs_wkt_str

    def extent(self):
        return self.ext

    def nodata_value(self):
        return self.nodata

    def read_window(self, x0, y0, nx, ny):
        return self.mat[y0 : y0 + ny, x0 : x0 + ny]


class RasterioRasterSource(RasterSource):
    def __init__(self, ds, band_idx=1, *, name=None):
        super().__init__()
        if isinstance(ds, (str, os.PathLike)):
            import rasterio

            ds = rasterio.open(ds)

        self.ds = ds
        self.band_idx = band_idx

        gt = self.ds.get_transform()
        if gt[2] != 0 or gt[4] != 0:
            raise ValueError("Rotated rasters are not supported.")

        if name:
            self.set_name(name)

    def res(self):
        dx = (self.ds.bounds.right - self.ds.bounds.left) / self.ds.width
        dy = (self.ds.bounds.top - self.ds.bounds.bottom) / self.ds.height

        return (dx, dy)

    def srs_wkt(self):
        crs = self.ds.crs
        if crs:
            return crs.wkt

    def extent(self):
        return (
            self.ds.bounds.left,
            self.ds.bounds.bottom,
            self.ds.bounds.right,
            self.ds.bounds.top,
        )

    def nodata_value(self):
        return self.ds.nodata

    def read_window(self, x0, y0, nx, ny):
        from rasterio.windows import Window

        arr = self.ds.read(self.band_idx, window=Window(x0, y0, nx, ny))

        scale = self.ds.scales[self.band_idx - 1]
        offset = self.ds.offsets[self.band_idx - 1]

        scaled = scale != 1.0 or offset != 0.0

        if scaled:
            if issubclass(arr.dtype.type, np.integer):
                arr = arr.astype(np.float64)

            arr = arr * scale + offset

        return arr


class XArrayRasterSource(RasterSource):
    def __init__(self, ds, band_idx=1, *, name=None):
        super().__init__()

        if isinstance(ds, (str, os.PathLike)):
            import rioxarray  # noqa: F401
            import xarray

            ds = xarray.open_dataset(ds)
            ds = ds[next(iter(ds.keys()))]  # get first variable, for now

        self.ds = ds
        if self.ds.rio.crs is None:
            # Set a default CRS to prevent clip_box from
            # complaining that we don't have one
            self.ds.rio.set_crs("EPSG:4326", inplace=True)
        self.band_idx = band_idx
        self.band_dim = self._band_dim(self.ds)
        self.bounds = self.ds.rio.bounds()

        if name:
            self.set_name(name)

    @staticmethod
    def _band_dim(ds):
        dims = list(ds.dims)
        dims.remove(ds.rio.x_dim)
        dims.remove(ds.rio.y_dim)

        if len(dims) == 0:
            return None
        elif len(dims) == 1:
            return dims[0]
        else:
            raise Exception("Cannot handle >1 non-spatial dimension")

    def srs_wkt(self):
        crs = self.ds.rio.crs
        if crs:
            return crs.wkt

    def res(self):
        return tuple(abs(x) for x in self.ds.rio.resolution())

    def extent(self):
        return self.bounds

    def nodata_value(self):
        return self.ds.rio.nodata

    def read_window(self, x0, y0, nx, ny):
        if nx == 0 or ny == 0:
            return np.array([[]], dtype=self.ds.dtype)

        lats = self.ds[self.ds.rio.y_dim]
        flipped = bool(len(lats) > 1 and lats[1] > lats[0])

        if flipped:
            y0 = self.ds.rio.height - y0 - ny

        selection = {}
        if self.band_dim is not None:
            selection[self.band_dim] = self.ds[self.band_dim][self.band_idx - 1]
        selection[self.ds.rio.x_dim] = self.ds[self.ds.rio.x_dim][x0 : x0 + nx]
        selection[self.ds.rio.y_dim] = self.ds[self.ds.rio.y_dim][y0 : y0 + ny]

        ret = self.ds.sel(**selection).to_numpy()

        if flipped:
            ret = np.flipud(ret)

        return ret
