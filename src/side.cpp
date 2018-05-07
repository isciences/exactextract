#include <ostream>

#include "side.h"

std::ostream& operator<< (std::ostream & os, const Side & s) {
    switch(s) {
        case Side::NONE:   os << "none";   return os;
        case Side::LEFT:   os << "left";   return os;
        case Side::RIGHT:  os << "right";  return os;
        case Side::TOP:    os << "top";    return os;
        case Side::BOTTOM: os << "bottom"; return os;
    }

    return os;
}