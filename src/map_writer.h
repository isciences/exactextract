#ifndef EXACTEXTRACT_MAPWRITER_H
#define EXACTEXTRACT_MAPWRITER_H

#include <map>
#include <list>

#include "output_writer.h"

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
}

#endif // EXACTEXTRACT_MAPWRITER_H