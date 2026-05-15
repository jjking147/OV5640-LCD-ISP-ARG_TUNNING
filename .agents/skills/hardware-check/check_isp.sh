#!/bin/bash
# check_isp.sh - ISP能力检查脚本

echo "========================================"
echo "  ISP Capability Check"
echo "========================================"

echo ""
echo "[1/6] ISP Devices"
echo "----------------------------------------"
echo "Video devices:"
ls -la /dev/video* 2>/dev/null || echo "  No video devices"

echo ""
echo "[2/6] ISP Kernel Modules"
echo "----------------------------------------"
echo "Loaded camera/ISP modules:"
lsmod | grep -E "video|v4l2|isp|csi|pxp" 2>/dev/null || echo "  No modules found"

echo ""
echo "[3/6] ISP Device Capabilities"
echo "----------------------------------------"
# 遍历所有video设备，检查ISP相关的
for dev in /dev/video*; do
    if [ -e "$dev" ]; then
        echo "Device: $dev"
        v4l2-ctl --device="$dev" --info 2>/dev/null | grep -E "Driver|Card|Bus" || echo "  Cannot query"
        echo ""
    fi
done

echo ""
echo "[4/6] ISP Controls"
echo "----------------------------------------"
# 检查第二个video设备（通常是ISP）
ISP_DEV="/dev/video1"
if [ -e "$ISP_DEV" ]; then
    echo "ISP device: $ISP_DEV"
    v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null || echo "  Cannot list controls"
else
    echo "ISP device not found at $ISP_DEV"
fi

echo ""
echo "[5/6] PXP (Pixel Pipeline) Capabilities"
echo "----------------------------------------"
# 检查PXP设备
PXP_DEV="/dev/video1"
if [ -e "$PXP_DEV" ]; then
    echo "PXP device: $PXP_DEV"
    v4l2-ctl --device="$PXP_DEV" --list-formats 2>/dev/null || echo "  Cannot list formats"
    v4l2-ctl --device="$PXP_DEV" --list-ctrls 2>/dev/null || echo "  Cannot list controls"
else
    echo "PXP device not found"
fi

echo ""
echo "[6/6] ISP Feature Detection"
echo "----------------------------------------"
# 检查ISP支持的功能
echo "Checking ISP features..."

# 检查是否有色彩校正
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "color_correction"; then
    echo "✅ Color Correction: Supported"
else
    echo "❌ Color Correction: Not found"
fi

# 检查是否有Gamma
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "gamma"; then
    echo "✅ Gamma Correction: Supported"
else
    echo "❌ Gamma Correction: Not found"
fi

# 检查是否有自动曝光
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "auto_exposure"; then
    echo "✅ Auto Exposure: Supported"
else
    echo "❌ Auto Exposure: Not found"
fi

# 检查是否有自动白平衡
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "auto_white_balance"; then
    echo "✅ Auto White Balance: Supported"
else
    echo "❌ Auto White Balance: Not found"
fi

# 检查是否有锐化
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "sharpness"; then
    echo "✅ Sharpening: Supported"
else
    echo "❌ Sharpening: Not found"
fi

# 检查是否有降噪
if v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null | grep -q "noise_reduction"; then
    echo "✅ Noise Reduction: Supported"
else
    echo "❌ Noise Reduction: Not found"
fi

echo ""
echo "========================================"
echo "  ISP Check Complete"
echo "========================================"
