#ifndef CLASS_NAME_H
#define CLASS_NAME_H

#include <opencv2/core/core.hpp>

extern const char *class_names[80];
const std::vector<cv::Scalar>& get_colors();

#endif 