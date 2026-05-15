/*
 * stability_test.c - 稳定性测试框架实现
 * 连续采集测试、性能监控、异常检测
 */

#include "stability_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

/* 获取当前时间(秒) */
static double get_time_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* 获取内存使用 */
size_t stability_get_memory_usage(void)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return 0;

    char line[256];
    size_t vmrss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "VmRSS: %zu kB", &vmrss) == 1) {
            vmrss *= 1024; /* 转换为bytes */
            break;
        }
    }

    fclose(fp);
    return vmrss;
}

/* 获取CPU使用率 */
double stability_get_cpu_usage(void)
{
    static long last_total = 0;
    static long last_idle = 0;

    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;

    char line[256];
    long user, nice, system, idle, iowait, irq, softirq, steal;

    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    }

    fclose(fp);

    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long total_diff = total - last_total;
    long idle_diff = idle - last_idle;

    last_total = total;
    last_idle = idle;

    if (total_diff == 0) return 0.0;

    return 100.0 * (1.0 - (double)idle_diff / total_diff);
}

int stability_init(stability_context_t *ctx, const stability_config_t *config)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *config;
    ctx->start_time = time(NULL);
    ctx->last_report_time = ctx->start_time;
    ctx->running = 0;

    /* 打开日志文件 */
    if (config->log_file) {
        ctx->log_fp = fopen(config->log_file, "a");
        if (!ctx->log_fp) {
            perror("Failed to open log file");
            return -1;
        }
    }

    /* 打开报告文件 */
    if (config->report_file) {
        ctx->report_fp = fopen(config->report_file, "w");
        if (!ctx->report_fp) {
            perror("Failed to open report file");
            if (ctx->log_fp) fclose(ctx->log_fp);
            return -1;
        }
    }

    /* 初始化统计 */
    ctx->stats.min_fps = 999999.0;
    ctx->stats.max_fps = 0.0;
    ctx->stats.max_latency = 0.0;

    stability_log(ctx, "INFO", "Stability test initialized");
    stability_log(ctx, "INFO", "Duration: %.1f hours", config->duration_hours);
    stability_log(ctx, "INFO", "Report interval: %d seconds", config->report_interval);

    return 0;
}

void stability_close(stability_context_t *ctx)
{
    if (ctx->log_fp) {
        fclose(ctx->log_fp);
        ctx->log_fp = NULL;
    }
    if (ctx->report_fp) {
        fclose(ctx->report_fp);
        ctx->report_fp = NULL;
    }
}

void stability_record_frame(stability_context_t *ctx, double latency_ms,
                           int dropped, int error)
{
    ctx->stats.total_frames++;

    if (dropped) ctx->stats.dropped_frames++;
    if (error) ctx->stats.error_frames++;

    /* 更新延迟统计 */
    if (latency_ms > ctx->stats.max_latency) {
        ctx->stats.max_latency = latency_ms;
    }

    /* 更新内存统计 */
    size_t current_memory = stability_get_memory_usage();
    if (current_memory > ctx->stats.peak_memory) {
        ctx->stats.peak_memory = current_memory;
    }

    /* 定期打印统计 */
    if (ctx->stats.total_frames % 100 == 0) {
        double elapsed = get_time_sec() - ctx->start_time;
        double current_fps = ctx->stats.total_frames / elapsed;

        if (current_fps < ctx->stats.min_fps) ctx->stats.min_fps = current_fps;
        if (current_fps > ctx->stats.max_fps) ctx->stats.max_fps = current_fps;

        stability_print_stats(ctx);
    }
}

void stability_print_stats(const stability_context_t *ctx)
{
    double elapsed = get_time_sec() - ctx->start_time;
    double current_fps = ctx->stats.total_frames / elapsed;

    printf("[STABILITY] Frames: %lu, Dropped: %lu, Errors: %lu, FPS: %.1f, "
           "Memory: %zu KB, CPU: %.1f%%\n",
           ctx->stats.total_frames, ctx->stats.dropped_frames,
           ctx->stats.error_frames, current_fps,
           ctx->stats.peak_memory / 1024, stability_get_cpu_usage());
}

void stability_log(stability_context_t *ctx, const char *level, const char *fmt, ...)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    va_list args;
    va_start(args, fmt);

    /* 输出到控制台 */
    printf("[%s] [%s] ", timestamp, level);
    vprintf(fmt, args);
    printf("\n");

    /* 输出到日志文件 */
    if (ctx->log_fp) {
        fprintf(ctx->log_fp, "[%s] [%s] ", timestamp, level);
        vfprintf(ctx->log_fp, fmt, args);
        fprintf(ctx->log_fp, "\n");
        fflush(ctx->log_fp);
    }

    va_end(args);
}

int stability_should_stop(const stability_context_t *ctx)
{
    double elapsed = get_time_sec() - ctx->start_time;
    double duration_sec = ctx->config.duration_hours * 3600.0;

    /* 检查时间 */
    if (elapsed >= duration_sec) {
        return 1;
    }

    /* 检查丢帧数 */
    if (ctx->config.max_dropped > 0 &&
        ctx->stats.dropped_frames >= ctx->config.max_dropped) {
        return 1;
    }

    return 0;
}

void stability_generate_report(stability_context_t *ctx)
{
    double elapsed = get_time_sec() - ctx->start_time;

    /* 计算平均值 */
    ctx->stats.total_time = elapsed;
    ctx->stats.avg_fps = ctx->stats.total_frames / elapsed;
    ctx->stats.avg_cpu_usage = stability_get_cpu_usage();

    /* 打印报告 */
    printf("\n========================================\n");
    printf("  Stability Test Report\n");
    printf("========================================\n");
    printf("  Duration:        %.2f hours\n", elapsed / 3600.0);
    printf("  Total Frames:    %lu\n", ctx->stats.total_frames);
    printf("  Dropped Frames:  %lu (%.2f%%)\n",
           ctx->stats.dropped_frames,
           100.0 * ctx->stats.dropped_frames / ctx->stats.total_frames);
    printf("  Error Frames:    %lu\n", ctx->stats.error_frames);
    printf("  Average FPS:     %.2f\n", ctx->stats.avg_fps);
    printf("  Min FPS:         %.2f\n", ctx->stats.min_fps);
    printf("  Max FPS:         %.2f\n", ctx->stats.max_fps);
    printf("  Max Latency:     %.2f ms\n", ctx->stats.max_latency);
    printf("  Peak Memory:     %zu KB\n", ctx->stats.peak_memory / 1024);
    printf("  Average CPU:     %.1f%%\n", ctx->stats.avg_cpu_usage);
    printf("  Restart Count:   %d\n", ctx->stats.restart_count);
    printf("========================================\n");

    /* 写入报告文件 */
    if (ctx->report_fp) {
        fprintf(ctx->report_fp, "# Stability Test Report\n\n");
        fprintf(ctx->report_fp, "## Summary\n\n");
        fprintf(ctx->report_fp, "- Duration: %.2f hours\n", elapsed / 3600.0);
        fprintf(ctx->report_fp, "- Total Frames: %lu\n", ctx->stats.total_frames);
        fprintf(ctx->report_fp, "- Dropped Frames: %lu (%.2f%%)\n",
                ctx->stats.dropped_frames,
                100.0 * ctx->stats.dropped_frames / ctx->stats.total_frames);
        fprintf(ctx->report_fp, "- Error Frames: %lu\n", ctx->stats.error_frames);
        fprintf(ctx->report_fp, "- Average FPS: %.2f\n", ctx->stats.avg_fps);
        fprintf(ctx->report_fp, "- Min FPS: %.2f\n", ctx->stats.min_fps);
        fprintf(ctx->report_fp, "- Max FPS: %.2f\n", ctx->stats.max_fps);
        fprintf(ctx->report_fp, "- Max Latency: %.2f ms\n", ctx->stats.max_latency);
        fprintf(ctx->report_fp, "- Peak Memory: %zu KB\n", ctx->stats.peak_memory / 1024);
        fprintf(ctx->report_fp, "- Average CPU: %.1f%%\n", ctx->stats.avg_cpu_usage);
        fprintf(ctx->report_fp, "- Restart Count: %d\n", ctx->stats.restart_count);

        /* 判定结果 */
        fprintf(ctx->report_fp, "\n## Result\n\n");
        if (ctx->stats.dropped_frames == 0 && ctx->stats.error_frames == 0) {
            fprintf(ctx->report_fp, "**PASS** - No dropped or error frames\n");
        } else if (ctx->stats.dropped_frames < ctx->stats.total_frames * 0.01) {
            fprintf(ctx->report_fp, "**PASS** - Dropped frames < 1%%\n");
        } else {
            fprintf(ctx->report_fp, "**FAIL** - Too many dropped/error frames\n");
        }

        fflush(ctx->report_fp);
    }

    stability_log(ctx, "INFO", "Report generated");
}

int stability_run(stability_context_t *ctx,
                  stability_init_func init_func,
                  stability_capture_func capture_func,
                  stability_cleanup_func cleanup_func,
                  void *user_data)
{
    stability_log(ctx, "INFO", "Starting stability test");

    /* 初始化 */
    if (init_func && init_func(user_data) != 0) {
        stability_log(ctx, "ERROR", "Initialization failed");
        return -1;
    }

    ctx->running = 1;
    uint8_t *buf = malloc(640 * 480 * 2);
    if (!buf) {
        stability_log(ctx, "ERROR", "Failed to allocate buffer");
        return -1;
    }

    /* 主测试循环 */
    while (ctx->running && !stability_should_stop(ctx)) {
        double latency_ms = 0;
        int ret = capture_func(user_data, buf, 640 * 480 * 2, &latency_ms);

        if (ret < 0) {
            stability_record_frame(ctx, latency_ms, 0, 1);
            stability_log(ctx, "WARN", "Capture error");

            if (ctx->config.enable_restart) {
                stability_log(ctx, "INFO", "Attempting restart");
                if (cleanup_func) cleanup_func(user_data);
                usleep(1000000);
                if (init_func && init_func(user_data) == 0) {
                    ctx->stats.restart_count++;
                    stability_log(ctx, "INFO", "Restart successful");
                }
            }
        } else if (ret == 0) {
            stability_record_frame(ctx, latency_ms, 0, 0);
        } else {
            stability_record_frame(ctx, latency_ms, 1, 0);
        }

        /* 检查是否需要生成报告 */
        time_t now = time(NULL);
        if (now - ctx->last_report_time >= ctx->config.report_interval) {
            stability_print_stats(ctx);
            ctx->last_report_time = now;
        }
    }

    free(buf);

    /* 清理 */
    if (cleanup_func) cleanup_func(user_data);

    /* 生成最终报告 */
    stability_generate_report(ctx);

    stability_log(ctx, "INFO", "Stability test completed");
    return 0;
}

void stability_stop(stability_context_t *ctx)
{
    ctx->running = 0;
    stability_log(ctx, "INFO", "Stopping stability test");
}
