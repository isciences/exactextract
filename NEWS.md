# version 0.2 (in development)

### Breaking changes

- exactextract now emits placeholder statistic results for features that do not
  intersect the raster. If the input raster has a NODATA value it will be used,
  otherwise NaN will be written.

### Enhancements

- Python bindings added
- Additional operations added: 
    - `cell_id`
    - `coverage`
    - `center_x`, `min_center_x`, `max_center_x`
    - `center_y`, `min_center_y`, `max_center_y`
    - `frac`
    - `weighted_frac`
- CLI: Additional options added:
    - `--nested-output` option to write the results of operations that return
      multiple values as an array
    - `--include-col` and `--include-geom` to include column(s) from the input
      features in the result
- CLI: Specifying an input feature id column is no longer required

# version 0.1 (2023-09-12)

- initial numbered release
