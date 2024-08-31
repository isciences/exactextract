# version 0.3 (in development)

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
