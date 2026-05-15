# 项目3：Camera + ISP + 图像质量调优 + 稳定性测试

## 基于项目2的架构，添加以下功能：

### 1. 图像质量调优模块 (ov5640_tuning.c/h)
- 通过I2C读写OV5640寄存器
- 调整参数：曝光、白平衡、锐度、降噪、饱和度
- 批量测试不同参数组合
- 输出调优报告（参数对比图）

### 2. ISP处理模块 (isp_processing.c/h)
- 色彩校正矩阵（CCM）
- Gamma校正
- 自动曝光/白平衡算法（AE/AWB）

### 3. 稳定性测试框架 (stability_test.c/h)
- 连续采集72小时测试
- 记录：帧率、丢帧数、内存使用、CPU占用
- 异常检测和恢复
- 日志系统

### 4. 性能分析工具 (perf_analysis.c/h)
- 帧率统计
- 延迟测量
- 内存泄漏检测
- CPU热点分析

### 5. 主程序 (camera_tuning_main.c)
- 整合所有模块
- 支持多种运行模式：
  - tuning: 图像调优模式
  - stability: 稳定性测试模式
  - perf: 性能分析模式

## 文件结构：
```
camera3项目/
├── app/
│   ├── Makefile
│   ├── camera_tuning_main.c
│   ├── v4l2_utils.c/h      (复用项目2)
│   ├── fb_utils.c/h        (复用项目2)
│   ├── imgproc.c/h         (复用项目2)
│   ├── ov5640_tuning.c/h   (新增)
│   ├── isp_processing.c/h  (新增)
│   ├── stability_test.c/h  (新增)
│   └── perf_analysis.c/h   (新增)
├── scripts/
│   ├── tuning_test.sh      (批量调优脚本)
│   ├── stability_run.sh    (稳定性测试脚本)
│   └── perf_report.sh      (性能报告脚本)
└── docs/
    ├── tuning_report.md    (调优报告)
    └── stability_report.md (稳定性报告)
```

## 开发顺序：
1. 先复用项目2的基础代码
2. 实现OV5640寄存器读写
3. 实现图像调优功能
4. 实现稳定性测试框架
5. 实现性能分析工具
6. 整合测试
