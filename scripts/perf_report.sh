#!/bin/bash
# perf_report.sh - 性能报告脚本

# 配置
TARGET_IP="192.168.1.100"
TARGET_DIR="/opt"
LOCAL_DIR="."
RESULTS_DIR="./results"
REPORT_FILE="perf_report.md"

# 创建结果目录
mkdir -p $RESULTS_DIR

# 编译
echo "Building..."
make clean
make

# 上传到目标板
echo "Uploading to target..."
scp camera_tuning root@$TARGET_IP:$TARGET_DIR/

# 运行性能测试
echo "Running performance test..."
ssh root@$TARGET_IP "cd $TARGET_DIR && ./camera_tuning -m perf" < /dev/null

# 下载结果
echo "Downloading results..."
scp root@$TARGET_IP:$TARGET_DIR/$REPORT_FILE $RESULTS_DIR/

echo "Performance test completed. Results in $RESULTS_DIR/"
