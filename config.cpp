#include "config.h"

const char *class_names[80] = {"person", "bicycle", "car",
                                    "motorcycle", "airplane", "bus",
                                    "train", "truck", "boat",
                                    "traffic light", "fire hydrant", "stop sign",
                                    "parking meter", "bench", "bird",
                                    "cat", "dog", "horse",
                                    "sheep", "cow", "elephant",
                                    "bear", "zebra", "giraffe",
                                    "backpack", "umbrella", "handbag",
                                    "tie", "suitcase", "frisbee",
                                    "skis", "snowboard", "sports ball",
                                    "kite", "baseball bat", "baseball glove",
                                    "skateboard", "surfboard", "tennis racket",
                                    "bottle", "wine glass", "cup",
                                    "fork", "knife", "spoon",
                                    "bowl", "banana", "apple",
                                    "sandwich", "orange", "broccoli",
                                    "carrot", "hot dog", "pizza",
                                    "donut", "cake", "chair",
                                    "couch", "potted plant", "bed",
                                    "dining table", "toilet", "tv",
                                    "laptop", "mouse", "remote",
                                    "keyboard", "cell phone", "microwave",
                                    "oven", "toaster", "sink",
                                    "refrigerator", "book", "clock",
                                    "vase", "scissors", "teddy bear",
                                    "hair drier", "toothbrush"};

/**
 * @brief 获取检测框颜色表
 * @return 19种 BGR 颜色
 */
const std::vector<cv::Scalar>& get_colors() {
    // static 变量不会销毁，保留在内存中
    static const std::vector<cv::Scalar> colors = {
        cv::Scalar(54, 67, 244),    // 红色
        cv::Scalar(99, 30, 233),    // 紫色
        cv::Scalar(176, 39, 156),   // 粉紫色
        cv::Scalar(183, 58, 103),   // 玫红色
        cv::Scalar(181, 81, 63),    // 橙红色
        cv::Scalar(243, 150, 33),   // 橙色
        cv::Scalar(244, 169, 3),    // 金黄色
        cv::Scalar(212, 188, 0),    // 黄色
        cv::Scalar(136, 150, 0),    // 黄绿色
        cv::Scalar(80, 175, 76),    // 绿色
        cv::Scalar(74, 195, 139),   // 青绿色
        cv::Scalar(57, 220, 205),   // 青色
        cv::Scalar(59, 235, 255),   // 天蓝色
        cv::Scalar(7, 193, 255),    // 亮蓝色
        cv::Scalar(0, 152, 255),    // 蓝色
        cv::Scalar(34, 87, 255),    // 深蓝色
        cv::Scalar(72, 85, 121),    // 蓝灰色
        cv::Scalar(158, 158, 158),  // 灰色
        cv::Scalar(139, 125, 96)    // 灰褐色
    };
    return colors;
}
