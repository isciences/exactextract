// Copyright (c) 2023 ISciences, LLC.
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
#include "feature_bindings.h"

namespace py = pybind11;

namespace exactextract {
class PyFeatureBase : public Feature
{
  public:
    PyFeatureBase()
    {
        m_context = initGEOS_r(nullptr, nullptr);
    }

    virtual ~PyFeatureBase()
    {
        if (m_geom != nullptr) {
            GEOSGeom_destroy_r(m_context, m_geom);
        }
    }

    // plumbing to connect python-specifc abstract methods
    // to Feature abstract methods

    float get_float(const std::string& name) const override
    {
        return get_py(name).cast<float>();
    }

    double get_double(const std::string& name) const override
    {
        return get_py(name).cast<double>();
    }

    std::int32_t get_int(const std::string& name) const override
    {
        return get_py(name).cast<int32_t>();
    }

    std::string get_string(const std::string& name) const override
    {
        return get_py(name).cast<std::string>();
    }

    void set(const std::string& name, std::size_t value) override
    {
        // TODO handle value > max python int
        set_py(name, py::int_(value));
    }

    void set(const std::string& name, std::string value) override
    {
        set_py(name, py::str(value));
    }

    void set(const std::string& name, float value) override
    {
        set_py(name, py::float_(value));
    }

    void set(const std::string& name, double value) override
    {
        set_py(name, py::float_(value));
    }

    void set(const std::string& name, std::int32_t value) override
    {
        set_py(name, py::int_(value));
    }

    const std::type_info& field_type(const std::string& name) const override
    {
        py::object value = get_py(name);

        if (py::isinstance<py::int_>(value)) {
            return typeid(int);
        }

        if (py::isinstance<py::float_>(value)) {
            return typeid(double);
        }

        if (py::isinstance<py::str>(value)) {
            return typeid(std::string);
        }

        throw std::runtime_error("Unhandled type for field " + name + " in PyFeatureBase::field_type");
    }

    const GEOSGeometry* geometry() const override
    {
        if (m_geom == nullptr) {
            py::object geom = geometry_py();
            if (py::isinstance<py::bytes>(geom)) {
                py::buffer_info info = py::buffer(geom).request();
                m_geom = GEOSGeomFromWKB_buf_r(m_context,
                                               (const unsigned char*)info.ptr,
                                               info.shape[0]);
            } else if (py::isinstance<py::str>(geom)) {
                std::string str = geom.cast<std::string>();
                m_geom = GEOSGeomFromWKT_r(m_context, str.c_str());

                if (m_geom == nullptr) {
                    auto reader = GEOSGeoJSONReader_create_r(m_context);
                    m_geom = GEOSGeoJSONReader_readGeometry_r(m_context, reader, str.c_str());
                    GEOSGeoJSONReader_destroy_r(m_context, reader);
                }
            }

            if (m_geom == nullptr) {
                throw std::runtime_error("Failed to parse geometry.");
            }
        }

        return m_geom;
    }

    void copy_to(Feature& other) const override
    {
        py::iterable field_list = fields();
        for (py::handle field : field_list) {
            std::string field_name = field.cast<std::string>();

            other.set(field_name, *this);
        }

        // TODO copy geometry
    }

    // abstract methods to be implemented in Python class

    virtual py::object get_py(const std::string& name) const = 0;

    virtual void set_py(const std::string& name, py::object value) = 0;

    virtual py::object geometry_py() const = 0;

    virtual py::object fields() const = 0;

    // debugging

    bool check()
    {
        return geometry() != nullptr;
    }

  private:
    GEOSContextHandle_t m_context;
    mutable GEOSGeometry* m_geom;
};

class PyFeature : public PyFeatureBase
{
  public:
    py::object geometry_py() const override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeature, "geometry", geometry_py);
    }

    py::object get_py(const std::string& name) const override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeature, "get", get_py, name);
    }

    void set_py(const std::string& name, py::object value) override
    {
        PYBIND11_OVERRIDE_PURE_NAME(void, PyFeature, "set", set_py, name, value);
    }

    py::object fields() const override
    {
        PYBIND11_OVERRIDE_PURE(py::object, PyFeature, fields);
    }
};

void
bind_feature(py::module& m)
{
    py::class_<Feature>(m, "FeatureBase")
      .def("copy_to", &Feature::copy_to);

    py::class_<PyFeatureBase, Feature>(m, "PyFeatureBase")
      // debugging methods
      .def("get_float", &PyFeature::get_float)
      .def("get_double", &PyFeature::get_double)
      .def("get_string", &PyFeature::get_string)
      .def("check", &PyFeatureBase::check);

    py::class_<PyFeature, PyFeatureBase, Feature>(m, "Feature")
      .def(py::init<>())
      .def("geometry", &PyFeature::geometry_py)
      .def("get", &PyFeature::get_py)
      .def("set", &PyFeature::set_py)
      .def("fields", &PyFeature::fields);
}
}
