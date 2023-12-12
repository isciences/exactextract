#include "grid.h"
#include "raster.h"
#include "raster_source.h"

#pragma once

namespace exactextract {

template<typename ValueType, typename WeightType>
class RasterCoverageIteration;

template<typename ValueType, typename WeightType>
struct CoverageValue
{
    float coverage;
    double x;
    double y;
    std::size_t cell;
    ValueType value;
    WeightType weight;
    double area;
};

template<typename ValueType = double, typename WeightType = double>
class RasterCoverageIterator
{

  public:
    const CoverageValue<ValueType, WeightType>& operator*() const
    {
        return m_loc;
    }

    RasterCoverageIterator& operator++()
    {
        increment();
        return *this;
    }

    RasterCoverageIterator operator++(int)
    {
        RasterCoverageIterator ret = *this;
        increment();
        return ret;
    }

    bool operator==(const RasterCoverageIterator& other)
    {
        return m_i == other.m_i && m_j == other.m_j;
    }

    bool operator!=(const RasterCoverageIterator& other)
    {
        return !(*this == other);
    }

  private:
    RasterCoverageIterator(const AbstractRaster<float>& coverage,
                           RasterView<ValueType>* values,
                           RasterView<WeightType>* weights,
                           const Grid<bounded_extent>& common_grid,
                           const AbstractRaster<double>* areas,
                           std::size_t i,
                           std::size_t j)
      : m_coverage(coverage)
      , m_cov_grid(coverage.grid())
      , m_common_grid(common_grid)
      , m_values_view(values)
      , m_weights_view(weights)
      , m_areas(areas)
      , m_i(i)
      , m_j(j)
    {
        m_loc.x = m_cov_grid.x_for_col(m_i);
        m_loc.y = m_cov_grid.y_for_row(m_j);

        if (m_i == 0 && m_j == 0) {
            m_loc.coverage = m_coverage(m_i, m_j);
            set_values();
        }
    }

    void set_values()
    {

        if (m_values_view) {
            if (!m_values_view->get(m_i, m_j, m_loc.value)) {
                // TODO handle NODATA
            }
        }

        if (m_weights_view) {
            if (!m_weights_view->get(m_i, m_j, m_loc.weight)) {
                // TODO handle NODATA
            }
        }

        if (m_areas) {
            m_areas->get(m_i, m_j, m_loc.area);
        }

        if (m_include_cell) {
            m_loc.cell = m_common_grid.get_cell(m_loc.x, m_loc.y);
        }
    }

    void increment()
    {
        bool found_coverage = false;

        while (!found_coverage) {
            if (++m_j == m_coverage.cols()) {
                m_j = 0;
                m_i++;
                m_loc.y = m_cov_grid.y_for_row(m_i);
            }

            if (m_i == m_coverage.rows()) {
                return;
            }

            m_loc.coverage = m_coverage(m_i, m_j);
            m_loc.x = m_cov_grid.x_for_col(m_j);

            found_coverage = true;
        }

        set_values();
    }

  private:
    const AbstractRaster<float>& m_coverage;
    Grid<bounded_extent> m_cov_grid;

    Grid<bounded_extent> m_common_grid;

    RasterView<ValueType>* m_values_view;
    RasterView<WeightType>* m_weights_view;
    const AbstractRaster<double>* m_areas;

    std::size_t m_i;
    std::size_t m_j;

    CoverageValue<ValueType, WeightType> m_loc;

    bool m_include_cell = true;

    friend class RasterCoverageIteration<ValueType, WeightType>;
};

template<typename ValueType, typename WeightType>
class RasterCoverageIteration
{

  public:
    using Iterator = RasterCoverageIterator<ValueType, WeightType>;

    RasterCoverageIteration(const AbstractRaster<float>& coverage,
                            RasterSource* values,
                            RasterSource* weights,
                            const Grid<bounded_extent>& common_grid,
                            const AbstractRaster<double>* areas)
      : m_coverage(coverage)
      , m_common_grid(common_grid)
      , m_areas(areas)
    {
        read_raster_data(values, weights);
    }

    Iterator begin() const
    {
        return Iterator(m_coverage, m_values_view.get(), m_weights_view.get(), m_common_grid, m_areas, 0, 0);
    }

    Iterator end() const
    {
        return Iterator(m_coverage, m_values_view.get(), m_weights_view.get(), m_common_grid, m_areas, m_coverage.rows(), 0);
    }

  private:
    void read_raster_data(RasterSource* values_src, RasterSource* weights_src)
    {
        if (values_src) {
            m_values = std::get<std::unique_ptr<AbstractRaster<ValueType>>>(values_src->read_box(m_coverage.grid().extent()));
            m_values_view = std::make_unique<RasterView<ValueType>>(*m_values, m_coverage.grid());
        }

        if (weights_src) {
            m_weights = std::get<std::unique_ptr<AbstractRaster<WeightType>>>(weights_src->read_box(m_coverage.grid().extent()));
            m_weights_view = std::make_unique<RasterView<WeightType>>(*m_weights, m_coverage.grid());
        }
    }

    const AbstractRaster<float>& m_coverage;

    const Grid<bounded_extent>& m_common_grid;

    const AbstractRaster<double>* m_areas;

    std::unique_ptr<AbstractRaster<ValueType>> m_values;
    std::unique_ptr<AbstractRaster<WeightType>> m_weights;

    std::unique_ptr<RasterView<ValueType>> m_values_view;
    std::unique_ptr<RasterView<WeightType>> m_weights_view;
};

}
