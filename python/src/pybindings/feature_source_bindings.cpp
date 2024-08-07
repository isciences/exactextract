// Copyright (c) 2023-2024 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "feature.h"
#include "feature_source.h"
#include "feature_source_bindings.h"

namespace py = pybind11;

namespace exactextract {
class PyFeatureSourceBase : public FeatureSource
{
  public:
    PyFeatureSourceBase()
      : m_initialized(false)
    {
    }

    std::size_t count() const override
    {
        return py_count().cast<std::size_t>();
    }

    bool next() override
    {
        if (!m_initialized) {
            m_it = py_iter();
            m_initialized = true;
        }

        auto iter = py::iter(m_it);
        if (iter == py::iterator::sentinel()) {
            m_initialized = false;
            return false;
        }
        m_feature = py::cast<py::object>(*iter);
        return true;
    }

    const Feature& feature() const override
    {
        return m_feature.cast<const Feature&>();
    }

    virtual py::object py_iter() = 0;

    virtual py::object py_srs_wkt() const
    {
        return py::none();
    }

    virtual py::object py_count() const = 0;

  private:
    py::object m_src;
    py::object m_feature;
    py::iterator m_it;
    bool m_initialized;
};

class PyFeatureSource : public PyFeatureSourceBase
{

  public:
    using PyFeatureSourceBase::PyFeatureSourceBase;

    py::object py_count() const override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeatureSource, "count", py_count);
    }

    py::object py_iter() override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeatureSource, "__iter__", py_iter);
    }

    py::object py_srs_wkt() const override
    {
        PYBIND11_OVERRIDE_NAME(py::str, PyFeatureSourceBase, "srs_wkt", py_srs_wkt);
    }
};

void
bind_feature_source(py::module& m)
{
    py::class_<FeatureSource>(m, "FeatureSourceBase");

    py::class_<PyFeatureSourceBase>(m, "PyFeatureSourceBase");

    py::class_<PyFeatureSource, PyFeatureSourceBase, FeatureSource>(m, "FeatureSource")
      .def(py::init<>())
      .def("__iter__", &PyFeatureSourceBase::py_iter)
      .def("count", &PyFeatureSourceBase::py_count)
      .def("srs_wkt", &PyFeatureSourceBase::py_srs_wkt)
      // debug
      .def("feature", &PyFeatureSourceBase::feature);
}
}
