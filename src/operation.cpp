#include "operation.h"

#include <sstream>

namespace exactextract {

void
Operation::setQuantileFieldNames()
{

    for (double q : m_quantiles) {
        std::stringstream ss;
        int qint = static_cast<int>(100 * q);
        ss << "q_" << qint;
        m_field_names.push_back(ss.str());
    }
}

}
