# vscode配置和代码

## 建立项目文件夹
```bash
# 确认在执行命令
mkdir 0012_imx6ull_deploy_yolo11n

cd ./0012_imx6ull_deploy_yolo11n
```

## vscode 配置
根据 02_ncnn_env_setup.md 和 03_opencv_env_setup.md 我们可以知道代码所需库的路径为：
- 在 0012_imx6ull_deploy_yolo11n 文件夹上
- 头文件搜索路径（`-I`）：
  - `../ncnn-master/src`
  - `../ncnn-master/build_imx6ull/src/`
  - `/opt/opencv-arm/include`

- 库搜索路径（`-L`）：
  - `../opencv-3.4.16/build-arm/lib`
  - `../ncnn-master/build_imx6ull/src/`

将上面的路径合理放到vscode配置就行了。

## 代码
略 （看代码文件，里面有详细的备注）

## 交叉编译
略（看makefile文件）


