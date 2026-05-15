/*
 * stability_test.h - 稳定性测试框架
 * 连续采集测试、性能监控、异常检测
 */

#ifndef STABILITY_TEST_H
#define STABILITY_TEST_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* 性能统计结构体 */
typedef struct {
    uint64_t total_frames;      /* 总帧数 */
    uint64_t dropped_frames;    /* 丢帧数 */
    uint64_t error_frames;      /* 错误帧数 */
    double   total_time;        /* 总运行时间(秒) */
    double   avg_fps;           /* 平均帧率 */
    double   min_fps;           /* 最小帧率 */
    double   max_fps;           /* 最大帧率 */
    double   avg_latency;       /* 平均延迟(ms) */
    double   max_latency;       /* 最大延迟(ms) */
    size_t   peak_memory;       /* 峰值内存使用(bytes) */
    double   avg_cpu_usage;     /* 平均CPU使用率(%) */
    int      restart_count;     /* 重启次数 */
} perf_stats_t;

/* 测试配置 */
typedef struct {
    double   duration_hours;    /* 测试时长(小时) */
    int      report_interval;   /* 报告间隔(秒) */
    int      max_dropped;       /* 最大允许丢帧数 */
    double   max_latency_ms;   /* 最大允许延迟(ms) */
    int      enable_restart;    /* 使能自动重启 */
    const char *log_file;      /* 日志文件路径 */
    const char *report_file;   /* 报告文件路径 */
} stability_config_t;

/* 测试上下文 */
typedef struct {
    stability_config_t config;
    perf_stats_t stats;
    time_t start_time;
    time_t last_report_time;
    FILE *log_fp;
    FILE *report_fp;
    int running;
} stability_context_t;

/* 回调函数类型 */
typedef int (*stability_capture_func)(void *user_data, uint8_t *buf, size_t size, double *latency_ms);
typedef int (*stability_init_func)(void *user_data);
typedef void (*stability_cleanup_func)(void *user_data);

/* 函数声明 */

/**
 * 初始化稳定性测试
 * @param ctx 测试上下文
 * @param config 测试配置
 * @return 0成功，-1失败
 */
int stability_init(stability_context_t *ctx, const stability_config_t *config);

/**
 * 关闭稳定性测试
 * @param ctx 测试上下文
 */
void stability_close(stability_context_t *ctx);

/**
 * 运行稳定性测试
 * @param ctx 测试上下文
 * @param init_func 初始化回调
 * @param capture_func 采集回调
 * @param cleanup_func 清理回调
 * @param user_data 用户数据
 * @return 0成功，-1失败
 */
int stability_run(stability_context_t *ctx,
                  stability_init_func init_func,
                  stability_capture_func capture_func,
                  stability_cleanup_func cleanup_func,
                  void *user_data);

/**
 * 停止稳定性测试
 * @param ctx 测试上下文
 */
void stability_stop(stability_context_t *ctx);

/**
 * 记录一帧
 * @param ctx 测试上下文
 * @param latency_ms 延迟(毫秒)
 * @param dropped 是否丢帧
 * @param error 是否错误
 */
void stability_record_frame(stability_context_t *ctx, double latency_ms,
                           int dropped, int error);

/**
 * 生成性能报告
 * @param ctx 测试上下文
 */
void stability_generate_report(stability_context_t *ctx);

/**
 * 打印当前统计
 * @param ctx 测试上下文
 */
void stability_print_stats(const stability_context_t *ctx);

/**
 * 获取当前内存使用
 * @return 内存使用(bytes)
 */
size_t stability_get_memory_usage(void);

/**
 * 获取当前CPU使用率
 * @return CPU使用率(%)
 */
double stability_get_cpu_usage(void);

/**
 * 记录日志
 * @param ctx 测试上下文
 * @param level 日志级别
 * @param fmt 格式字符串
 */
void stability_log(stability_context_t *ctx, const char *level, const char *fmt, ...);

/**
 * 检查是否应该停止
 * @param ctx 测试上下文
 * @return 1应该停止，0继续
 */
int stability_should_stop(const stability_context_t *ctx);

#endif /* STABILITY_TEST_H */
