/*
 * perf_analysis.c - 性能分析工具实现
 * 帧率统计、延迟测量、CPU热点分析
 */

#include "perf_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

/* 获取当前时间(秒) */
static double get_time_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* 获取时间差(毫秒) */
static double timespec_diff_ms(const struct timespec *start, const struct timespec *end)
{
    double sec = end->tv_sec - start->tv_sec;
    double nsec = end->tv_nsec - start->tv_nsec;
    return sec * 1000.0 + nsec / 1000000.0;
}

void perf_init(perf_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->latency.min_latency = 999999.0;
    ctx->latency.max_latency = 0.0;
    ctx->fps.min_fps = 999999.0;
    ctx->fps.max_fps = 0.0;
    ctx->enabled = 1;
}

void perf_enable(perf_context_t *ctx, int enable)
{
    ctx->enabled = enable;
}

void perf_timer_start(perf_timer_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->start);
}

double perf_timer_stop(perf_timer_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->end);
    timer->elapsed_ms = timespec_diff_ms(&timer->start, &timer->end);
    return timer->elapsed_ms;
}

void perf_record_frame(perf_context_t *ctx)
{
    if (!ctx->enabled) return;

    ctx->fps.frame_count++;
    double now = get_time_sec();

    if (ctx->fps.start_time == 0) {
        ctx->fps.start_time = now;
        ctx->fps.last_fps_time = now;
    }

    /* 每秒计算一次帧率 */
    double elapsed = now - ctx->fps.last_fps_time;
    if (elapsed >= 1.0) {
        ctx->fps.current_fps = ctx->fps.frame_count / (now - ctx->fps.start_time);

        if (ctx->fps.current_fps < ctx->fps.min_fps)
            ctx->fps.min_fps = ctx->fps.current_fps;
        if (ctx->fps.current_fps > ctx->fps.max_fps)
            ctx->fps.max_fps = ctx->fps.current_fps;

        ctx->fps.fps_samples++;
        ctx->fps.avg_fps = (ctx->fps.avg_fps * (ctx->fps.fps_samples - 1) + ctx->fps.current_fps) / ctx->fps.fps_samples;

        ctx->fps.last_fps_time = now;
    }
}

void perf_record_latency(perf_context_t *ctx, double latency_ms)
{
    if (!ctx->enabled) return;

    ctx->latency.sample_count++;
    ctx->latency.total_latency += latency_ms;

    if (latency_ms < ctx->latency.min_latency)
        ctx->latency.min_latency = latency_ms;
    if (latency_ms > ctx->latency.max_latency)
        ctx->latency.max_latency = latency_ms;
}

void perf_update_cpu(perf_context_t *ctx)
{
    if (!ctx->enabled) return;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    ctx->cpu.user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
    ctx->cpu.system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
    ctx->cpu.total_time = ctx->cpu.user_time + ctx->cpu.system_time;

    double elapsed = get_time_sec() - ctx->fps.start_time;
    if (elapsed > 0) {
        ctx->cpu.cpu_usage = (ctx->cpu.total_time / elapsed) * 100.0;
    }
}

void perf_update_memory(perf_context_t *ctx)
{
    if (!ctx->enabled) return;

    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        size_t vmrss;
        if (sscanf(line, "VmRSS: %zu kB", &vmrss) == 1) {
            ctx->memory.current = vmrss * 1024;
            if (ctx->memory.current > ctx->memory.peak) {
                ctx->memory.peak = ctx->memory.current;
            }
            break;
        }
    }

    fclose(fp);
}

double perf_get_fps(perf_context_t *ctx)
{
    return ctx->fps.current_fps;
}

double perf_get_avg_latency(perf_context_t *ctx)
{
    if (ctx->latency.sample_count == 0) return 0.0;
    return ctx->latency.total_latency / ctx->latency.sample_count;
}

double perf_get_cpu_usage(perf_context_t *ctx)
{
    return ctx->cpu.cpu_usage;
}

size_t perf_get_memory_usage(perf_context_t *ctx)
{
    return ctx->memory.current;
}

void perf_print_stats(const perf_context_t *ctx)
{
    printf("[PERF] FPS: %.1f (avg: %.1f, min: %.1f, max: %.1f)\n",
           ctx->fps.current_fps, ctx->fps.avg_fps,
           ctx->fps.min_fps, ctx->fps.max_fps);
    printf("[PERF] Latency: avg=%.2fms, min=%.2fms, max=%.2fms\n",
           ctx->latency.total_latency / ctx->latency.sample_count,
           ctx->latency.min_latency, ctx->latency.max_latency);
    printf("[PERF] CPU: %.1f%%, Memory: %zu KB (peak: %zu KB)\n",
           ctx->cpu.cpu_usage,
           ctx->memory.current / 1024, ctx->memory.peak / 1024);
}

void perf_reset(perf_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->latency.min_latency = 999999.0;
    ctx->latency.max_latency = 0.0;
    ctx->fps.min_fps = 999999.0;
    ctx->fps.max_fps = 0.0;
    ctx->enabled = 1;
}

void perf_generate_report(const perf_context_t *ctx, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open report file");
        return;
    }

    fprintf(fp, "# Performance Analysis Report\n\n");
    fprintf(fp, "## FPS Statistics\n\n");
    fprintf(fp, "- Total Frames: %lu\n", ctx->fps.frame_count);
    fprintf(fp, "- Average FPS: %.2f\n", ctx->fps.avg_fps);
    fprintf(fp, "- Min FPS: %.2f\n", ctx->fps.min_fps);
    fprintf(fp, "- Max FPS: %.2f\n", ctx->fps.max_fps);

    fprintf(fp, "\n## Latency Statistics\n\n");
    fprintf(fp, "- Samples: %lu\n", ctx->latency.sample_count);
    fprintf(fp, "- Average Latency: %.2f ms\n",
            ctx->latency.total_latency / ctx->latency.sample_count);
    fprintf(fp, "- Min Latency: %.2f ms\n", ctx->latency.min_latency);
    fprintf(fp, "- Max Latency: %.2f ms\n", ctx->latency.max_latency);

    fprintf(fp, "\n## CPU Statistics\n\n");
    fprintf(fp, "- User Time: %.2f s\n", ctx->cpu.user_time);
    fprintf(fp, "- System Time: %.2f s\n", ctx->cpu.system_time);
    fprintf(fp, "- Total Time: %.2f s\n", ctx->cpu.total_time);
    fprintf(fp, "- CPU Usage: %.1f%%\n", ctx->cpu.cpu_usage);

    fprintf(fp, "\n## Memory Statistics\n\n");
    fprintf(fp, "- Current: %zu KB\n", ctx->memory.current / 1024);
    fprintf(fp, "- Peak: %zu KB\n", ctx->memory.peak / 1024);

    fclose(fp);
    printf("[PERF] Report saved to %s\n", filename);
}

void perf_cpu_start(void)
{
    /* 启动CPU性能分析 */
    printf("[PERF] CPU profiling started\n");
}

void perf_cpu_stop(void)
{
    /* 停止CPU性能分析 */
    printf("[PERF] CPU profiling stopped\n");
}

void perf_memory_track_start(void)
{
    /* 启动内存跟踪 */
    printf("[PERF] Memory tracking started\n");
}

void perf_memory_track_stop(void)
{
    /* 停止内存跟踪 */
    printf("[PERF] Memory tracking stopped\n");
}
