/* Optimize the label IR */

#ifndef _SGCPP_LABEL_INTERMEDIATE_HPP
#define _SGCPP_LABEL_INTERMEDIATE_HPP

#include "../ir/intermediate.hpp"

/* Optimizes label IR. After optimization is done, assume that the old optimization block
    CANNOT be used nor copied anymore. */
void optimize_labels(Intermediate::Block &old, Intermediate::Block &optimized);

#endif