# Camera + OV5640寄存器调优 + 稳定性测试

基于i.MX6ULL + OV5640的Camera嵌入式开发项目3

## 项目概述

本项目专注于 **OV5640摄像头硬件调优**，通过I2C读写OV5640内部寄存器，控制其内置ISP功能，实现图像质量优化。

## 核心技术栈

### 1. OV5640寄存器调优（核心）
- **I2C通信**：通过 `/dev/i2c-1` 读写OV5640寄存器
- **寄存器控制**：曝光、增益、白平衡、锐度、降噪、饱和度
- **硬件ISP**：利用OV5640内置的ISP处理，不消耗CPU

### 2. RGB565直接输出（性能优化）
- **OV5640直接输出RGB565**：避免CPU做YUYV→RGB565格式转换
- **零拷贝显示**：RGB565直接写入framebuffer
- **CPU占用极低**：只做memcpy，不做像素级运算

### 3. 稳定性测试
- 72小时连续运行测试
- 帧率、丢帧、内存、CPU监控
- 自动异常恢复

### 4. 性能分析
- 实时帧率统计
- 延迟测量
- CPU/内存监控

## 与项目2的区别

| 项目2 | 项目3 |
|-------|-------|
| **软件算法** | **硬件调优** |
| 灰度、Sobel边缘检测 | OV5640寄存器读写 |
| CPU运行图像处理算法 | 控制OV5640内置ISP |
| 应用层开发 | 驱动层+硬件层开发 |

## 文件结构

```
camera3项目/
├── app/
│   ├── Makefile
│   ├── camera_tuning_main.c      # 主程序
│   ├── v4l2_utils.c/h            # V4L2采集
│   ├── fb_utils.c/h              # Framebuffer显示
│   ├── ov5640_tuning.c/h         # OV5640寄存器调优（核心）
│   ├── stability_test.c/h        # 稳定性测试
│   └── perf_analysis.c/h         # 性能分析
├── scripts/
│   ├── hardware_check.sh         # 硬件能力检查
│   ├── tuning_test.sh            # 调优测试
│   ├── stability_run.sh          # 稳定性测试
│   └── perf_report.sh            # 性能测试
└── README.md
```

## 编译

```bash
cd app
make
```

## 运行模式

### 1. 预览模式
```bash
./camera_tuning -m preview
```

### 2. 调优模式
```bash
./camera_tuning -m tuning
```

交互命令：
- `e <值>` - 设置曝光 (十六进制)
- `g <值>` - 设置增益
- `s <值>` - 设置锐度
- `d <值>` - 设置降噪
- `a` - 批量测试
- `r` - 恢复默认
- `p` - 打印参数

### 3. 稳定性测试
```bash
./camera_tuning -m stability
```

### 4. 性能测试
```bash
./camera_tuning -m perf
```

## OV5640 关键寄存器

| 寄存器 | 功能 | 说明 |
|--------|------|------|
| 0x3500-0x3502 | 曝光 | 16位值 |
| 0x350A-0x350B | 增益 | 10位值 |
| 0x3400-0x3405 | 白平衡 | RGB增益 |
| 0x5308 | 锐度 | 8位值 |
| 0x5309 | 降噪 | 8位值 |
| 0x5300 | 饱和度 | 8位值 |

## 技术要点

1. **I2C通信**：通过 `/dev/i2c-1` 访问OV5640（地址0x3c）
2. **寄存器读写**：16位寄存器地址，8位数据
3. **硬件ISP**：OV5640内置ISP，通过寄存器控制
4. **V4L2采集**：使用RGB565格式，640x480分辨率
5. **零拷贝显示**：RGB565直接写入framebuffer，CPU占用极低

## 开发环境

- 硬件：正点原子i.MX6ULL阿尔法板 + OV5640摄像头
- 系统：Linux 4.1.15
- 工具链：arm-linux-gnueabihf-gcc
