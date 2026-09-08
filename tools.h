#ifndef TOOLS_H
#define TOOLS_H

#define MAX_STRIDE 32

#include <iostream>
#include <opencv2/core/core.hpp>        // OpenCV 核心（cv::Mat）
#include <opencv2/opencv.hpp>           // OpenCV 全部功能
#include "layer.h"
#include "net.h"

#include <vector>
#include <chrono>

#include "config.h"

struct Object {
    cv::Rect_<float> rect;  // 检测框（x, y, width, height）
    int label;              // 类别索引（0-79）
    float prob;             // 置信度（0-1）
};

int detect_picture_yolo11(const char *param_path,const char *bin_path,
                                const cv::Mat &img_bgr,std::vector<Object> &objects);
void draw_objects(const cv::Mat &img_bgr, const std::vector<Object> &objects);


#endif