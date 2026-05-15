#!/bin/bash
# hardware_check.sh - 硬件能力检查脚本
# 在开发板上运行此脚本，检查摄像头硬件能力

echo "========================================"
echo "  硬件能力检查报告"
echo "  时间: $(date)"
echo "========================================"

echo ""
echo "[1] 系统信息"
echo "----------------------------------------"
uname -a

echo ""
echo "[2] Video设备"
echo "----------------------------------------"
ls -la /dev/video* 2>/dev/null || echo "没有video设备"

echo ""
echo "[3] I2C设备"
echo "----------------------------------------"
ls -la /dev/i2c-* 2>/dev/null || echo "没有i2c设备"

echo ""
echo "[4] 摄像头相关内核模块"
echo "----------------------------------------"
lsmod | grep -E "video|v4l2|isp|csi|pxp|ov5640" || echo "没有相关模块"

echo ""
echo "[5] V4L2能力 (/dev/video0)"
echo "----------------------------------------"
if [ -e /dev/video0 ]; then
    v4l2-ctl --device=/dev/video0 --all 2>/dev/null || echo "v4l2-ctl不可用"
else
    echo "/dev/video0不存在"
fi

echo ""
echo "[6] 支持的像素格式"
echo "----------------------------------------"
if [ -e /dev/video0 ]; then
    v4l2-ctl --device=/dev/video0 --list-formats-ext 2>/dev/null || echo "无法查询格式"
else
    echo "/dev/video0不存在"
fi

echo ""
echo "[7] ISP控制参数 (/dev/video1)"
echo "----------------------------------------"
if [ -e /dev/video1 ]; then
    v4l2-ctl --device=/dev/video1 --list-ctrls 2>/dev/null || echo "无法查询ISP控制"
else
    echo "/dev/video1不存在"
fi

echo ""
echo "[8] I2C设备扫描"
echo "----------------------------------------"
if command -v i2cdetect &> /dev/null; then
    i2cdetect -y 0 2>/dev/null || echo "I2C扫描失败"
else
    echo "i2cdetect不可用"
fi

echo ""
echo "[9] OV5640芯片ID"
echo "----------------------------------------"
if command -v i2cget &> /dev/null; then
    i2cget -y 0 0x3c 0x300a w 2>/dev/null || echo "无法读取芯片ID"
else
    echo "i2cget不可用"
fi

echo ""
echo "[10] 内核日志（摄像头相关）"
echo "----------------------------------------"
dmesg | grep -i -E "camera|ov5640|v4l2|csi" | tail -20 || echo "没有相关日志"

echo ""
echo "[11] 设备树摄像头节点"
echo "----------------------------------------"
find /proc/device-tree -name "*csi*" -o -name "*camera*" -o -name "*ov5640*" 2>/dev/null || echo "没有找到相关节点"

echo ""
echo "[12] PXP设备"
echo "----------------------------------------"
ls -la /dev/pxp* 2>/dev/null || echo "没有pxp设备"

echo ""
echo "[13] Framebuffer设备"
echo "----------------------------------------"
ls -la /dev/fb* 2>/dev/null || echo "没有fb设备"

echo ""
echo "[14] 输入设备"
echo "----------------------------------------"
ls -la /dev/input/event* 2>/dev/null || echo "没有输入设备"

echo ""
echo "========================================"
echo "  检查完成"
echo "========================================"
