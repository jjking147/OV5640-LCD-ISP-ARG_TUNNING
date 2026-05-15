#!/bin/bash
# check_v4l2.sh - V4L2能力检查脚本

DEVICE=${1:-/dev/video0}

echo "========================================"
echo "  V4L2 Hardware Capability Check"
echo "  Device: $DEVICE"
echo "========================================"

# 检查设备是否存在
if [ ! -e "$DEVICE" ]; then
    echo "[ERROR] Device $DEVICE not found!"
    echo "Available video devices:"
    ls -la /dev/video* 2>/dev/null || echo "  No video devices found"
    exit 1
fi

echo ""
echo "[1/6] Device Info"
echo "----------------------------------------"
ls -la "$DEVICE"

echo ""
echo "[2/6] V4L2 Capabilities"
echo "----------------------------------------"
v4l2-ctl --device="$DEVICE" --all 2>/dev/null || echo "v4l2-ctl not available"

echo ""
echo "[3/6] Supported Formats"
echo "----------------------------------------"
v4l2-ctl --device="$DEVICE" --list-formats-ext 2>/dev/null || echo "Cannot list formats"

echo ""
echo "[4/6] Supported Frame Sizes (YUYV)"
echo "----------------------------------------"
v4l2-ctl --device="$DEVICE" --list-framesizes=YUYV 2>/dev/null || echo "Cannot list frame sizes"

echo ""
echo "[5/6] Supported Controls"
echo "----------------------------------------"
v4l2-ctl --device="$DEVICE" --list-ctrls 2>/dev/null || echo "Cannot list controls"

echo ""
echo "[6/6] Current Settings"
echo "----------------------------------------"
v4l2-ctl --device="$DEVICE" --get-fmt-video 2>/dev/null || echo "Cannot get format"

echo ""
echo "========================================"
echo "  Check Complete"
echo "========================================"
