#ifndef SIDE_H
#define SIDE_H

#include <iostream>

enum class Side {
    NONE,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

std::ostream& operator<< (std::ostream & os, const Side & s);

#endif

