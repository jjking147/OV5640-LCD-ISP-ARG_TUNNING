#!/bin/bash
# check_all.sh - 综合硬件能力检查脚本

REPORT_FILE=${1:-"hardware_report.md"}
VIDEO_DEV=${2:-"/dev/video0"}
I2C_BUS=${3:-0}
I2C_ADDR=${4:-0x3c}

echo "========================================"
echo "  Comprehensive Hardware Check"
echo "========================================"
echo "  Report: $REPORT_FILE"
echo "  Video Device: $VIDEO_DEV"
echo "  I2C Bus: $I2C_BUS"
echo "  I2C Address: $I2C_ADDR"
echo "========================================"

# 生成报告头
cat > "$REPORT_FILE" << 'EOF'
# 硬件能力调研报告

## 目标硬件
EOF

echo "- 视频设备: $VIDEO_DEV" >> "$REPORT_FILE"
echo "- I2C总线: /dev/i2c-$I2C_BUS" >> "$REPORT_FILE"
echo "- 摄像头地址: $I2C_ADDR" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 检查系统信息
echo ""
echo "[1/8] System Info"
echo "----------------------------------------"
echo "## 系统信息" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "- 内核版本: $(uname -r)" >> "$REPORT_FILE"
echo "- 架构: $(uname -m)" >> "$REPORT_FILE"
echo "- 主机名: $(uname -n)" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 检查设备树
echo ""
echo "[2/8] Device Tree"
echo "----------------------------------------"
echo "## 设备树信息" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
if [ -d "/proc/device-tree" ]; then
    echo "### 摄像头相关节点" >> "$REPORT_FILE"
    find /proc/device-tree -name "*csi*" -o -name "*camera*" -o -name "*ov5640*" 2>/dev/null | while read node; do
        echo "- $node" >> "$REPORT_FILE"
    done
    echo "" >> "$REPORT_FILE"
else
    echo "- 设备树信息不可用" >> "$REPORT_FILE"
fi

# 检查V4L2
echo ""
echo "[3/8] V4L2 Capabilities"
echo "----------------------------------------"
echo "## V4L2能力" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
if [ -e "$VIDEO_DEV" ]; then
    echo "- 设备状态: ✅ 存在" >> "$REPORT_FILE"
    V4L2_INFO=$(v4l2-ctl --device="$VIDEO_DEV" --all 2>/dev/null)
    if [ -n "$V4L2_INFO" ]; then
        echo "$V4L2_INFO" | grep -E "Driver|Card|Bus|Capabilities" | while read line; do
            echo "- $line" >> "$REPORT_FILE"
        done
    fi
    echo "" >> "$REPORT_FILE"

    echo "### 支持格式" >> "$REPORT_FILE"
    echo '```' >> "$REPORT_FILE"
    v4l2-ctl --device="$VIDEO_DEV" --list-formats-ext 2>/dev/null >> "$REPORT_FILE" || echo "无法查询格式" >> "$REPORT_FILE"
    echo '```' >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
else
    echo "- 设备状态: ❌ 不存在" >> "$REPORT_FILE"
fi

# 检查ISP
echo ""
echo "[4/8] ISP Capabilities"
echo "----------------------------------------"
echo "## ISP能力" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
ISP_DEV="/dev/video1"
if [ -e "$ISP_DEV" ]; then
    echo "- ISP设备: ✅ 存在 ($ISP_DEV)" >> "$REPORT_FILE"
    echo "### ISP控制参数" >> "$REPORT_FILE"
    echo '```' >> "$REPORT_FILE"
    v4l2-ctl --device="$ISP_DEV" --list-ctrls 2>/dev/null >> "$REPORT_FILE" || echo "无法查询ISP控制" >> "$REPORT_FILE"
    echo '```' >> "$REPORT_FILE"
else
    echo "- ISP设备: ❌ 不存在" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# 检查I2C
echo ""
echo "[5/8] I2C Devices"
echo "----------------------------------------"
echo "## I2C设备" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
if [ -e "/dev/i2c-$I2C_BUS" ]; then
    echo "- I2C总线: ✅ 存在 (/dev/i2c-$I2C_BUS)" >> "$REPORT_FILE"

    # 检查OV5640
    if command -v i2cget &> /dev/null; then
        ID_H=$(i2cget -y $I2C_BUS $I2C_ADDR 0x300a 2>/dev/null)
        ID_L=$(i2cget -y $I2C_BUS $I2C_ADDR 0x300b 2>/dev/null)
        if [ -n "$ID_H" ] && [ -n "$ID_L" ]; then
            echo "- OV5640芯片ID: $ID_H $ID_L" >> "$REPORT_FILE"
            if [ "$ID_H" = "0x56" ] && [ "$ID_L" = "0x40" ]; then
                echo "- 摄像头状态: ✅ OV5640检测到" >> "$REPORT_FILE"
            else
                echo "- 摄像头状态: ❌ 未知芯片" >> "$REPORT_FILE"
            fi
        else
            echo "- 摄像头状态: ❌ 无法读取芯片ID" >> "$REPORT_FILE"
        fi
    else
        echo "- I2C工具: ❌ 未安装 (需要i2c-tools)" >> "$REPORT_FILE"
    fi
else
    echo "- I2C总线: ❌ 不存在" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# 检查内核模块
echo ""
echo "[6/8] Kernel Modules"
echo "----------------------------------------"
echo "## 内核模块" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "### 摄像头相关模块" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
lsmod | grep -E "video|v4l2|isp|csi|pxp|ov5640" 2>/dev/null >> "$REPORT_FILE" || echo "无相关模块" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 检查内核日志
echo ""
echo "[7/8] Kernel Messages"
echo "----------------------------------------"
echo "## 内核日志" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "### 摄像头相关日志" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
dmesg | grep -i -E "camera|ov5640|v4l2|csi|pxp" 2>/dev/null | tail -20 >> "$REPORT_FILE" || echo "无相关日志" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 可行性评估
echo ""
echo "[8/8] Feasibility Assessment"
echo "----------------------------------------"
echo "## 可行性评估" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 评估各项能力
echo "### 功能支持情况" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "| 功能 | 状态 | 说明 |" >> "$REPORT_FILE"
echo "|------|------|------|" >> "$REPORT_FILE"

# V4L2采集
if [ -e "$VIDEO_DEV" ]; then
    echo "| V4L2采集 | ✅ 支持 | 设备存在 |" >> "$REPORT_FILE"
else
    echo "| V4L2采集 | ❌ 不支持 | 设备不存在 |" >> "$REPORT_FILE"
fi

# ISP处理
if [ -e "$ISP_DEV" ]; then
    echo "| ISP处理 | ✅ 支持 | ISP设备存在 |" >> "$REPORT_FILE"
else
    echo "| ISP处理 | ❌ 不支持 | ISP设备不存在 |" >> "$REPORT_FILE"
fi

# I2C调优
if [ -e "/dev/i2c-$I2C_BUS" ]; then
    echo "| I2C调优 | ✅ 支持 | I2C总线存在 |" >> "$REPORT_FILE"
else
    echo "| I2C调优 | ❌ 不支持 | I2C总线不存在 |" >> "$REPORT_FILE"
fi

echo "" >> "$REPORT_FILE"

# 建议
echo "### 开发建议" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "1. 使用V4L2 API进行图像采集" >> "$REPORT_FILE"
echo "2. 使用I2C接口调整OV5640参数" >> "$REPORT_FILE"
if [ -e "$ISP_DEV" ]; then
    echo "3. 利用ISP进行图像后处理" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# 风险点
echo "### 风险点" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
if [ ! -e "$VIDEO_DEV" ]; then
    echo "- ❗ 视频设备不存在，需要检查硬件连接和驱动" >> "$REPORT_FILE"
fi
if [ ! -e "/dev/i2c-$I2C_BUS" ]; then
    echo "- ❗ I2C总线不存在，无法进行寄存器调优" >> "$REPORT_FILE"
fi
if ! command -v v4l2-ctl &> /dev/null; then
    echo "- ❗ v4l2-ctl未安装，建议安装v4l-utils" >> "$REPORT_FILE"
fi
if ! command -v i2cget &> /dev/null; then
    echo "- ❗ i2c-tools未安装，建议安装i2c-tools" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

echo "========================================"
echo "  Check Complete"
echo "  Report saved to: $REPORT_FILE"
echo "========================================"
