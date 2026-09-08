#include <float.h>      // 浮点数限制（FLT_MAX 等）
#include <stdio.h>      // 标准输入输出（printf, fprintf）
#include <algorithm>    // std::max, std::min, std::sort 等算法
#include <memory>       // 智能指针
#include <vector>       // std::vector 容器
#include <iostream>     // std::cout 输出

#include "tools.h"

/*
* 命令行输入模板：
*   ./yolo_demo <model_dir> <image_path>
* 
* 参数说明：
*   argv[0]  - 程序名（./yolo_demo）
*   argv[1]  - 模型文件夹路径（model_dir）
*              文件夹内需包含：
*                - model.ncnn.param（网络结构文件）
*                - model.ncnn.bin（网络权重文件）
*   argv[2]  - 待检测图片路径（image_path）
* 
* 示例：
*   ./yolo_demo ./models test.jpg
*   ./yolo_demo /home/root/yolo11 /home/root/images/001.jpg
*/
int main(int argc, char **argv){
    if (argc != 3){
        //标准错误流（stderr）输出错误信息
        fprintf(stderr, "Usage: %s <model_dir> <image_path>\n", argv[0]);   
        return -1;
    }

    // argv 是指针数组（每个元素是 char*），argv+argc 是数组末尾
    // vector 构造函数会把 [argv, argv+argc) 范围内的所有字符串拷贝进来
    std::vector<std::string> args(argv, argv + argc);

    // args[1]      模型文件夹路径
    std::string param_path = args[1] + "/model.ncnn.param"; // .param 文件路径
    std::string bin_path = args[1] + "/model.ncnn.bin";     // .bin 文件路径
    const std::string& image_path = args[2];   // 图片路径

    cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (image.empty()){
        fprintf(stderr, "cv::imread %s failed\n", argv[2]);
        return -1;
    }

    std::vector<Object> objects;
    detect_picture_yolo11(param_path.c_str(), bin_path.c_str(), image, objects);

    draw_objects(image, objects);

    return 0;
}