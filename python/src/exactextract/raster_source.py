#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import pathlib
from typing import Optional, Tuple, Union
from osgeo import gdal

from _exactextract import RasterSource


class GDALRasterSource(RasterSource):
    def __init__(self, ds, band_idx: int = 1):
        super().__init__()
        self.ds = ds

        # Sanity check inputs
        if band_idx is not None and band_idx <= 0:
            raise ValueError("Raster band index starts from 1!")

        self.band = self.ds.GetRasterBand(band_idx)

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

    def read_window(self, x0, y0, nx, ny):
        return self.band.ReadAsArray(xoff=x0, yoff=y0, win_xsize=nx, win_ysize=ny)


class NumPyRasterSource(RasterSource):
    def __init__(self, mat, xmin=None, ymin=None, xmax=None, ymax=None):
        super().__init__()
        self.mat = mat

        assert (xmin is None) == (ymin is None) == (xmax is None) == (ymax is None)
        if xmin is None:
            self.ext = (0, 0, self.mat.shape[1], self.mat.shape[0])
        else:
            self.ext = (xmin, ymin, xmax, ymax)

    def res(self):
        ny, nx = self.mat.shape
        dy = (self.ext[3] - self.ext[1]) / ny
        dx = (self.ext[2] - self.ext[0]) / nx

        return (dx, dy)

    def extent(self):
        return self.ext

    def read_window(self, x0, y0, nx, ny):
        return self.mat[y0 : y0 + ny, x0 : x0 + ny]
