#include <map>
#include <list>
#include <pybind11/pybind11.h>

#include "output_writer.h"

namespace py = pybind11;

namespace exactextract
{
    class MapWriter : public OutputWriter
    {

    public:
        explicit MapWriter();

        ~MapWriter() override;

        void add_operation(const Operation &op) override;

        void reset_operation();

        void set_registry(const StatsRegistry *reg) override;

        void write(const std::string &fid) override;

        void clear();

        std::map<const std::string, std::list<double>> get_map();

    private:
        std::map<const std::string, std::list<double>> m_dict = {};
        const StatsRegistry *m_reg;
        bool id_field_defined = false;
    };

    void bind_writer(py::module &m);
}