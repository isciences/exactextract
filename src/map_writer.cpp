#include <map>
#include <list>

#include "processor.h"
#include "utils.h"
#include "map_writer.h"

namespace exactextract
{
    MapWriter::MapWriter() {}

    MapWriter::~MapWriter() {}

    void MapWriter::add_operation(const Operation &op)
    {
        m_ops.push_back(&op);
    }

    void MapWriter::reset_operation()
    {
        m_ops.clear();
    }

    void MapWriter::set_registry(const StatsRegistry *reg)
    {
        m_reg = reg;
    }

    void MapWriter::write(const std::string &fid)
    {
        if (!m_dict.count(fid))
        {
            m_dict[fid] = std::list<double>();
        }
        for (const auto &op : m_ops)
        {
            if (m_reg->contains(fid, *op))
            {
                const auto &stats = m_reg->stats(fid, *op);
                auto fetcher = op->result_fetcher();

                auto val = fetcher(stats);
                if (val.has_value())
                {
                    m_dict[fid].push_back(val.value());
                }
                else
                {
                    m_dict[fid].push_back(std::numeric_limits<double>::quiet_NaN());
                }
            }
        }
    }

    void MapWriter::clear()
    {
        m_dict.clear();
    }

    std::map<const std::string, std::list<double>> MapWriter::get_map()
    {
        return m_dict;
    }
}