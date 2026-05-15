/*
 * camera_tuning_main.c - 主程序
 * Camera + ISP + 图像质量调优 + 稳定性测试
 *
 * 架构:
 *   采集线程: V4L2 dequeue → ISP处理 → 存入ring buffer
 *   显示线程: 从ring buffer取帧 → 写入framebuffer
 *   调优线程: OV5640参数调整 + 采集测试图像
 *   监控线程: 性能统计 + 稳定性监控
 *
 * 编译:
 *   arm-linux-gnueabihf-gcc -O2 -pthread -o camera_tuning *.c -lm
 */

#include "v4l2_utils.h"
#include "fb_utils.h"
#include "ov5640_tuning.h"
#include "stability_test.h"
#include "perf_analysis.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

/* ========== 参数配置 ========== */
#define CAPTURE_W 640
#define CAPTURE_H 480
#define DISPLAY_W 1024
#define DISPLAY_H 600
#define RING_SIZE 8

/* 运行模式 */
typedef enum {
    MODE_PREVIEW,      /* 普通预览模式 */
    MODE_TUNING,       /* 图像调优模式 */
    MODE_STABILITY,    /* 稳定性测试模式 */
    MODE_PERF          /* 性能分析模式 */
} run_mode_t;

/* ========== Ring Buffer ========== */
typedef struct {
    uint8_t *data[RING_SIZE];
    size_t size[RING_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} ring_buffer_t;

static ring_buffer_t ring;
static volatile int quit = 0;
static run_mode_t run_mode = MODE_PREVIEW;

/* 全局模块 */
static ov5640_tuning_t ov5640;
static perf_context_t perf;

static int ring_init(ring_buffer_t *rb)
{
    memset(rb, 0, sizeof(*rb));
    for (int i = 0; i < RING_SIZE; i++) {
        rb->data[i] = malloc(CAPTURE_W * CAPTURE_H * 2);
        if (!rb->data[i])
            return -1;
    }
    pthread_mutex_init(&rb->lock, NULL);
    pthread_cond_init(&rb->cond, NULL);
    return 0;
}

static void ring_free(ring_buffer_t *rb)
{
    for (int i = 0; i < RING_SIZE; i++) {
        free(rb->data[i]);
        rb->data[i] = NULL;
    }
    pthread_mutex_destroy(&rb->lock);
    pthread_cond_destroy(&rb->cond);
}

static int ring_put(ring_buffer_t *rb, const void *data, size_t size)
{
    pthread_mutex_lock(&rb->lock);
    if (rb->count >= RING_SIZE) {
        rb->tail = (rb->tail + 1) % RING_SIZE;
        rb->count--;
    }
    memcpy(rb->data[rb->head], data, size);
    rb->size[rb->head] = size;
    rb->head = (rb->head + 1) % RING_SIZE;
    rb->count++;
    pthread_cond_signal(&rb->cond);
    pthread_mutex_unlock(&rb->lock);
    return 0;
}

static int ring_get(ring_buffer_t *rb, void *data, size_t *size)
{
    pthread_mutex_lock(&rb->lock);
    while (rb->count == 0)
        pthread_cond_wait(&rb->cond, &rb->lock);
    memcpy(data, rb->data[rb->tail], rb->size[rb->tail]);
    *size = rb->size[rb->tail];
    rb->tail = (rb->tail + 1) % RING_SIZE;
    rb->count--;
    pthread_mutex_unlock(&rb->lock);
    return 0;
}

/* ========== 信号处理 ========== */
static void sig_handler(int sig)
{
    (void)sig;
    quit = 1;
}

/* ========== 采集线程 ========== */
/* 直接传递V4L2采集的数据，不经过软件ISP处理 */
/* 图像质量由OV5640内部ISP通过寄存器调优实现 */
static void *capture_thread(void *arg)
{
    v4l2_camera_t *cam = (v4l2_camera_t *)arg;
    int frame_count = 0;
    int fps_count = 0;
    perf_timer_t timer;
    struct timespec fps_start, fps_now;

    clock_gettime(CLOCK_MONOTONIC, &fps_start);

    while (!quit) {
        void *data = NULL;
        size_t size = 0;

        perf_timer_start(&timer);
        int idx = v4l2_camera_get_frame(cam, &data, &size);
        double latency = perf_timer_stop(&timer);

        if (idx < 0) {
            if (quit) break;
            usleep(10000);
            continue;
        }

        /* 直接传递原始数据，不做软件ISP处理 */
        ring_put(&ring, data, size);
        v4l2_camera_put_frame(cam, idx);

        /* 更新性能统计 */
        perf_record_frame(&perf);
        perf_record_latency(&perf, latency);

        frame_count++;
        fps_count++;

        /* 每秒打印一次帧率 */
        clock_gettime(CLOCK_MONOTONIC, &fps_now);
        double elapsed = (fps_now.tv_sec - fps_start.tv_sec) +
                         (fps_now.tv_nsec - fps_start.tv_nsec) / 1e9;
        if (elapsed >= 1.0) {
            double fps = fps_count / elapsed;
            printf("[CAP] FPS: %.1f, Frames: %d, Latency: %.2f ms\n",
                   fps, frame_count, latency);
            fps_count = 0;
            fps_start = fps_now;
        }
    }

    printf("[CAP] thread exit, total %d frames\n", frame_count);
    return NULL;
}

/* ========== 显示线程 ========== */
static void *display_thread(void *arg)
{
    fb_context_t *fb = (fb_context_t *)arg;
    uint8_t *frame = malloc(CAPTURE_W * CAPTURE_H * 2);
    if (!frame)
        return NULL;
    size_t frame_size;
    int offset_x = (DISPLAY_W - CAPTURE_W) / 2;
    int offset_y = (DISPLAY_H - CAPTURE_H) / 2;
    int display_count = 0;
    int fps_count = 0;
    struct timespec fps_start, fps_now;

    clock_gettime(CLOCK_MONOTONIC, &fps_start);
    printf("[DISP] offset=(%d,%d), thread start\n", offset_x, offset_y);

    while (!quit) {
        ring_get(&ring, frame, &frame_size);
        (void)frame_size;

        /* RGB565直接复制到framebuffer，不需要格式转换 */
        /* 每行复制，处理居中显示 */
        uint16_t *src = (uint16_t *)frame;
        for (int y = 0; y < CAPTURE_H && (y + offset_y) < DISPLAY_H; y++) {
            uint16_t *dst = &fb->backbuf[(y + offset_y) * fb->width + offset_x];
            memcpy(dst, &src[y * CAPTURE_W], CAPTURE_W * 2);
        }

        fb_flush(fb);
        display_count++;
        fps_count++;

        /* 每秒打印一次帧率 */
        clock_gettime(CLOCK_MONOTONIC, &fps_now);
        double elapsed = (fps_now.tv_sec - fps_start.tv_sec) +
                         (fps_now.tv_nsec - fps_start.tv_nsec) / 1e9;
        if (elapsed >= 1.0) {
            double fps = fps_count / elapsed;
            printf("[DISP] FPS: %.1f, Frames: %d\n", fps, display_count);
            fps_count = 0;
            fps_start = fps_now;
        }
    }

    free(frame);
    printf("[DISP] thread exit, %d displayed\n", display_count);
    return NULL;
}

/* ========== 监控线程 ========== */
static void *monitor_thread(void *arg)
{
    (void)arg;

    while (!quit) {
        /* 更新性能统计 */
        perf_update_cpu(&perf);
        perf_update_memory(&perf);

        /* 打印统计 */
        if (run_mode == MODE_PERF) {
            perf_print_stats(&perf);
        }

        sleep(1);
    }

    return NULL;
}

/* ========== 调优线程 ========== */
static void *tuning_thread(void *arg)
{
    v4l2_camera_t *cam = (v4l2_camera_t *)arg;
    uint8_t *buf = malloc(CAPTURE_W * CAPTURE_H * 2);

    printf("[TUNING] Thread started\n");
    printf("[TUNING] Commands:\n");
    printf("[TUNING]   e <value>  - Set exposure (hex)\n");
    printf("[TUNING]   g <value>  - Set gain (hex)\n");
    printf("[TUNING]   s <value>  - Set sharpness (hex)\n");
    printf("[TUNING]   d <value>  - Set denoise (hex)\n");
    printf("[TUNING]   a          - Run auto tuning\n");
    printf("[TUNING]   r          - Restore defaults\n");
    printf("[TUNING]   p          - Print current params\n");
    printf("[TUNING]   q          - Quit\n");

    while (!quit) {
        char cmd[64];
        printf("[TUNING] > ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
            break;

        if (cmd[0] == 'q') {
            quit = 1;
            break;
        } else if (cmd[0] == 'e') {
            uint16_t val;
            if (sscanf(cmd + 1, "%hx", &val) == 1) {
                ov5640_set_exposure(&ov5640, val);
                printf("[TUNING] Exposure set to 0x%04X\n", val);
            }
        } else if (cmd[0] == 'g') {
            uint16_t val;
            if (sscanf(cmd + 1, "%hx", &val) == 1) {
                ov5640_set_gain(&ov5640, val);
                printf("[TUNING] Gain set to 0x%04X\n", val);
            }
        } else if (cmd[0] == 's') {
            uint8_t val;
            if (sscanf(cmd + 1, "%hhx", &val) == 1) {
                ov5640_set_sharpness(&ov5640, val);
                printf("[TUNING] Sharpness set to 0x%02X\n", val);
            }
        } else if (cmd[0] == 'd') {
            uint8_t val;
            if (sscanf(cmd + 1, "%hhx", &val) == 1) {
                ov5640_set_denoise(&ov5640, val);
                printf("[TUNING] Denoise set to 0x%02X\n", val);
            }
        } else if (cmd[0] == 'a') {
            printf("[TUNING] Running auto tuning...\n");
            /* 批量测试 */
            ov5640_batch_tuning(&ov5640, "tuning_test",
                               NULL, NULL, NULL);
        } else if (cmd[0] == 'r') {
            ov5640_restore_defaults(&ov5640);
            printf("[TUNING] Defaults restored\n");
        } else if (cmd[0] == 'p') {
            ov5640_print_params(&ov5640);
        }
    }

    free(buf);
    printf("[TUNING] Thread exit\n");
    return NULL;
}

/* ========== 稳定性测试回调函数 ========== */
static int my_stability_init(void *user_data)
{
    (void)user_data;
    printf("[STABILITY] Init\n");
    return 0;
}

static int my_stability_capture(void *user_data, uint8_t *buf, size_t size, double *latency_ms)
{
    v4l2_camera_t *cam = (v4l2_camera_t *)user_data;
    perf_timer_t timer;

    perf_timer_start(&timer);
    void *data = NULL;
    size_t frame_size = 0;
    int idx = v4l2_camera_get_frame(cam, &data, &frame_size);
    double latency = perf_timer_stop(&timer);

    if (idx < 0) {
        *latency_ms = latency;
        return -1;
    }

    if (frame_size > size) frame_size = size;
    memcpy(buf, data, frame_size);
    v4l2_camera_put_frame(cam, idx);

    *latency_ms = latency;
    return 0;
}

static void my_stability_cleanup(void *user_data)
{
    (void)user_data;
    printf("[STABILITY] Cleanup\n");
}

/* ========== 主函数 ========== */
int main(int argc, char *argv[])
{
    v4l2_camera_t cam;
    fb_context_t fb;
    pthread_t cap_tid, disp_tid, mon_tid, tun_tid;
    stability_context_t stability;

    /* 解析命令行参数 */
    int opt;
    while ((opt = getopt(argc, argv, "m:")) != -1) {
        switch (opt) {
            case 'm':
                if (strcmp(optarg, "tuning") == 0)
                    run_mode = MODE_TUNING;
                else if (strcmp(optarg, "stability") == 0)
                    run_mode = MODE_STABILITY;
                else if (strcmp(optarg, "perf") == 0)
                    run_mode = MODE_PERF;
                else
                    run_mode = MODE_PREVIEW;
                break;
            default:
                fprintf(stderr, "Usage: %s [-m preview|tuning|stability|perf]\n", argv[0]);
                return 1;
        }
    }

    printf("========================================\n");
    printf("  i.MX6ULL Camera Tuning & Stability Test\n");
    printf("  Mode: %s\n",
           run_mode == MODE_PREVIEW ? "PREVIEW" :
           run_mode == MODE_TUNING ? "TUNING" :
           run_mode == MODE_STABILITY ? "STABILITY" : "PERF");
    printf("========================================\n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* 初始化模块 */
    if (fb_init(&fb, "/dev/fb0", 16) < 0) {
        fprintf(stderr, "FB init failed\n");
        return 1;
    }

    if (v4l2_camera_init(&cam, "/dev/video1", CAPTURE_W, CAPTURE_H,
                         CAM_PIXFMT, 4) < 0) {
        fprintf(stderr, "Camera init failed\n");
        fb_close(&fb);
        return 1;
    }

    if (ring_init(&ring) < 0) {
        fprintf(stderr, "Ring buffer init failed\n");
        v4l2_camera_close(&cam);
        fb_close(&fb);
        return 1;
    }

    /* 初始化OV5640调优模块 */
    if (ov5640_tuning_init(&ov5640, "/dev/i2c-1") < 0) {
        fprintf(stderr, "OV5640 tuning init failed (may not be available)\n");
    }

    /* 初始化性能分析 */
    perf_init(&perf);

    v4l2_camera_start(&cam);

    /* 启动线程 */
    pthread_create(&cap_tid, NULL, capture_thread, &cam);
    pthread_create(&disp_tid, NULL, display_thread, &fb);
    pthread_create(&mon_tid, NULL, monitor_thread, NULL);

    if (run_mode == MODE_TUNING) {
        pthread_create(&tun_tid, NULL, tuning_thread, &cam);
    }

    if (run_mode == MODE_STABILITY) {
        /* 运行稳定性测试 */
        stability_config_t config = {
            .duration_hours = 72.0,
            .report_interval = 60,
            .max_dropped = 1000,
            .max_latency_ms = 100.0,
            .enable_restart = 1,
            .log_file = "stability.log",
            .report_file = "stability_report.md"
        };

        stability_init(&stability, &config);
        stability_run(&stability, my_stability_init, my_stability_capture,
                     my_stability_cleanup, &cam);
    }

    /* 等待退出 */
    while (!quit) {
        sleep(1);
    }

    /* 清理 */
    pthread_join(cap_tid, NULL);
    pthread_join(disp_tid, NULL);
    pthread_join(mon_tid, NULL);

    if (run_mode == MODE_TUNING) {
        pthread_join(tun_tid, NULL);
    }

    /* 生成报告 */
    if (run_mode == MODE_PERF) {
        perf_generate_report(&perf, "perf_report.md");
    }

    ov5640_tuning_close(&ov5640);
    v4l2_camera_close(&cam);
    ring_free(&ring);
    fb_close(&fb);

    printf("[MAIN] exit cleanly.\n");
    return 0;
}
