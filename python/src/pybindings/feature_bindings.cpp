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
#include "feature_bindings.h"

#include <pybind11/numpy.h>

namespace py = pybind11;

namespace exactextract {
class PyFeatureBase : public Feature
{
  public:
    PyFeatureBase()
      : m_geom(nullptr)
    {
    }

    virtual ~PyFeatureBase()
    {
        if (m_geom != nullptr) {
            GEOSGeom_destroy_r(m_context, m_geom);
        }
    }

    // plumbing to connect python-specifc abstract methods
    // to Feature abstract methods

    double get_double(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        return get_py(name).cast<double>();
    }

    DoubleArray get_double_array(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        py::array_t<double> arr = get_py(name);
        return { arr.data(), static_cast<std::size_t>(arr.size()) };
    }

    std::int32_t get_int(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        return get_py(name).cast<int32_t>();
    }

    std::int64_t get_int64(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        return get_py(name).cast<int64_t>();
    }

    IntegerArray get_integer_array(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        py::array_t<std::int32_t> arr = get_py(name);
        return { arr.data(), static_cast<std::size_t>(arr.size()) };
    }

    Integer64Array get_integer64_array(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        py::array_t<std::int64_t> arr = get_py(name);
        return { arr.data(), static_cast<std::size_t>(arr.size()) };
    }

    std::string get_string(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        return get_py(name).cast<std::string>();
    }

    void set(const std::string& name, std::string value) override
    {
        py::gil_scoped_acquire gil;
        set_py(name, py::str(value));
    }

    void set(const std::string& name, std::int32_t value) override
    {
        py::gil_scoped_acquire gil;
        set_py(name, py::int_(value));
    }

    void set(const std::string& name, std::int64_t value) override
    {
        py::gil_scoped_acquire gil;
        set_py(name, py::int_(value));
    }

    void set(const std::string& name, double value) override
    {
        py::gil_scoped_acquire gil;
        set_py(name, py::float_(value));
    }

    void set(const std::string& name, const DoubleArray& value) override
    {
        set_array(name, value);
    }

    void set(const std::string& name, const IntegerArray& value) override
    {
        set_array(name, value);
    }

    void set(const std::string& name, const Integer64Array& value) override
    {
        set_array(name, value);
    }

    template<typename T>
    void set_array(const std::string& name, const T& value)
    {
        py::gil_scoped_acquire gil;

        std::size_t shape[1]{ value.size };
        py::array_t<typename T::value_type> values(shape);
        auto x = values.template mutable_unchecked<1>();
        for (std::size_t i = 0; i < value.size; i++) {
            x(i) = value.data[i];
        }
        set_py(name, values);
    }

    ValueType field_type(const std::string& name) const override
    {
        py::gil_scoped_acquire gil;
        py::object value = get_py(name);

        try {
            return get_type(value);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Unhandled type for field " + name + " in PyFeatureBase::field_type");
        }
    }

    const GEOSGeometry* geometry() const override
    {
        if (m_geom == nullptr) {
            py::gil_scoped_acquire gil;

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

            if (m_geom == nullptr && !py::isinstance<py::none>(geom)) {
                throw std::runtime_error("Failed to parse geometry.");
            }
        }

        return m_geom;
    }

    void set_geometry(const GEOSGeometry* other) override
    {
        if (m_geom != nullptr) {
            GEOSGeom_destroy_r(m_context, m_geom);
            m_geom = nullptr;
        }

        py::gil_scoped_acquire gil;

        if (other == nullptr) {
            set_geometry_py(py::none());
        } else {
            std::string_view format = set_geometry_format_py().cast<std::string_view>();

            if (format == "wkb") {
                std::size_t wkb_size;
                const unsigned char* buf = GEOSGeomToWKB_buf_r(m_context, other, &wkb_size);
                py::bytes wkb(reinterpret_cast<const char*>(buf), wkb_size);
                set_geometry_py(wkb);
            } else if (format == "geojson") {
                auto writer = GEOSGeoJSONWriter_create_r(m_context);
                char* geojson = GEOSGeoJSONWriter_writeGeometry_r(m_context, writer, other, 0);
                set_geometry_py(py::str(geojson));
                GEOSFree_r(m_context, geojson);
                GEOSGeoJSONWriter_destroy_r(m_context, writer);
            } else {
                throw std::runtime_error("Unhandled format in set_geometry: " + std::string(format));
            }
        }
    }

    void copy_to(Feature& other) const override
    {
        py::gil_scoped_acquire gil;

        py::iterable field_list = fields();
        for (py::handle field : field_list) {
            std::string field_name = field.cast<std::string>();

            other.set(field_name, *this);
        }

        other.set_geometry(geometry());
    }

    // abstract methods to be implemented in Python class

    virtual py::object get_py(const std::string& name) const = 0;

    virtual void set_py(const std::string& name, py::object value) = 0;

    virtual void set_geometry_py(py::object value) = 0;

    virtual py::object set_geometry_format_py() = 0;

    virtual py::object geometry_py() const = 0;

    virtual py::object fields() const = 0;

    // debugging

    bool check()
    {
        return geometry() != nullptr;
    }

  private:
    static inline GEOSContextHandle_t m_context = initGEOS_r(nullptr, nullptr);
    mutable GEOSGeometry* m_geom;
};

class PyFeature : public PyFeatureBase
{
  public:
    py::object geometry_py() const override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeature, "geometry", geometry_py);
    }

    void set_geometry_py(py::object wkb) override
    {
        PYBIND11_OVERRIDE_PURE_NAME(void, PyFeature, "set_geometry", set_geometry_py, wkb);
    }

    py::object set_geometry_format_py() override
    {
        PYBIND11_OVERRIDE_PURE_NAME(py::object, PyFeature, "set_geometry_format", set_geometry_format_py);
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

Feature::ValueType
get_type(const py::object& value)
{
    if (py::isinstance<py::int_>(value)) {
        return Feature::ValueType::INT;
    }

    if (py::isinstance<py::float_>(value)) {
        return Feature::ValueType::DOUBLE;
    }

    if (py::isinstance<py::str>(value)) {
        return Feature::ValueType::STRING;
    }

    if (py::isinstance<py::array_t<double>>(value)) {
        return Feature::ValueType::DOUBLE_ARRAY;
    }

    if (py::isinstance<py::array_t<std::int32_t>>(value)) {
        return Feature::ValueType::INT_ARRAY;
    }

    if (py::isinstance<py::array_t<std::int64_t>>(value)) {
        return Feature::ValueType::INT64_ARRAY;
    }

    throw std::invalid_argument("Unhandled Python type.");
}

void
bind_feature(py::module& m)
{
    py::class_<Feature>(m, "FeatureBase")
      .def("copy_to", &Feature::copy_to);

    py::class_<PyFeatureBase, Feature>(m, "PyFeatureBase")
      // debugging methods
      .def("get_double", &PyFeature::get_double)
      .def("get_string", &PyFeature::get_string)
      .def("check", &PyFeatureBase::check);

    py::class_<PyFeature, PyFeatureBase, Feature>(m, "Feature")
      .def(py::init<>())
      .def("geometry", &PyFeature::geometry_py)
      .def("set_geometry", &PyFeature::set_geometry_py)
      .def("get", &PyFeature::get_py)
      .def("set", &PyFeature::set_py)
      .def("fields", &PyFeature::fields);
}
}
