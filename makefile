# ============================================
# 变量定义区
# ============================================
# 定义输出目录
BUILD_DIR = build

# 目标文件放到 build 目录
TARGET = $(BUILD_DIR)/$(notdir $(CURDIR))

CROSS_COMPILE = arm-buildroot-linux-gnueabihf-
CC = $(CROSS_COMPILE)g++
READELF = $(CROSS_COMPILE)readelf	

# 编译选项
# -g: 调试信息
# -std=c++11: C++11 标准
# -fopenmp: 启用 OpenMP 多线程并行（你的代码中使用了）
CFLAGS = -finput-charset=UTF-8 -g -std=c++11 -fopenmp

# 包含路径
INCLUDES = -I../ncnn-master/src \
           -I../ncnn-master/build_imx6ull/src/ \
           -I/opt/opencv-arm/include

# 库路径
LDFLAGS = -L../opencv-3.4.16/build-arm/lib \
          -L../ncnn-master/build_imx6ull/src/

# 链接库
LIBS = -lncnn \
       -lopencv_core \
       -lopencv_imgproc \
       -lopencv_imgcodecs \
       -lpthread \
	   -fopenmp


# 在文件夹内查找所有的 *.c、*.cpp和 之后生成的 *.o文件
src = $(wildcard *.c *.cpp)
objs = $(patsubst %.c,$(BUILD_DIR)/%.o,$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(src)))

# 定义依赖文件目录（放在 build 下）
dep_dir = $(BUILD_DIR)/.deps
# 把 objs 里的每个 .o 文件名，转换成 .deps/xxx.o.d 的依赖文件路径
dep_files := $(patsubst $(BUILD_DIR)/%.o,$(dep_dir)/%.d, $(objs))
# 查找并返回实际存在的文件
dep_files := $(wildcard $(dep_files))

$(TARGET): $(objs)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "====== 检查链接库 ======"
	$(READELF) -d $(TARGET) | grep NEEDED || true

# 创建 .deps 目录
$(shell mkdir -p $(BUILD_DIR) $(dep_dir))

# ============================================
# 依赖文件导入区
# ============================================
# 如果有文件就导入链接
ifneq ($(dep_files),)
include $(dep_files)
endif

# ============================================
# 构建规则区
# ============================================
$(BUILD_DIR)/%.o : %.c
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -MD -MF $(dep_dir)/$*.d

$(BUILD_DIR)/%.o : %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -MD -MF $(dep_dir)/$*.d

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)

distclean:
	rm -rf $(BUILD_DIR)

.PHONY: clean distclean