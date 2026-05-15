/*
 * ov5640_tuning.c - OV5640 图像质量调优模块实现
 * 通过I2C读写OV5640寄存器，调整图像参数
 */

#include "ov5640_tuning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <time.h>

/* OV5640寄存器地址 */
#define OV5640_REG_EXPOSURE_H   0x3500
#define OV5640_REG_EXPOSURE_M   0x3501
#define OV5640_REG_EXPOSURE_L   0x3502
#define OV5640_REG_GAIN_H       0x350A
#define OV5640_REG_GAIN_L       0x350B
#define OV5640_REG_WB_R_H       0x3400
#define OV5640_REG_WB_R_L       0x3401
#define OV5640_REG_WB_G_H       0x3402
#define OV5640_REG_WB_G_L       0x3403
#define OV5640_REG_WB_B_H       0x3404
#define OV5640_REG_WB_B_L       0x3405
#define OV5640_REG_SHARPNESS    0x5308
#define OV5640_REG_DENOISE      0x5309
#define OV5640_REG_SATURATION   0x5300
#define OV5640_REG_BRIGHTNESS   0x5587
#define OV5640_REG_CONTRAST     0x5300

/* 默认参数 */
static const ov5640_params_t default_params = {
    .exposure = 0x0400,
    .gain = 0x0010,
    .wb_r = 0x0400,
    .wb_g = 0x0400,
    .wb_b = 0x0400,
    .sharpness = 0x20,
    .denoise = 0x20,
    .saturation = 0x80,
    .brightness = 0x80,
    .contrast = 0x80
};

/* I2C读写辅助函数 */
static int i2c_read_reg(int fd, uint8_t addr, uint16_t reg, uint8_t *value)
{
    uint8_t reg_buf[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    uint8_t data;

    struct i2c_msg msgs[2] = {
        {
            .addr = addr,
            .flags = 0,
            .len = 2,
            .buf = reg_buf
        },
        {
            .addr = addr,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &data
        }
    };

    struct i2c_rdwr_ioctl_data msgset = {
        .msgs = msgs,
        .nmsgs = 2
    };

    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        perror("I2C read failed");
        return -1;
    }

    *value = data;
    return 0;
}

static int i2c_write_reg(int fd, uint8_t addr, uint16_t reg, uint8_t value)
{
    uint8_t buf[3] = {(reg >> 8) & 0xFF, reg & 0xFF, value};

    struct i2c_msg msg = {
        .addr = addr,
        .flags = 0,
        .len = 3,
        .buf = buf
    };

    struct i2c_rdwr_ioctl_data msgset = {
        .msgs = &msg,
        .nmsgs = 1
    };

    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        perror("I2C write failed");
        return -1;
    }

    return 0;
}

int ov5640_tuning_init(ov5640_tuning_t *ctx, const char *i2c_dev)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->i2c_fd = open(i2c_dev, O_RDWR);
    if (ctx->i2c_fd < 0) {
        perror("Failed to open I2C device");
        return -1;
    }

    ctx->default_params = default_params;
    ctx->params = default_params;

    printf("[OV5640] Initialized on %s\n", i2c_dev);
    return 0;
}

void ov5640_tuning_close(ov5640_tuning_t *ctx)
{
    if (ctx->i2c_fd >= 0) {
        close(ctx->i2c_fd);
        ctx->i2c_fd = -1;
    }
}

int ov5640_read_reg(ov5640_tuning_t *ctx, uint16_t reg, uint8_t *value)
{
    return i2c_read_reg(ctx->i2c_fd, OV5640_I2C_ADDR, reg, value);
}

int ov5640_write_reg(ov5640_tuning_t *ctx, uint16_t reg, uint8_t value)
{
    return i2c_write_reg(ctx->i2c_fd, OV5640_I2C_ADDR, reg, value);
}

int ov5640_set_exposure(ov5640_tuning_t *ctx, uint16_t exposure)
{
    int ret = 0;
    ret |= ov5640_write_reg(ctx, OV5640_REG_EXPOSURE_H, (exposure >> 12) & 0x0F);
    ret |= ov5640_write_reg(ctx, OV5640_REG_EXPOSURE_M, (exposure >> 4) & 0xFF);
    ret |= ov5640_write_reg(ctx, OV5640_REG_EXPOSURE_L, (exposure & 0x0F) << 4);
    if (ret == 0) ctx->params.exposure = exposure;
    return ret;
}

int ov5640_set_gain(ov5640_tuning_t *ctx, uint16_t gain)
{
    int ret = 0;
    ret |= ov5640_write_reg(ctx, OV5640_REG_GAIN_H, (gain >> 8) & 0x03);
    ret |= ov5640_write_reg(ctx, OV5640_REG_GAIN_L, gain & 0xFF);
    if (ret == 0) ctx->params.gain = gain;
    return ret;
}

int ov5640_set_white_balance(ov5640_tuning_t *ctx, uint16_t r, uint16_t g, uint16_t b)
{
    int ret = 0;
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_R_H, (r >> 8) & 0x03);
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_R_L, r & 0xFF);
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_G_H, (g >> 8) & 0x03);
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_G_L, g & 0xFF);
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_B_H, (b >> 8) & 0x03);
    ret |= ov5640_write_reg(ctx, OV5640_REG_WB_B_L, b & 0xFF);
    if (ret == 0) {
        ctx->params.wb_r = r;
        ctx->params.wb_g = g;
        ctx->params.wb_b = b;
    }
    return ret;
}

int ov5640_set_sharpness(ov5640_tuning_t *ctx, uint8_t sharpness)
{
    int ret = ov5640_write_reg(ctx, OV5640_REG_SHARPNESS, sharpness);
    if (ret == 0) ctx->params.sharpness = sharpness;
    return ret;
}

int ov5640_set_denoise(ov5640_tuning_t *ctx, uint8_t denoise)
{
    int ret = ov5640_write_reg(ctx, OV5640_REG_DENOISE, denoise);
    if (ret == 0) ctx->params.denoise = denoise;
    return ret;
}

int ov5640_set_saturation(ov5640_tuning_t *ctx, uint8_t saturation)
{
    int ret = ov5640_write_reg(ctx, OV5640_REG_SATURATION, saturation);
    if (ret == 0) ctx->params.saturation = saturation;
    return ret;
}

int ov5640_set_brightness(ov5640_tuning_t *ctx, uint8_t brightness)
{
    int ret = ov5640_write_reg(ctx, OV5640_REG_BRIGHTNESS, brightness);
    if (ret == 0) ctx->params.brightness = brightness;
    return ret;
}

int ov5640_set_contrast(ov5640_tuning_t *ctx, uint8_t contrast)
{
    int ret = ov5640_write_reg(ctx, OV5640_REG_CONTRAST, contrast);
    if (ret == 0) ctx->params.contrast = contrast;
    return ret;
}

int ov5640_restore_defaults(ov5640_tuning_t *ctx)
{
    return ov5640_apply_params(ctx, &ctx->default_params);
}

int ov5640_apply_params(ov5640_tuning_t *ctx, const ov5640_params_t *params)
{
    int ret = 0;

    ret |= ov5640_set_exposure(ctx, params->exposure);
    ret |= ov5640_set_gain(ctx, params->gain);
    ret |= ov5640_set_white_balance(ctx, params->wb_r, params->wb_g, params->wb_b);
    ret |= ov5640_set_sharpness(ctx, params->sharpness);
    ret |= ov5640_set_denoise(ctx, params->denoise);
    ret |= ov5640_set_saturation(ctx, params->saturation);
    ret |= ov5640_set_brightness(ctx, params->brightness);
    ret |= ov5640_set_contrast(ctx, params->contrast);

    if (ret == 0) {
        ctx->params = *params;
        printf("[OV5640] Parameters applied successfully\n");
    }

    return ret;
}

int ov5640_read_params(ov5640_tuning_t *ctx, ov5640_params_t *params)
{
    uint8_t val;
    int ret = 0;

    /* 读取曝光 */
    ret |= ov5640_read_reg(ctx, OV5640_REG_EXPOSURE_H, &val);
    params->exposure = (val & 0x0F) << 12;
    ret |= ov5640_read_reg(ctx, OV5640_REG_EXPOSURE_M, &val);
    params->exposure |= val << 4;
    ret |= ov5640_read_reg(ctx, OV5640_REG_EXPOSURE_L, &val);
    params->exposure |= (val >> 4) & 0x0F;

    /* 读取增益 */
    ret |= ov5640_read_reg(ctx, OV5640_REG_GAIN_H, &val);
    params->gain = (val & 0x03) << 8;
    ret |= ov5640_read_reg(ctx, OV5640_REG_GAIN_L, &val);
    params->gain |= val;

    /* 读取白平衡 */
    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_R_H, &val);
    params->wb_r = (val & 0x03) << 8;
    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_R_L, &val);
    params->wb_r |= val;

    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_G_H, &val);
    params->wb_g = (val & 0x03) << 8;
    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_G_L, &val);
    params->wb_g |= val;

    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_B_H, &val);
    params->wb_b = (val & 0x03) << 8;
    ret |= ov5640_read_reg(ctx, OV5640_REG_WB_B_L, &val);
    params->wb_b |= val;

    /* 读取其他参数 */
    ret |= ov5640_read_reg(ctx, OV5640_REG_SHARPNESS, &params->sharpness);
    ret |= ov5640_read_reg(ctx, OV5640_REG_DENOISE, &params->denoise);
    ret |= ov5640_read_reg(ctx, OV5640_REG_SATURATION, &params->saturation);
    ret |= ov5640_read_reg(ctx, OV5640_REG_BRIGHTNESS, &params->brightness);
    ret |= ov5640_read_reg(ctx, OV5640_REG_CONTRAST, &params->contrast);

    if (ret == 0) {
        ctx->params = *params;
    }

    return ret;
}

void ov5640_print_params(const ov5640_tuning_t *ctx)
{
    printf("[OV5640] Current Parameters:\n");
    printf("  Exposure:    0x%04X\n", ctx->params.exposure);
    printf("  Gain:        0x%04X\n", ctx->params.gain);
    printf("  White Balance: R=0x%04X G=0x%04X B=0x%04X\n",
           ctx->params.wb_r, ctx->params.wb_g, ctx->params.wb_b);
    printf("  Sharpness:   0x%02X\n", ctx->params.sharpness);
    printf("  Denoise:     0x%02X\n", ctx->params.denoise);
    printf("  Saturation:  0x%02X\n", ctx->params.saturation);
    printf("  Brightness:  0x%02X\n", ctx->params.brightness);
    printf("  Contrast:    0x%02X\n", ctx->params.contrast);
}

int ov5640_batch_tuning(ov5640_tuning_t *ctx, const char *test_name,
                        int (*capture_func)(void *user_data, uint8_t *buf, size_t size),
                        int (*save_func)(const char *filename, const uint8_t *buf, size_t size),
                        void *user_data)
{
    char filename[256];
    uint8_t *buf = NULL;
    size_t buf_size = 640 * 480 * 2; /* YUYV */

    /* 检查回调函数是否有效 */
    if (!capture_func || !save_func) {
        printf("[TUNING] Error: capture_func and save_func must not be NULL\n");
        printf("[TUNING] Skipping batch test, just tuning parameters...\n");

        /* 只做参数调整测试，不采集保存 */
        uint16_t exposure_tests[] = {0x0100, 0x0200, 0x0400, 0x0800, 0x1000};
        for (int i = 0; i < 5; i++) {
            ov5640_set_exposure(ctx, exposure_tests[i]);
            usleep(100000);
            printf("[TUNING] Exposure set to 0x%04X\n", exposure_tests[i]);
        }

        uint8_t sharpness_tests[] = {0x00, 0x20, 0x40, 0x60, 0x80};
        for (int i = 0; i < 5; i++) {
            ov5640_set_sharpness(ctx, sharpness_tests[i]);
            usleep(100000);
            printf("[TUNING] Sharpness set to 0x%02X\n", sharpness_tests[i]);
        }

        uint8_t saturation_tests[] = {0x00, 0x40, 0x80, 0xC0, 0xFF};
        for (int i = 0; i < 5; i++) {
            ov5640_set_saturation(ctx, saturation_tests[i]);
            usleep(100000);
            printf("[TUNING] Saturation set to 0x%02X\n", saturation_tests[i]);
        }

        ov5640_restore_defaults(ctx);
        printf("[TUNING] Defaults restored\n");
        return 0;
    }

    buf = malloc(buf_size);
    if (!buf) {
        perror("malloc failed");
        return -1;
    }

    printf("[TUNING] Starting batch test: %s\n", test_name);

    /* 测试不同曝光值 */
    uint16_t exposure_tests[] = {0x0100, 0x0200, 0x0400, 0x0800, 0x1000};
    for (int i = 0; i < 5; i++) {
        ov5640_set_exposure(ctx, exposure_tests[i]);
        usleep(100000); /* 等待生效 */

        if (capture_func(user_data, buf, buf_size) == 0) {
            snprintf(filename, sizeof(filename), "%s_exp_0x%04X.yuv",
                     test_name, exposure_tests[i]);
            save_func(filename, buf, buf_size);
            printf("[TUNING] Saved: %s\n", filename);
        }
    }

    /* 测试不同锐度 */
    uint8_t sharpness_tests[] = {0x00, 0x20, 0x40, 0x60, 0x80};
    for (int i = 0; i < 5; i++) {
        ov5640_set_sharpness(ctx, sharpness_tests[i]);
        usleep(100000);

        if (capture_func(user_data, buf, buf_size) == 0) {
            snprintf(filename, sizeof(filename), "%s_sharp_0x%02X.yuv",
                     test_name, sharpness_tests[i]);
            save_func(filename, buf, buf_size);
            printf("[TUNING] Saved: %s\n", filename);
        }
    }

    /* 测试不同饱和度 */
    uint8_t saturation_tests[] = {0x00, 0x40, 0x80, 0xC0, 0xFF};
    for (int i = 0; i < 5; i++) {
        ov5640_set_saturation(ctx, saturation_tests[i]);
        usleep(100000);

        if (capture_func(user_data, buf, buf_size) == 0) {
            snprintf(filename, sizeof(filename), "%s_sat_0x%02X.yuv",
                     test_name, saturation_tests[i]);
            save_func(filename, buf, buf_size);
            printf("[TUNING] Saved: %s\n", filename);
        }
    }

    /* 恢复默认参数 */
    ov5640_restore_defaults(ctx);

    free(buf);
    printf("[TUNING] Batch test completed: %s\n", test_name);
    return 0;
}
