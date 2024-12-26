/* Optimize the label IR */

#ifndef _SGCPP_LABEL_INTERMEDIATE_HPP
#define _SGCPP_LABEL_INTERMEDIATE_HPP

#include "../ir/intermediate.hpp"

void optimize_labels(Intermediate::Block &old, Intermediate::Block &optimized);

#endif