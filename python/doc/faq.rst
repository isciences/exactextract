FAQ
===

How can I fix an "incompatible extents" error?
----------------------------------------------

When calculating weighted statistics using two rasters, ``exactextract`` accepts the extent of two rasters as equivalent if the origin points are within a specified tolerance of a pixel. This tolerance can be increased using a ``grid_compat_tol`` argument. A more robust solution would be to resample the rasters to a common grid using a tool such as ``gdalwarp``. The GDAL VRT format allows this resampling to be expressed as an XML file, without actually materializing the resampled raster to disk.
