# Camera Tuning & Stability Test Makefile
# 交叉编译工具链
CC = /opt/toolchains/arm-linux-gnueabihf-gcc-4.9.4/bin/arm-linux-gnueabihf-gcc
CFLAGS = -O2 -Wall -pthread -std=c99 -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

# 目标文件
TARGET = camera_tuning
OBJS = camera_tuning_main.o v4l2_utils.o fb_utils.o \
       ov5640_tuning.o stability_test.o perf_analysis.o

# 源文件
SRCS = camera_tuning_main.c v4l2_utils.c fb_utils.c \
       ov5640_tuning.c stability_test.c perf_analysis.c

# 头文件
HDRS = v4l2_utils.h fb_utils.h ov5640_tuning.h \
       stability_test.h perf_analysis.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	scp $(TARGET) root@192.168.1.100:/opt/

# 编译到本地 (x86)
local: CC = gcc
local: CFLAGS = -O2 -Wall -pthread -std=c99 -D_POSIX_C_SOURCE=199309L -DLOCAL_BUILD
local: $(TARGET)

# 调试版本
debug: CFLAGS = -g -Wall -pthread -std=c99 -D_POSIX_C_SOURCE=199309L -DDEBUG
debug: $(TARGET)

# 性能分析版本
perf: CFLAGS = -O2 -Wall -pthread -std=c99 -D_POSIX_C_SOURCE=199309L -pg
perf: $(TARGET)

.PHONY: all clean install local debug perf
