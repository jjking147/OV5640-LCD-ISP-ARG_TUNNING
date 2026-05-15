---
name: hardware-check
description: 硬件能力调研工具，在写代码前检查目标硬件是否支持所需功能。用于V4L2能力查询、ISP功能检查、摄像头格式支持验证等。当用户要实现摄像头相关功能时，先用此skill验证可行性。
---

# Hardware Check - 硬件能力调研工具

在编写嵌入式Camera代码之前，先检查目标硬件是否支持所需功能，避免写出无法运行的代码。

## 使用场景

当用户说：
- "我要实现XX功能"
- "摄像头能支持XX格式吗"
- "板子有ISP功能吗"
- "帮我写V4L2采集代码"
- "实现XX图像处理"

**先问自己：硬件支持吗？** 然后用此skill检查。

## 检查流程

### Step 1: 确认目标硬件

向用户确认：
1. 目标板子型号（如i.MX6ULL）
2. 摄像头型号（如OV5640）
3. 目标设备路径（如/dev/video0）
4. 连接方式（DVP/MIPI/USB）

### Step 2: 检查V4L2能力

运行检查命令：

```bash
# 检查设备是否存在
ls -la /dev/video*

# 查询V4L2能力
v4l2-ctl --device=/dev/video0 --all

# 查询支持的格式
v4l2-ctl --device=/dev/video0 --list-formats-ext

# 查询支持的帧大小
v4l2-ctl --device=/dev/video0 --list-framesizes=YUYV

# 查询支持的帧率
v4l2-ctl --device=/dev/video0 --list-frameintervals=width=640,height=480,pixelformat=YUYV
```

### Step 3: 检查ISP能力

```bash
# 检查ISP设备
ls -la /dev/video*

# 查询ISP控制参数
v4l2-ctl --device=/dev/video1 --list-ctrls

# 查询ISP支持的功能
v4l2-ctl --device=/dev/video1 --list-controls
```

### Step 4: 检查I2C设备

```bash
# 检查I2C总线
ls -la /dev/i2c-*

# 扫描I2C设备（需要i2c-tools）
i2cdetect -y 0

# 读取OV5640芯片ID
i2cget -y 0 0x3c 0x300a w
```

### Step 5: 检查内核驱动

```bash
# 查看已加载的摄像头驱动
lsmod | grep video
lsmod | grep ov5640
lsmod | grep mx6s

# 查看内核日志中的摄像头信息
dmesg | grep -i camera
dmesg | grep -i ov5640
dmesg | grep -i v4l2

# 查看设备树中的摄像头节点
ls /proc/device-tree/soc/
cat /proc/device-tree/soc/bus/.../status
```

### Step 6: 检查PXP（像素处理引擎）

```bash
# 检查PXP设备
ls -la /dev/pxp*

# 查询PXP能力
v4l2-ctl --device=/dev/video1 --list-formats
```

## 输出报告格式

```markdown
# 硬件能力调研报告

## 目标硬件
- 板子: i.MX6ULL
- 摄像头: OV5640
- 设备: /dev/video0

## V4L2能力
- 设备状态: ✅ 存在
- 驱动: mx6s-csi
- 支持格式: YUYV, UYVY, RGB565
- 最大分辨率: 2592x1944
- 最小分辨率: 320x240
- 支持帧率: 15fps@1080p, 30fps@720p, 30fps@VGA

## ISP能力
- ISP设备: ✅ 存在 (/dev/video1)
- 支持功能: 曝光、白平衡、色彩校正
- 控制参数: ...

## I2C设备
- I2C总线: /dev/i2c-0
- OV5640地址: 0x3c ✅ 检测到
- 芯片ID: 0x5640 ✅ 正确

## 可行性评估
- V4L2采集: ✅ 支持
- 格式转换: ✅ 支持 (PXP)
- ISP处理: ✅ 支持
- 寄存器调优: ✅ 支持 (I2C)

## 建议
1. 使用YUYV格式采集
2. 利用PXP做格式转换
3. 通过I2C调整OV5640参数
4. 使用ISP做色彩校正

## 风险点
1. 无
```

## 关键检查项

### V4L2 Capabilities
- `V4L2_CAP_VIDEO_CAPTURE` - 视频采集
- `V4L2_CAP_STREAMING` - 流式传输
- `V4L2_CAP_READWRITE` - 读写模式
- `V4L2_CAP_DEVICE_CAPS` - 设备能力

### 像素格式
- YUYV - 最常用，无需转换
- UYVY - 需要字节序转换
- RGB565 - 直接显示到FB
- MJPEG - 压缩格式，节省带宽
- NV12/NV21 - YUV420SP，用于ISP

### ISP功能
- 曝光控制 (Auto Exposure)
- 白平衡 (Auto White Balance)
- 色彩校正 (Color Correction Matrix)
- Gamma校正
- 降噪 (Noise Reduction)
- 锐化 (Sharpening)

## 常见问题

### Q: v4l2-ctl命令不存在
```bash
# 安装v4l-utils
apt-get install v4l-utils  # Debian/Ubuntu
opkg install v4l-utils     # Yocto
```

### Q: i2cget/i2cset命令不存在
```bash
# 安装i2c-tools
apt-get install i2c-tools
```

### Q: 设备节点不存在
1. 检查设备树是否正确配置
2. 检查内核驱动是否加载
3. 检查硬件连接

### Q: 摄像头检测不到
1. 检查I2C连接
2. 检查电源和时钟
3. 检查复位引脚
4. 查看dmesg日志

## 决策树

```
要实现XX功能
    ↓
需要什么硬件能力？
    ↓
检查对应capability
    ↓
┌─────────────────────────────────────┐
│ 支持？                               │
│   是 → 继续开发                       │
│   否 → 寻找替代方案 或 修改需求        │
└─────────────────────────────────────┘
```

## 示例对话

用户: "我要实现摄像头实时预览+边缘检测"

助手:
1. 先确认硬件信息
2. 运行hardware-check检查
3. 输出调研报告
4. 基于报告给出实现建议
5. 确认可行后再写代码

## 注意事项

1. **必须在写代码前检查** - 避免写出无法运行的代码
2. **检查所有依赖** - 不仅是主设备，还有辅助设备
3. **记录检查结果** - 作为项目文档的一部分
4. **考虑边界情况** - 不同分辨率/格式可能有不同支持情况
5. **测试实际效果** - 理论支持不等于实际可用
