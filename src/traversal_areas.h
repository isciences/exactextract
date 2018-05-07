#ifndef TRAVERSAL_AREAS_H
#define TRAVERSAL_AREAS_H

#include <vector>

#include "box.h"
#include "coordinate.h"

double left_hand_area(const Box & box, const std::vector<const std::vector<Coordinate>* > & coord_lists);

#endif