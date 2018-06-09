#ifndef TRAVERSAL_H
#define TRAVERSAL_H

#include <vector>

#include "coordinate.h"
#include "side.h"

namespace exactextract {

    class Traversal {
    public:
        Traversal() : m_entry{Side::NONE}, m_exit{Side::NONE} {}

        bool is_closed_ring() const;

        bool empty() const;

        bool entered() const;

        bool exited() const;

        bool traversed() const;

        bool multiple_unique_coordinates() const;

        void enter(const Coordinate &c, Side s);

        void exit(const Coordinate &c, Side s);

        Side entry_side() const { return m_entry; }

        Side exit_side() const { return m_exit; }

        const Coordinate &last_coordinate() const;

        const Coordinate &exit_coordinate() const;

        void add(const Coordinate &c);

        void force_exit(Side s) { m_exit = s; }

        const std::vector<Coordinate> &coords() const { return m_coords; }

    private:
        std::vector<Coordinate> m_coords;
        Side m_entry;
        Side m_exit;
    };

}

#endif