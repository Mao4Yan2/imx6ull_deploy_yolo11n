#include "tools.h"

/**
 * @brief 计算两个检测框的交集面积
 * @param a 第一个检测框
 * @param b 第二个检测框
 * @return 交集面积（像素单位）
 */
static inline float intersection_area(const Object &a, const Object &b) {
    // 在 OpenCV 中，cv::Rect_<T> 类重载了 & 运算符，用于计算两个矩形的交集区域的x, y, width, height
    cv::Rect_<float> inter = a.rect & b.rect;  // 矩形交集
    return inter.area();                       // 交集面积
}

/**
 * @brief 快速排序（降序）- 按置信度从大到小(左边大，右边小)排列检测框
 * 
 * @param objects 待排序的检测框数组
 * @param start   排序区间的起始索引（包含）
 * @param end     排序区间的结束索引（包含）
 * 
 * 功能说明：
 *   1. 选择区间中间元素的置信度作为基准值（pivot）
 *   2. 分区：将置信度大于基准的放左边，小于基准的放右边
 *   3. 递归排序左右两个子区间
 *   4. 当子区间较大时使用 OpenMP 并行加速
 */
static void QuickSort(std::vector<Object> &objects, int start, int end){
    if (start >= end)
        return;

    int left = start;
    int right = end;
    float mid_prob = objects[(right + left)/2].prob;

    while (left <= right){
        while(left <= right && objects[left].prob > mid_prob) left++;
        while(left <= right && objects[right].prob < mid_prob) right--;
        if (left <= right){
            std::swap(objects[left], objects[right]);
            left++;
            right--;
        }
    }

    if((right - start) > 100 || (end - left) > 100){
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                if(start < right)
                    QuickSort(objects, start, right);
            }
            #pragma omp section
            {
                if(left < end)
                    QuickSort(objects, left, end);
            }
        }
    }
    else{
        if(start < right)
            QuickSort(objects, start, right);
        if(left < end)
            QuickSort(objects, left, end);
    }
}

/**
 * @brief 快速排序（降序）- 重载版本：排序整个数组
 * 
 * @param objects 待排序的检测框数组（引用传递，直接修改原数组）
 * 
 * 功能说明：
 *   1. 这是快速排序的便捷接口，用于排序整个数组
 *   2. 内部调用三参数版本的 QuickSort 完成实际排序
 *   3. 排序结果：按置信度（prob）从大到小排列
 * 
 * 使用示例：
 *   std::vector<Object> objects;
 *   // ... 添加检测框 ...
 *   QuickSort(objects);  // 排序整个数组
 */  
static void QuickSort(std::vector<Object> &objects){
    if(objects.empty()){
        return;
    }

    QuickSort(objects, 0, objects.size()-1);
}

/**
 * @brief NMS（非极大值抑制）- 去除重叠度高的检测框
 * 
 * @param faceobjects   输入：已按置信度降序排序的检测框数组
 * @param picked        输出：保留下来的检测框索引
 * @param nms_threshold IoU 阈值（通常 0.5，范围 0-1）
 * @param agnostic      是否忽略类别（true=所有类别一起做NMS，false=只对同类做NMS）
 * 
 * 功能说明：
 *   1. 输入必须是已按置信度降序排序的检测框
 *   2. 贪心策略：保留置信度最高的框，抑制与它重叠度高的框
 *   3. IoU = 交集面积 / 并集面积，超过阈值则抑制
 *   4. 时间复杂度：O(n*m)，n=总框数，m=保留框数
 * 
 * 使用前提：
 *   - 必须先调用 QuickSort 对检测框按置信度降序排序
 *   - 排序后 confidence[i] >= confidence[i+1]
 */
static void nms_sorted_bboxes(
    const std::vector<Object> &faceobjects,  // 输入：已按置信度降序排序的检测框
    std::vector<int> &picked,                // 输出：保留的检测框索引
    float nms_threshold,                     // IoU 阈值（通常 0.5）
    bool agnostic = false                    // 是否忽略类别（true=所有类别一起NMS）
){
    picked.clear(); // 清空输出
    const int n = faceobjects.size();   // 获取检测框数量

    std::vector<float> areas(n); // 1个vector，n个float
    // 预计算所有框的面积
    for (int i = 0; i < n; i++){
        areas[i] = faceobjects[i].rect.area();
    }

    for (int i = 0; i < n; i++)
    {
        const Object &a = faceobjects[i];   // 当前待检测的框（置信度第 i 高）
        bool keep = true;                   // 标记：是否保留此框（true=保留，false=抑制）
        int picked_size = picked.size();    // 缓存大小

        // 前面有快排排序置信度了，可以添加 && keep 提前跳出
        for (int j = 0; j < picked_size && keep; j++) 
        {
            const Object &b = faceobjects[picked[j]];  // 已保留的框
            if (!agnostic && a.label != b.label)    // 由调用者传入
                continue;  // 类别不同，跳过NMS比较

            // 计算交集面积 IoU（交并比）
            float inter_area = intersection_area(a, b);
            // 计算并集面积 = A面积 + B面积 - 交集面积
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            // 计算 IoU = 交集 / 并集
            if (inter_area / union_area > nms_threshold)
                keep = false;  // 重叠度过高，抑制当前框   
        }
        if (keep)
            picked.push_back(i);  // 如果没被抑制，添加到结果中 
    }
}

/**
 * @brief Sigmoid 激活函数
 * 
 * @param x 输入值（任意实数）
 * @return 输出值（范围 0 到 1）
 * 
 * 数学公式：σ(x) = 1 / (1 + e^(-x))
 * 
 * 功能说明：
 *   - 将任意实数映射到 (0, 1) 区间
 *   - 常用于将网络输出的 logits 转换为概率
 *   - 在 YOLO 中用于转换置信度
 * 
 * 示例：
 *   sigmoid(0)    = 0.5
 *   sigmoid(2)    = 0.881
 *   sigmoid(-2)   = 0.119
 */
static inline float sigmoid(float x) 
{ 
    return static_cast<float>(1.f / (1.f + exp(-x))); 
}

/**
 * @brief 数值限制函数（Clamp）
 * 
 * @param d   输入值
 * @param min 下限
 * @param max 上限
 * @return 限制在 [min, max] 范围内的值
 * 
 * 功能说明：
 *   - 如果 d < min，返回 min
 *   - 如果 d > max，返回 max
 *   - 否则返回 d
 * 
 * 示例：
 *   clampf(5, 0, 10)   = 5    // 在范围内
 *   clampf(-3, 0, 10)  = 0    // 小于下限
 *   clampf(15, 0, 10)  = 10   // 大于上限
 */
static inline float clampf(float d, float min, float max)
{
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

/**
 * @brief 解析 YOLO11 网络输出，转换为可用的检测框
 * 
 * @param inputs               网络原始输出（一维数组，未解析的浮点数）
 * @param confidence_threshold 置信度阈值（低于此值的检测框被过滤）
 * @param num_channels         输出通道数 = 4 + num_labels（4个坐标 + 类别数）
 * @param num_anchors          锚框数量（候选检测框数量）
 * @param num_labels           类别数量（如 COCO 数据集为 80）
 * @param infer_img_width      推理图像宽度（用于坐标转换）
 * @param infer_img_height     推理图像高度（用于坐标转换）
 * @param objects              输出参数：解析后的检测框数组（引用传递）
 * 
 * 数据流：
 *   网络输出（一维数组）
 *     ↓ 重塑为矩阵
 *   Mat(num_channels × num_anchors)
 *     ↓ 转置
 *   Mat(num_anchors × num_channels)
 *     ↓ 逐行遍历
 *   每个锚框的坐标和类别分数
 *     ↓ 置信度过滤
 *   高置信度检测框
 *     ↓ 坐标转换
 *   Object 对象（像素坐标）
 *     ↓ 添加到结果
 *   检测框数组
 */
static void parse_yolo11_detections(
    float *inputs,
    float confidence_threshold,
    int num_channels,
    int num_anchors,
    int num_labels,
    int infer_img_width,
    int infer_img_height,
    std::vector<Object> &objects
){
    std::vector<Object> detections; // 创建临时数组存储检测结果

    // 创建 Mat：num_channels 行 × num_anchors 列; 
    // CV_32F, 每个元素是 32 位浮点数
    // inputs, output 最终指向的是 inputs 的同一块内存，没有拷贝数据
    cv::Mat output = cv::Mat(num_channels, num_anchors, CV_32F, inputs);
    // 转置：变成 num_anchors 行 × num_channels 列
    output = output.t();

    // 翻转前：按"类型"分组（所有 x 在一起，所有 y 在一起）
    // 翻转后：按"框"分组（每个框的 x、y、w 在一起）
    /*
    示例（3个锚框，80类）：
            x    y    w    h    cls0   cls1  ... cls79
    行0:  [0.5, 0.5, 0.1,  0.1,  0.01, 0.02, ..., 0.95]
    行1:  [0.3, 0.7, 0.2,  0.15, 0.8,  0.01, ..., 0.01]
    行2:  [0.8, 0.2, 0.05, 0.08, 0.1,  0.7, ...,  0.02]
    */

    std::cout << "output shape: [" << output.rows << ", " << output.cols << "]" << std::endl;

    for (int i = 0; i < num_anchors; i++)
    {
        // 获取第 i 行的指针;拿到第 i 个框的所有 85 个数字。
        const float *row_ptr = output.row(i).ptr<float>();
        
        // 指针分解
        const float *bboxes_ptr = row_ptr;        // 指向 x（前4个是坐标）
        const float *scores_ptr = row_ptr + 4;    // 指向类别分数（跳过4个坐标）
        
        // 找到最大类别分数
        // std::max_element, 找到一段数据中的最大值，并返回指向它的指针（迭代器）
        const float *max_s_ptr = std::max_element(scores_ptr, scores_ptr + num_labels);
        float score = *max_s_ptr;  // 最大分数值

        if (score > confidence_threshold)
        {
            // 只有置信度超过阈值才处理

            // 顺序读取 x, y, w, h（指针"后缀自增",先返回值、后自增的特性）
            float x = *bboxes_ptr++;  // 中心点 x
            float y = *bboxes_ptr++;  // 中心点 y
            float w = *bboxes_ptr++;  // 宽度
            float h = *bboxes_ptr;    // 高度

            // clampf 的作用: 防止坐标超出图像边界，确保框完全在图像内。
            // 将中心点坐标转换为左上角 (x0, y0) 和右下角 (x1, y1)
            float x0 = clampf((x - 0.5f * w), 0.f, (float)infer_img_width);
            float y0 = clampf((y - 0.5f * h), 0.f, (float)infer_img_height);
            float x1 = clampf((x + 0.5f * w), 0.f, (float)infer_img_width);
            float y1 = clampf((y + 0.5f * h), 0.f, (float)infer_img_height);

            // 数据格式转换
            // 把 YOLO 输出的"中心点+宽高"格式，转成 OpenCV 标准的"左上角+宽高"格式，并打包成统一的 Object 对象
            cv::Rect_<float> bbox;
            bbox.x = x0;
            bbox.y = y0;
            bbox.width = x1 - x0;   // 宽度 = 右边界 - 左边界
            bbox.height = y1 - y0;  // 高度 = 下边界 - 上边界
            
            Object object;
            object.label = max_s_ptr - scores_ptr;  // 类别索引（指针差值）
            object.prob = score;                    // 置信度
            object.rect = bbox;                     // 边界框
            detections.push_back(object);           // 添加到结果
        }
    }
    objects = detections;  // 将临时结果赋值给输出参数
}

/**
 * @brief YOLO11 目标检测主函数（单张图片）
 * 
 * @param param_path NCNN 模型参数文件路径（.param 文件）
 * @param bin_path   NCNN 模型权重文件路径（.bin 文件）
 * @param img_bgr    输入图像（BGR 格式，OpenCV 标准）
 * @param objects    输出参数：检测到的目标框数组
 * @return int       0=成功，其他=失败
 * 
 * 完整流程：
 *   输入图像 (BGR)
 *     ↓ 加载模型
 *   初始化 NCNN 网络
 *     ↓ 图像预处理
 *   Letterbox 缩放 + 填充 + 归一化
 *     ↓ 前向推理
 *   网络输出 (8400 × 84)
 *     ↓ 解析检测结果
 *   候选框提取（置信度过滤）
 *     ↓ 后处理
 *   排序 + NMS + 坐标还原
 *     ↓
 *   最终检测结果
 * 
 * 使用示例：
 *   cv::Mat img = cv::imread("test.jpg");
 *   std::vector<Object> objects;
 *   detect_picture_yolo11("model.param", "model.bin", img, objects);
 *   // objects 现在包含所有检测结果
 */
int detect_picture_yolo11(
    const char *param_path,         // NCNN 模型参数文件路径（.param）
    const char *bin_path,           // NCNN 模型权重文件路径（.bin）
    const cv::Mat &img_bgr,         // 输入图像（BGR 格式）
    std::vector<Object> &objects    // 输出：检测结果
){
    ncnn::Net yolo11;   // 创建网络对象

    // 1. 模型加载计时
    auto t_load_start = std::chrono::steady_clock::now();
    yolo11.load_param(param_path);      // 加载网络结构
    yolo11.load_model(bin_path);        // 加载权重参数
    auto t_load_end = std::chrono::steady_clock::now();
    printf("1. Model load: %.2f ms\n", 
           std::chrono::duration<double, std::milli>(t_load_end - t_load_start).count());


    // 2. 预处理计时
    auto t_pre_start = std::chrono::steady_clock::now();
    // 设置参数
    const int target_size = 640;         // 目标输入尺寸（YOLO 标准 640×640）
    const float prob_threshold = 0.25f;  // 置信度阈值
    const float nms_threshold = 0.45f;   // NMS IoU 阈值

    // 获取原图尺寸
    int img_w = img_bgr.cols;  // 原图宽度
    int img_h = img_bgr.rows;  // 原图高度

    /*---------------图像预处理-------------*/
    /*
    Letterbox 原理：
        1.原图 800×600 -> 缩放后 640×480（保持宽高比）
        2.填充到 640×640（上下各填充 80 像素）
    */

    // 等比缩放
    int w = img_w;
    int h = img_h;
    float scale = 1.f;

    if (w > h){
        scale = (float)target_size / w;  // 以宽度为基准缩放
        w = target_size;                  // 宽度变为 640
        h = h * scale;                    // 高度等比例缩放
    }
    else{
        scale = (float)target_size / h;  // 以高度为基准缩放
        h = target_size;                  // 高度变为 640
        w = w * scale;                    // 宽度等比例缩放
    }

    // 图像缩放和颜色转换
    // ncnn 的方式更适合推理部署, 速度快、省内存
    ncnn::Mat img_processed = ncnn::Mat::from_pixels_resize(
        img_bgr.data,               // 原始图像数据
        ncnn::Mat::PIXEL_BGR2RGB,   // BGR 转 RGB
        img_w, img_h,               // 原始尺寸
        w, h                        // 缩放后尺寸
    );

    // 向上取整到 32 的倍数的计算公式
    // MAX_STRIDE = 32（YOLO11 最大步长)
    // (target_size + MAX_STRIDE - 1) / MAX_STRIDE--这里两个 int 相除，结果就是 int，自动截断
    int wpad = (target_size + MAX_STRIDE - 1) / MAX_STRIDE * MAX_STRIDE - w;
    int hpad = (target_size + MAX_STRIDE - 1) / MAX_STRIDE * MAX_STRIDE - h;

    // 填充边界
    ncnn::Mat img_pad;
    // hpad - hpad / 2 : 处理奇数的情况，确保上下（或左右）填充总量正确(奇数/2, 会少一个像素)
    ncnn::copy_make_border(
        img_processed, img_pad,
        hpad / 2,                    // 上边填充
        hpad - hpad / 2,             // 下边填充
        wpad / 2,                    // 左边填充
        wpad - wpad / 2,             // 右边填充
        ncnn::BORDER_CONSTANT,       // 填充方式：常数
        114.f                        // 填充值（灰色）
    );

    // 创建一个数组，包含 3 个值（对应 RGB 三个通道）; 1/255 是缩放因子，用于把像素值缩小
    const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};

    // substract_mean_normalize(mean, norm) 函数执行：  输出 = (输入 - mean) * norm
    // 第一个参数 0：均值（mean），这里传 0 表示不减均值
    img_pad.substract_mean_normalize(0, norm_vals);
    auto t_pre_end = std::chrono::steady_clock::now();
    printf("2. Preprocess: %.2f ms\n", 
           std::chrono::duration<double, std::milli>(t_pre_end - t_pre_start).count());

    

    /*---------------推理-------------*/
    // 3. create_extractor 计时
    auto t_create_start = std::chrono::steady_clock::now();
    ncnn::Extractor ex = yolo11.create_extractor();
    auto t_create_end = std::chrono::steady_clock::now();
    printf("3. Create extractor: %.2f ms\n", 
           std::chrono::duration<double, std::milli>(t_create_end - t_create_start).count());



    // 调试
    std::cout << "in0 Shape: ["
          << img_pad.w << ", "  // 宽度
          << img_pad.h << ", "  // 高度
          << img_pad.d << ", "  // 深度
          << img_pad.c << "]"   // 通道数
          << std::endl;

    // 4. 输入数据计时
    auto t_input_start = std::chrono::steady_clock::now();
    // "in0"不能改, .param 文件里写的是什么, 代码就用什么
    ex.input("in0", img_pad);
    auto t_input_end = std::chrono::steady_clock::now();
    printf("4. Input: %.2f ms\n", 
           std::chrono::duration<double, std::milli>(t_input_end - t_input_start).count());


    std::vector<Object> proposals;  // 所有候选检测框

    // stride 32 检测头
    {
        ncnn::Mat out;

        // 5. 纯推理计时
        auto t_infer_start = std::chrono::steady_clock::now();
        // 执行推理
        /*
        1. 前向传播（forward propagation）
        2. 逐层计算（卷积、激活、池化等）
        3. 得到最终输出
        */
        ex.extract("out0", out);  // 提取输出
        auto t_infer_end = std::chrono::steady_clock::now();
        printf("5. Pure inference: %.2f ms\n", 
            std::chrono::duration<double, std::milli>(t_infer_end - t_infer_start).count());


        std::cout << "pred Shape: ["
                << out.w << ", "  // 8400（锚框数）
                << out.h << ", "  // 84（4坐标 + 80类别）
                << out.d << ", "  // 1
                << out.c << "]"   // 1
                << std::endl;

        std::vector<Object> objects32;
        // 总字节数 ÷ 单个元素字节数; class_names[0]是指针
        const int num_labels = sizeof(class_names) / sizeof(class_names[0]);
        
        // 解析检测结果
        parse_yolo11_detections(
            (float *)out.data,       // 原始输出
            prob_threshold,           // 置信度阈值
            out.h,                    // 通道数（84）
            out.w,                    // 锚框数（8400）
            num_labels,               // 类别数（80）
            img_pad.w, img_pad.h,      // 输入尺寸
            objects32                 // 输出
        );
        
        // 合并到总候选框
        // 从 proposals.end() 开始存放 objects32.begin() 到 objects32.end() 的候选框
        proposals.insert(proposals.end(), objects32.begin(), objects32.end());
    }


    /*-----------------后处理-----------------*/
    // 6. 后处理计时
    auto t_post_start = std::chrono::steady_clock::now();

    // 按置信度降序排序
    QuickSort(proposals);

    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    // 坐标还原（关键步骤）
    int count = picked.size();
    objects.resize(count);  // 改变 objects 的大小为 count 个元素

    for (int i = 0; i < count; i++)
    {
        objects[i] = proposals[picked[i]];

        // 从填充后的坐标还原到原图坐标
        // 坐标还原公式： 原始坐标 = (填充后坐标 - 填充偏移) / 缩放比例
        float x0 = (objects[i].rect.x - (wpad / 2)) / scale;
        float y0 = (objects[i].rect.y - (hpad / 2)) / scale;
        float x1 = (objects[i].rect.x + objects[i].rect.width - (wpad / 2)) / scale;
        float y1 = (objects[i].rect.y + objects[i].rect.height - (hpad / 2)) / scale;

        // 把坐标限制在图像范围内，防止检测框跑到图片外面
        x0 = std::max(std::min(x0, (float)(img_w - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(img_h - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(img_w - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(img_h - 1)), 0.f);

        // 更新检测框
        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }
    auto t_post_end = std::chrono::steady_clock::now();
    printf("6. Post-process: %.2f ms\n", 
           std::chrono::duration<double, std::milli>(t_post_end - t_post_start).count());

    return 0;
}

/**
 * @brief 绘制目标检测结果并保存图像
 * 
 * @param img_bgr 输入图像（BGR 格式，OpenCV 默认读取格式）
 * @param objects 检测结果数组（包含检测框位置、类别、置信度）
 * 
 * 功能说明：
 *   1. 克隆输入图像（避免修改原图）
 *   2. 在图像上绘制每个检测框（使用不同颜色区分）
 *   3. 在每个检测框上方绘制标签（类别名 + 置信度）
 *   4. 将标注后的图像保存到 ./images/output.jpg
 * 
 * 使用示例：
 *   cv::Mat img = cv::imread("test.jpg");
 *   std::vector<Object> objects;
 *   // ... 检测 ...
 *   draw_objects(img, objects);
 *   // 结果保存在 ./images/output.jpg
 */
void draw_objects(
    const cv::Mat &img_bgr,              // 输入图像（BGR格式）
    const std::vector<Object> &objects // 检测结果数组
){
    auto t1 = std::chrono::steady_clock::now();
    const std::vector<cv::Scalar>& colors = get_colors();
    int color_index = 0;                    // 颜色索引（循环使用19种颜色）
    cv::Mat image = img_bgr.clone();           // 克隆原图（避免修改原图）

    for (size_t i = 0; i < objects.size(); i++){

        const Object &obj = objects[i];  // 获取当前检测框（引用，避免拷贝）
        
        // 循环使用颜色
        cv::Scalar color = colors[color_index % colors.size()];
        color_index++;

        // 打印检测信息（调试）
        fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", 
                obj.label,      // 类别索引
                obj.prob,       // 置信度（0-1）
                obj.rect.x,     // 左上角 x
                obj.rect.y,     // 左上角 y
                obj.rect.width, // 宽度
                obj.rect.height // 高度
            );

        /*
        参数说明：
        - image: 目标图像
        - obj.rect: 矩形位置和大小
        - cc: 颜色
        - 2: 线条粗细（像素）
        */
        cv::rectangle(image, obj.rect, color, 2);

        char text[256];
        // sprintf 把类别名和置信度格式化成字符串，存入 text 数组
        sprintf(text, "%s %.1f", class_names[obj.label], obj.prob * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(
            text,                        // 文本内容
            cv::FONT_HERSHEY_SIMPLEX,   // 字体
            0.5,                        // 字体缩放
            1,                          // 线条粗细
            &baseLine                   // 基线位置（输出参数）
        );

        // cv::putText 的坐标默认是文字的左下角位置。
        // 在 OpenCV 中, 原点 (0, 0) 在左上角, x轴向右增大, y轴向下增大

        int x = obj.rect.x;                          // 标签起始 x（检测框左上角）
        int y = obj.rect.y - label_size.height - baseLine;  // 标签在检测框上方

        // 如果标签有一点超出图像顶部，放在检测框内部顶部
        if (y < label_size.height)
            y = obj.rect.y + label_size.height;
        // 如果标签超出左边界，右移动一点
        if (x < 0)
            x = 0;  

        cv::rectangle(
            image, 
            cv::Rect(cv::Point(x, y), 
            cv::Size(label_size.width, label_size.height + baseLine)),
            color,   // 背景颜色（与检测框同色）
            -1    // -1 表示填充
        );
        cv::putText(
            image,                       // 目标图像
            text,                        // 文本内容
            cv::Point(x, y + label_size.height),  // 文本左下角位置
            cv::FONT_HERSHEY_SIMPLEX,   // 字体
            0.5,                        // 字体缩放
            cv::Scalar(255, 255, 255)   // 白色
        );

        
    }
    bool success_img = cv::imwrite("./images/output.jpg", image);
    if (!success_img){
        std::cout << "failed to save" << std::endl;
    }
    else{
        std::cout << "save successfully" << std::endl;
    }
    auto t2 = std::chrono::steady_clock::now();
    double load_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
    fprintf(stderr, "7.draw picture: %.2f ms\n", load_time);
}