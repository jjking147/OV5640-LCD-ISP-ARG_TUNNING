#!/bin/bash
# tuning_test.sh - 图像调优测试脚本

# 配置
TARGET_IP="192.168.1.100"
TARGET_DIR="/opt"
LOCAL_DIR="."
RESULTS_DIR="./results"

# 创建结果目录
mkdir -p $RESULTS_DIR

# 编译
echo "Building..."
make clean
make

# 上传到目标板
echo "Uploading to target..."
scp camera_tuning root@$TARGET_IP:$TARGET_DIR/

# 运行调优测试
echo "Running tuning test..."
ssh root@$TARGET_IP "cd $TARGET_DIR && ./camera_tuning -m tuning" < /dev/null

# 下载结果
echo "Downloading results..."
scp root@$TARGET_IP:$TARGET_DIR/*.yuv $RESULTS_DIR/
scp root@$TARGET_IP:$TARGET_DIR/tuning_report.md $RESULTS_DIR/

echo "Tuning test completed. Results in $RESULTS_DIR/"
