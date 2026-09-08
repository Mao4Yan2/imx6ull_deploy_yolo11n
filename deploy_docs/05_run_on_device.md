# 设备运行测试

## 注意：
03_opencv_env_setup.md 结尾的动态库，一定要放到**动态链接器默认搜索路径**上

## 执行问题
1. 由于输入硬编码了，命令行输入模板： ./yolo_demo <model_dir> <image_path>
2. <model_dir>文件夹的.param 文件固定命名为：`model.ncnn.param`；bin 文件固定命名为：`model.ncnn.bin`

## 执行文件夹示例
目录
```
run/
├── 0012_imx6ull_deploy_yolo  # 可执行程序
├── images/                   # 存放测试图片
│   └── test.jpg (任意图片文件)
└── model/                    # 模型文件夹（作为 <model_dir> 参数传入）
    ├── model.ncnn.param      # 固定命名
    └── model.ncnn.bin        # 固定命名
```

将目录放到板子上，执行 `./0012_imx6ull_deploy_yolo ./model ./images/test.jpg`

## 板子运行输出
空着



