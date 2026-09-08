# i.MX6ULL opencv环境搭建与移植
---
## 流程
```
第1步：在 PC 上交叉编译 OpenCV
  PC 编译 → 生成 ARM 版 OpenCV 库
  ↓
第2步：安装到 PC 的 /opt/opencv-arm
  sudo make install → 复制到 PC 的目录
  ↓
第3步：在 PC 上交叉编译你的程序
  arm-gcc + /opt/opencv-arm 的库 → 生成 ARM 可执行文件
  ↓
第4步：传输到 i.MX6ULL
  scp 可执行文件 + 库文件 → 开发板
```
---
## 一、下载opencv包
因为PC宿主机上的opencv框架是x86的，所以不能作为链接文件去够造imx6ull开发板所需要的执行文件，因此我们需要重新交叉编译.
### 下载 OpenCV:
```text
# 下载 OpenCV
https://github.com/opencv/opencv/archive/3.4.16.zip
unzip opencv-3.4.16.zip && cd opencv-3.4.16

# opencv-3.4.16 就是opencv
```

## 二、使用自己的交叉工具链创建自己的工具链文件

**OpenCV 自带的 arm-gnueabi.toolchain.cmake 是别人写的通用配置，里面的编译器路径可能和你的不匹配。**

```
# 你的交叉编译器可能是：
arm-buildroot-linux-gnueabihf-gcc
arm-buildroot-linux-gnueabihf-g++

# OpenCV 自带的可能写的是：
arm-linux-gnueabi-gcc  # 或者其他名字
```

 **确保在 `/opencv-3.4.16/platforms/linux`路径，创建自己的工具链文件：**
```text
# 创建新工具链文件
vi arm-buildroot-gnueabihf.toolchain1.cmake
```
在文件内输入以下内容（与ncnn的一致）。注意 `arm-buildroot-linux-gnueabihf-gcc` 和 `arm-buildroot-linux-gnueabihf-g++` 是自己的交叉工具链
```
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 关键：指定你自己的交叉编译器
set(CMAKE_C_COMPILER "arm-buildroot-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-buildroot-linux-gnueabihf-g++")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 针对 i.MX6ULL 的优化
set(CMAKE_C_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=neon")
set(CMAKE_CXX_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=neon")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
```
然后保存退出

## 三、安装 opencv
确保自己在 `~/opencv-3.4.16` 路径，然后执行以下命令：
```
# 1.创建构建目录
mkdir build-arm && cd build-arm

# 2.配置交叉编译（DBUILD_LIST 去掉 highgui，可以选择加上）
# 如果要视频和摄像头，DBUILD_LIST 里添加 video 和 videoio
# DCMAKE_TOOLCHAIN_FILE 这里是自己上面创建的 arm-buildroot-gnueabihf.toolchain1.cmake 文件
cmake \
  -DCMAKE_TOOLCHAIN_FILE=../platforms/linux/arm-buildroot-gnueabihf.toolchain1.cmake \
  -DCMAKE_INSTALL_PREFIX=/opt/opencv-arm \
  -DBUILD_LIST=core,imgcodecs,imgproc \
  -DWITH_GTK=OFF \
  -DWITH_JPEG=ON \
  -DWITH_PNG=ON ..

# 3.编译并安装
make -j4
sudo make install
```

"3.配置交叉编译"的各个参数的含义如下：

| 参数                                                                      | 作用                                                                            |
|:------------------------------------------------------------------------|:------------------------------------------------------------------------------|
| `-DCMAKE_TOOLCHAIN_FILE=../platforms/linux/arm-gnueabi.toolchain.cmake` | **指定交叉编译工具链**。这个文件里定义了使用哪个 ARM 编译器（如 `arm-linux-gnueabi-gcc`），以及 ARM 架构的具体参数。 |
| `-DCMAKE_INSTALL_PREFIX=/opt/opencv-arm`                                | **指定安装路径**。编译完成后，生成的库文件（.so）和头文件（.h）会被安装到这个目录下，方便后续你的程序链接时引用。                 |
| `-DBUILD_LIST=core,highgui,imgcodecs,imgproc`                           | **指定只编译核心模块**。OpenCV 有很多模块，这里只选了最基础的四个（核心、图像显示、图像编解码、图像处理），可以大幅缩短编译时间。        |
| `-DWITH_GTK=OFF`                                                        | **关闭 GTK 图形界面支持**。imx6ull 开发板通常跑的是精简系统，不需要 GUI 显示，关闭它可以避免编译时缺少依赖库而报错。         |
| `-DWITH_JPEG=ON` 和 `-DWITH_PNG=ON`                                      | **开启 JPEG 和 PNG 图片格式支持**。因为目标检测需要读取图片，这两个开关确保交叉编译出来的 OpenCV 能正常处理这两种常见格式。     |
| `..`                                                                    | **指定源码目录**。告诉 CMake，源码（CMakeLists.txt）在上一级目录（即解压后的 `opencv-3.4.16` 文件夹下）。     |

## 环境文件
### opencv 动态库
opencv 代码的库文件路径为：
```
# 路径
/opt/opencv-arm/include

# 文件
drwxr-xr-x 7 root root   4096 Sep  3 03:13 core
-rw-r--r-- 1 root root 151205 Oct  8  2021 core.hpp
-rw-r--r-- 1 root root   5249 Sep  3 03:03 cvconfig.h
drwxr-xr-x 2 root root   4096 Sep  3 03:13 imgcodecs
-rw-r--r-- 1 root root  16479 Oct  8  2021 imgcodecs.hpp
drwxr-xr-x 4 root root   4096 Sep  3 03:13 imgproc
-rw-r--r-- 1 root root 243229 Oct  8  2021 imgproc.hpp
-rw-r--r-- 1 root root   4503 Oct  8  2021 opencv.hpp
-rw-r--r-- 1 root root    427 Sep  3 03:03 opencv_modules.hpp
```
opencv 动态库路径为：
```
# 路径
~/project/opencv-3.4.16/build-arm/lib

# 文件
-rw-rw-r-- 1 book book 1280120 Sep  3 03:07 libcarotene.a
lrwxrwxrwx 1 book book      21 Sep  3 03:07 libopencv_core.so -> libopencv_core.so.3.4
lrwxrwxrwx 1 book book      24 Sep  3 03:07 libopencv_core.so.3.4 -> libopencv_core.so.3.4.16
-rwxrwxr-x 1 book book 3708028 Sep  3 03:07 libopencv_core.so.3.4.16
lrwxrwxrwx 1 book book      26 Sep  3 03:08 libopencv_imgcodecs.so -> libopencv_imgcodecs.so.3.4
lrwxrwxrwx 1 book book      29 Sep  3 03:08 libopencv_imgcodecs.so.3.4 -> libopencv_imgcodecs.so.3.4.16
-rwxrwxr-x 1 book book 2184572 Sep  3 03:08 libopencv_imgcodecs.so.3.4.16
lrwxrwxrwx 1 book book      24 Sep  3 03:08 libopencv_imgproc.so -> libopencv_imgproc.so.3.4
lrwxrwxrwx 1 book book      27 Sep  3 03:08 libopencv_imgproc.so.3.4 -> libopencv_imgproc.so.3.4.16
-rwxrwxr-x 1 book book 4144080 Sep  3 03:08 libopencv_imgproc.so.3.4.16
```
将动态库文件移动到板子上的**动态链接器默认搜索路径**,查看**动态链接器默认搜索路径**的指令：`cat /etc/ld.so.conf.d/*.conf`
- 建议放到 `/usr/local/lib`
