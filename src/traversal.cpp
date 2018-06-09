#include <cstddef>

#include "traversal.h"

namespace exactextract {

    void Traversal::add(const Coordinate &c) {
        m_coords.push_back(c);
    }

    bool Traversal::empty() const {
        return m_coords.empty();
    }

    void Traversal::enter(const Coordinate &c, Side s) {
        if (!m_coords.empty()) {
            throw std::runtime_error("Traversal already started");
        }

        add(c);
        m_entry = s;
    }

    void Traversal::exit(const Coordinate &c, Side s) {
        add(c);
        m_exit = s;
    }

    bool Traversal::is_closed_ring() const {
        return m_coords.size() >= 3 && m_coords[0] == m_coords[m_coords.size() - 1];
    }

    bool Traversal::entered() const {
        return m_entry != Side::NONE;
    }

    bool Traversal::exited() const {
        return m_exit != Side::NONE;
    }

    bool Traversal::multiple_unique_coordinates() const {
        for (size_t i = 1; i < m_coords.size(); i++) {
            if (m_coords[0] != m_coords[i]) {
                return true;
            }
        }

        return false;
    }

    bool Traversal::traversed() const {
        return entered() && exited();
    }

    const Coordinate &Traversal::last_coordinate() const {
        return m_coords.at(m_coords.size() - 1);
    }

    const Coordinate &Traversal::exit_coordinate() const {
        if (!exited()) {
            throw std::runtime_error("Can't get exit coordinate from incomplete traversal.");
        }

        return last_coordinate();
    }

}