# version 0.3 (in development)

# version 0.2.2 (2025-04-27)

### Enhancements

- Build: Avoid building documentation by default
- Python: Use GEOS 3.13 in binary distributions
- Python: Clear test warnings

# version 0.2.1 (2025-03-06)

### Enhancements

- CLI: Warn if inputs do not have the same CRS

### Fixes

- Python: Use pyproj to check CRS equality if GDAL not available
- Python: Correct formatting of `pyproject.toml`
- Python: Fix tests that fail with GDAL < 3.6
- Fix clang 16 warnings

# version 0.2 (2024-08-31)

### Breaking changes

- CLI now processes all bands of a multi-band raster if no band is specified.

### Enhancements

- Python bindings added
- Additional operations added: 
    - `cell_id`
    - `coverage`
    - `center_x`, `min_center_x`, `max_center_x`
    - `center_y`, `min_center_y`, `max_center_y`
    - `frac`, `weighted_frac`
    - `values`, `weights`
- CLI: Additional options added:
    - `--nested-output` option to write the results of operations that return
      multiple values as an array
    - `--include-col` and `--include-geom` to include column(s) from the input
      features in the result
- CLI: Specifying an input feature id column is no longer required

# version 0.1 (2023-09-12)

- initial numbered release
