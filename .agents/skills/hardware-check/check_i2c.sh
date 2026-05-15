#!/bin/bash
# check_i2c.sh - I2C设备检查脚本

BUS=${1:-0}
ADDR=${2:-0x3c}

echo "========================================"
echo "  I2C Device Check"
echo "  Bus: /dev/i2c-$BUS"
echo "  Address: $ADDR"
echo "========================================"

# 检查I2C设备是否存在
if [ ! -e "/dev/i2c-$BUS" ]; then
    echo "[ERROR] I2C bus /dev/i2c-$BUS not found!"
    echo "Available I2C buses:"
    ls -la /dev/i2c-* 2>/dev/null || echo "  No I2C devices found"
    exit 1
fi

echo ""
echo "[1/5] I2C Bus Info"
echo "----------------------------------------"
ls -la /dev/i2c-$BUS

echo ""
echo "[2/5] Scan I2C Bus"
echo "----------------------------------------"
if command -v i2cdetect &> /dev/null; then
    i2cdetect -y $BUS
else
    echo "i2cdetect not available (install i2c-tools)"
fi

echo ""
echo "[3/5] OV5640 Chip ID"
echo "----------------------------------------"
if command -v i2cget &> /dev/null; then
    # 读取芯片ID寄存器 (0x300A, 0x300B)
    ID_H=$(i2cget -y $BUS $ADDR 0x300a 2>/dev/null)
    ID_L=$(i2cget -y $BUS $ADDR 0x300b 2>/dev/null)
    if [ -n "$ID_H" ] && [ -n "$ID_L" ]; then
        echo "Chip ID: $ID_H $ID_L"
        if [ "$ID_H" = "0x56" ] && [ "$ID_L" = "0x40" ]; then
            echo "Status: ✅ OV5640 detected"
        else
            echo "Status: ❌ Unknown chip"
        fi
    else
        echo "Status: ❌ Cannot read chip ID"
    fi
else
    echo "i2cget not available (install i2c-tools)"
fi

echo ""
echo "[4/5] OV5640 Register Dump (partial)"
echo "----------------------------------------"
if command -v i2cget &> /dev/null; then
    echo "Exposure (0x3500-0x3502):"
    i2cget -y $BUS $ADDR 0x3500 2>/dev/null || echo "  Failed"
    i2cget -y $BUS $ADDR 0x3501 2>/dev/null || echo "  Failed"
    i2cget -y $BUS $ADDR 0x3502 2>/dev/null || echo "  Failed"

    echo "Gain (0x350A-0x350B):"
    i2cget -y $BUS $ADDR 0x350a 2>/dev/null || echo "  Failed"
    i2cget -y $BUS $ADDR 0x350b 2>/dev/null || echo "  Failed"

    echo "White Balance (0x3400-0x3405):"
    for reg in 0x3400 0x3401 0x3402 0x3403 0x3404 0x3405; do
        i2cget -y $BUS $ADDR $reg 2>/dev/null || echo "  Failed"
    done
else
    echo "i2cget not available"
fi

echo ""
echo "[5/5] OV5640 Mode Status"
echo "----------------------------------------"
if command -v i2cget &> /dev/null; then
    echo "System Control (0x3008):"
    i2cget -y $BUS $ADDR 0x3008 2>/dev/null || echo "  Failed"

    echo "IO Control (0x3017):"
    i2cget -y $BUS $ADDR 0x3017 2>/dev/null || echo "  Failed"

    echo "Clock Control (0x3034):"
    i2cget -y $BUS $ADDR 0x3034 2>/dev/null || echo "  Failed"
else
    echo "i2cget not available"
fi

echo ""
echo "========================================"
echo "  Check Complete"
echo "========================================"
