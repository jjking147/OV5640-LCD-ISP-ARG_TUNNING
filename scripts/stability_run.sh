#!/bin/bash
# stability_run.sh - 稳定性测试脚本

# 配置
TARGET_IP="192.168.1.100"
TARGET_DIR="/opt"
LOCAL_DIR="."
RESULTS_DIR="./results"
LOG_FILE="stability.log"
REPORT_FILE="stability_report.md"
DURATION_HOURS=72

# 创建结果目录
mkdir -p $RESULTS_DIR

# 编译
echo "Building..."
make clean
make

# 上传到目标板
echo "Uploading to target..."
scp camera_tuning root@$TARGET_IP:$TARGET_DIR/

# 运行稳定性测试
echo "Running stability test for $DURATION_HOURS hours..."
echo "Log file: $RESULTS_DIR/$LOG_FILE"
echo "Report file: $RESULTS_DIR/$REPORT_FILE"

# 后台运行
ssh root@$TARGET_IP "cd $TARGET_DIR && nohup ./camera_tuning -m stability > $LOG_FILE 2>&1 &"

echo "Stability test started in background."
echo "Monitor with: ssh root@$TARGET_IP 'tail -f $TARGET_DIR/$LOG_FILE'"
echo "Stop with: ssh root@$TARGET_IP 'pkill camera_tuning'"

# 等待测试完成
echo "Waiting for test to complete..."
sleep $((DURATION_HOURS * 3600))

# 下载结果
echo "Downloading results..."
scp root@$TARGET_IP:$TARGET_DIR/$LOG_FILE $RESULTS_DIR/
scp root@$TARGET_IP:$TARGET_DIR/$REPORT_FILE $RESULTS_DIR/

echo "Stability test completed. Results in $RESULTS_DIR/"
