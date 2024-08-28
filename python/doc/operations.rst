Available Operations
====================

``exactextract`` provides numerous built-in operations for summarizing gridded data.
This page provides a list of the :ref:`operations` and :ref:`arguments tuning their behavior <operation_arguments>`.
If a built-in operation is not suitable, custom operations can be defined when using eithe the Python or R bindings.

.. note::

   All of the operations listed in this page are supported in the command-line interface and Python bindings.
   Some operations are not currently available in R.

.. _operations:

Built-in operations
-------------------

  * :math:`x_i` represents the value of the *ith* raster cell,
  * :math:`c_i` represents the fraction of the *ith* raster cell that is covered by the polygon (unless directed otherwise by the ``coverage_weight`` argument), and
  * :math:`w_i` represents the weight of the *ith* raster cell.

Values in the "example result" column refer to the value and weighting rasters shown below.
In these images, values of the "value raster" range from 1 to 4, and values of the "weighting raster" range from 5 to 8.
The area covered by the polygon is shaded purple.

.. list-table::
    :header-rows: 1

    * - Example Value Raster
      - Example Weighting Raster
    * - .. image:: /_static/operations/readme_example_values.svg
           :width: 200
           :height: 200
      - .. image:: /_static/operations/readme_example_weights.svg
           :width: 200
           :height: 200

.. list-table::
    :header-rows: 1

    * - Name
      - Formula
      - Description
      - Typical Application
      - Example Result

    * - cell_id
      -
      - Array with 0-based index of each cell that intersects the polygon, increasing left-to-right.
      -
      - ``[ 0, 2, 3]``
    * - center_x
      -
      - Array with cell center x-coordinate for each cell that intersects the polygon. Each cell center 
        may or may not be inside the polygon.
      - 
      - ``[ 0.5, 0.5, 1.5 ]``
    * - center_y       
      -                                                                                    
      - Array with cell center y-coordinate for each cell that intersects the polygon. Each cell center may or may not be inside the polygon. 
      - 
      - ``[ 1.5, 0.5, 0.5 ]`` 
    * - coefficient_of_variation 
      - stdev / mean                                                               
      - Population coefficient of variation of cell values that intersect the polygon, taking into account coverage fraction. 
      - 
      - ``0.41``
    * - count          
      - :math:`\Sigma{c_i}`                                                                 
      - Sum of all cell coverage fractions. 
      - 
      - ``0.5 + 0 + 1 + 0.25 = 1.75`` 
    * - coverage       
      - 
      - Array with coverage fraction of each cell that intersects the polygon 
      - 
      - ``[ 0.5, 1.0, 0.25 ]``
    * - frac           
      -                                                                                    
      - Fraction of covered cells that are occupied by each distinct raster value. 
      - Land cover summary 
      - 
    * - majority       
      -                                                                                    
      - The raster value occupying the greatest number of cells, taking into account cell coverage fractions but not weighting raster values. 
      - Most common land cover type 
      - ``3`` 
    * - max            
      -                                                                                    
      - Maximum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account.  
      - Maximum temperature 
      - ``4`` 
    * - max_center_x   
      - 
      - Cell center x-coordinate for the cell containing the maximum value intersected by the polygon. The center of this cell may or may not be inside the polygon. 
      - Highest point in watershed 
      - ``1.5`` 
    * - max_center_y   
      - 
      - Cell center y-coordinate for the cell containing the maximum value intersected by the polygon. The center of this cell may or may not be inside the polygon. 
      - Highest point in watershed 
      - ``0.5`` 
    * - mean           
      - :math:`\frac{\Sigma{x_ic_i}}{\Sigma{c_i}}`                           
      - Mean value of cells that intersect the polygon, weighted by the percent of each cell that is covered. 
      - Average temperature 
      - ``4.5/1.75 = 2.57`` 
    * - median         
      -                                                                                      
      - Median value of cells that intersect the polygon, weighted by the percent of each cell that is covered 
      - Average temperature 
      - ``2`` 
    * - min            
      -                                                                                    
      - Minimum value of cells that intersect the polygon, not taking coverage fractions or weighting raster values into account. 
      - Minimum elevation 
      - ``1`` 
    * - min_center_x   
      - 
      - Cell center x-coordinate for the cell containing the minimum value intersected by the polygon. The center of this cell may or may not be inside the polygon. 
      - Lowest point in watershed 
      - ``0.5`` 
    * - min_center_y   
      - 
      - Cell center y-coordinate for the cell containing the minimum value intersected by the polygon. The center of this cell may or may not be inside the polygon. 
      - Lowest point in watershed 
      - ``1.5`` 
    * - minority       
      -                                                                                    
      - The raster value occupying the least number of cells, taking into account cell coverage fractions but not weighting raster values. 
      - Least common land cover type 
      - ``4`` 
    * - quantile       
      - 
      - Specified quantile of cells that intersect the polygon, weighted by the percent of each cell that is covered. Quantile value is specified by the `q` argument, 0 to 1. 
      - 
      -
    * - stdev          
      - :math:`\sqrt{\frac{\Sigma{c_i}(x_i - \bar{x})^{2}}{\Sigma{c_i}}}`                                                                  
      - Population standard deviation of cell values that intersect the polygon, taking into account coverage fraction. 
      - 
      - ``1.05`` 
    * - sum            
      - :math:`\Sigma{x_ic_i}`                                                 
      - Sum of values of raster cells that intersect the polygon, with each raster value weighted by its coverage fraction. 
      - Total population 
      - ``0.5*1 + 0&2 + 1.0*3 + 0.25*4 = 4.5`` 
    * - unique         
      -                                                                                      
      - Array of unique raster values for cells that intersect the polygon 
      - 
      - ``[ 1, 3, 4 ]`` 
    * - values         
      -                                                                                      
      - Array of raster values for each cell that intersects the polygon 
      - 
      - ``[ 1, 3, 4 ]`` 
    * - variance       
      - :math:`\frac{\Sigma{c_i}(x_i - \bar{x})^{2}}{\Sigma{c_i}}`   
      - Population variance of cell values that intersect the polygon, taking into account coverage fraction. 
      - 
      - ``1.10``
    * - variety        
      -                                                                                    
      - The number of distinct raster values in cells wholly or partially covered by the polygon. 
      - Number of land cover types 
      - ``3``
    * - weighted_frac  
      -                                                                                    
      - Fraction of covered cells that are occupied by each distinct raster value, weighted by the value of a second weighting raster. 
      - Population-weighted land cover summary 
      - 
    * - weighted_mean  
      - :math:`\frac{\Sigma{x_ic_iwi}}{\Sigma{c_iw_i}}`
      - Mean value of cells that intersect the polygon, weighted by the product over the coverage fraction and the weighting raster. 
      - Population-weighted average temperature 
      - ``31.5 / (0.5*5 + 0*6 + 1.0*7 + 0.25*8) = 2.74``
    * - weighted_stdev 
      -                                                                                      
      - Weighted version of `stdev`. 
      - 
      -
    * - weighted_sum   
      - :math:`\Sigma{x_ic_iw_i}`                                       
      - Sum of raster cells covered by the polygon, with each raster value weighted by its coverage fraction and weighting raster value. 
      - Total crop production lost 
      - ``0.5*1*5 + 0*2*6 + 1.0*3*7 + 0.25*4*8 = 31.5``
    * - weighted_variance 
      -                                                                                   
      - Weighted version of `variance` 
      - 
      -
    * - weights        
      -                                                                                      
      - Array of weight values for each cell that intersects the polygon 
      - 
      - ``[ 5, 7, 8 ]`` 

.. _operation_arguments:

Operation arguments
-------------------

The behavior of these statistics may be modified by the following arguments:

  * ``coverage_weight`` - specifies the value to use for :math:`c_i` in the above formulas. The following methods are available:
     - `fraction` - the default method, with :math:`c_i` ranging from 0 to 1
     - `none` - :math:`c_i` is always equal to 1; all pixels are given the same weight in the above calculations regardless of their coverage fraction
     - `area_cartesian` - :math:`c_i` is the fraction of the pixel multiplied by it x and y resolutions
     - `area_spherical_m2` - :math:`c_i` is the fraction of the pixel that is covered multiplied by a spherical approximation of the cell's area in square meters
     - `area_spherical_km2` - :math:`c_i` is the fraction of the pixel that is covered multiplied by a spherical approximation of the cell's area in square kilometers
  * ``default_value`` - specifies a value to be used for NODATA pixels instead of ignoring them
  * ``default_weight`` - specifies a weighing value to be used for NODATA pixels instead of ignoring them
  * ``min_coverage_frac`` - specifies the minimum fraction of the pixel (0 to 1) that must be covered in order for a pixel to be considered in calculations. Defaults to 0.
