# 在imx6ull部署yolo11n

## 目录

```text
project/
├── 0012_imx6ull_deploy_yolo11n/
├── ncnn-master/
└── opencv-3.4.16/
```
`0012_imx6ull_deploy_yolo11n/`就是该文件夹
`ncnn-master/`和`opencv-3.4.16/`请看后面教程自行下载

## 文档目录 deploy_docs
| 文件                          | 内容            |
|:----------------------------|:--------------|
| 01_model_acquisition.md     | 模型获取          |
| 02_ncnn_env_setup.md        | NCNN 环境配置     |
| 03_ncnn_env_setup.md        | opencv 环境配置   |
| 04_vscode_setup_and_code.md | VS Code 配置与代码 |
| 05_run_on_device.md         | 板端运行          |

## 最快方式
1. 根据 03_ncnn_env_setup.md 移植opencv的动态库到板子上
2. 把这个文件夹内的run文件夹给移动到板子上
3. 在板子上的run文件夹内执行命令`./0012_imx6ull_deploy_yolo ./model ./images/bus.jpg`