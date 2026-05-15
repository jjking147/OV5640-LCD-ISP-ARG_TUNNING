/*
 * perf_analysis.h - 性能分析工具
 * 帧率统计、延迟测量、CPU热点分析
 */

#ifndef PERF_ANALYSIS_H
#define PERF_ANALYSIS_H

#include <stdint.h>
#include <time.h>
#include <stdio.h>

/* 性能计时器 */
typedef struct {
    struct timespec start;
    struct timespec end;
    double elapsed_ms;
} perf_timer_t;

/* 帧率统计 */
typedef struct {
    uint64_t frame_count;
    double   start_time;
    double   last_fps_time;
    double   current_fps;
    double   avg_fps;
    double   min_fps;
    double   max_fps;
    uint64_t fps_samples;
} fps_stats_t;

/* 延迟统计 */
typedef struct {
    double   min_latency;
    double   max_latency;
    double   total_latency;
    uint64_t sample_count;
} latency_stats_t;

/* CPU统计 */
typedef struct {
    double   user_time;
    double   system_time;
    double   total_time;
    double   cpu_usage;
} cpu_stats_t;

/* 内存统计 */
typedef struct {
    size_t   current;
    size_t   peak;
    size_t   total_alloc;
    size_t   total_free;
    int      alloc_count;
    int      free_count;
} memory_stats_t;

/* 性能分析上下文 */
typedef struct {
    fps_stats_t     fps;
    latency_stats_t latency;
    cpu_stats_t     cpu;
    memory_stats_t  memory;
    int             enabled;
} perf_context_t;

/* 函数声明 */

/**
 * 初始化性能分析
 * @param ctx 性能上下文
 */
void perf_init(perf_context_t *ctx);

/**
 * 使能/禁用性能分析
 * @param ctx 性能上下文
 * @param enable 1使能，0禁用
 */
void perf_enable(perf_context_t *ctx, int enable);

/**
 * 开始计时
 * @param timer 计时器
 */
void perf_timer_start(perf_timer_t *timer);

/**
 * 停止计时
 * @param timer 计时器
 * @return 毫秒
 */
double perf_timer_stop(perf_timer_t *timer);

/**
 * 记录一帧
 * @param ctx 性能上下文
 */
void perf_record_frame(perf_context_t *ctx);

/**
 * 记录延迟
 * @param ctx 性能上下文
 * @param latency_ms 延迟(毫秒)
 */
void perf_record_latency(perf_context_t *ctx, double latency_ms);

/**
 * 更新CPU统计
 * @param ctx 性能上下文
 */
void perf_update_cpu(perf_context_t *ctx);

/**
 * 更新内存统计
 * @param ctx 性能上下文
 */
void perf_update_memory(perf_context_t *ctx);

/**
 * 获取当前帧率
 * @param ctx 性能上下文
 * @return 帧率
 */
double perf_get_fps(perf_context_t *ctx);

/**
 * 获取平均延迟
 * @param ctx 性能上下文
 * @return 平均延迟(毫秒)
 */
double perf_get_avg_latency(perf_context_t *ctx);

/**
 * 获取CPU使用率
 * @param ctx 性能上下文
 * @return CPU使用率(%)
 */
double perf_get_cpu_usage(perf_context_t *ctx);

/**
 * 获取内存使用
 * @param ctx 性能上下文
 * @return 内存使用(bytes)
 */
size_t perf_get_memory_usage(perf_context_t *ctx);

/**
 * 打印性能统计
 * @param ctx 性能上下文
 */
void perf_print_stats(const perf_context_t *ctx);

/**
 * 重置性能统计
 * @param ctx 性能上下文
 */
void perf_reset(perf_context_t *ctx);

/**
 * 生成性能报告
 * @param ctx 性能上下文
 * @param filename 报告文件名
 */
void perf_generate_report(const perf_context_t *ctx, const char *filename);

/**
 * 开始CPU性能分析
 */
void perf_cpu_start(void);

/**
 * 停止CPU性能分析
 */
void perf_cpu_stop(void);

/**
 * 开始内存跟踪
 */
void perf_memory_track_start(void);

/**
 * 停止内存跟踪
 */
void perf_memory_track_stop(void);

#endif /* PERF_ANALYSIS_H */
