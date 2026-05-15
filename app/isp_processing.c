/*
 * isp_processing.c - ISP图像处理模块实现
 * 色彩校正、Gamma校正、自动曝光/白平衡
 */

#include "isp_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* 默认色彩校正矩阵 (单位矩阵，无校正) */
static const ccm_t default_ccm = {
    .matrix = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    },
    .offset = {0.0f, 0.0f, 0.0f}
};

/* 生成Gamma查找表 */
static void generate_gamma_lut(gamma_t *gamma)
{
    for (int i = 0; i < 256; i++) {
        float normalized = i / 255.0f;
        float corrected = powf(normalized, 1.0f / gamma->gamma);
        int val = (int)(corrected * 255.0f);
        if (val > 255) val = 255;
        if (val < 0) val = 0;
        gamma->curve[i] = (uint8_t)val;
    }
}

int isp_init(isp_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    /* 初始化色彩校正矩阵 */
    ctx->ccm = default_ccm;

    /* 初始化Gamma */
    ctx->gamma.gamma = 2.2f;
    generate_gamma_lut(&ctx->gamma);

    /* 初始化自动曝光 */
    ctx->ae.target_brightness = 128;
    ctx->ae.min_exposure = 0x0100;
    ctx->ae.max_exposure = 0x1000;
    ctx->ae.exposure_factor = 1.0f;

    /* 初始化自动白平衡 */
    ctx->awb.target_r = 0.299f;
    ctx->awb.target_g = 0.587f;
    ctx->awb.target_b = 0.114f;
    ctx->awb.r_gain = 1.0f;
    ctx->awb.g_gain = 1.0f;
    ctx->awb.b_gain = 1.0f;

    ctx->ae_enabled = 0;
    ctx->awb_enabled = 0;

    printf("[ISP] Initialized\n");
    return 0;
}

void isp_set_ccm(isp_context_t *ctx, const ccm_t *ccm)
{
    ctx->ccm = *ccm;
}

void isp_set_gamma(isp_context_t *ctx, float gamma)
{
    if (gamma < 0.1f) gamma = 0.1f;
    if (gamma > 5.0f) gamma = 5.0f;
    ctx->gamma.gamma = gamma;
    generate_gamma_lut(&ctx->gamma);
}

void isp_enable_ae(isp_context_t *ctx, int enable)
{
    ctx->ae_enabled = enable;
    printf("[ISP] Auto Exposure: %s\n", enable ? "enabled" : "disabled");
}

void isp_enable_awb(isp_context_t *ctx, int enable)
{
    ctx->awb_enabled = enable;
    printf("[ISP] Auto White Balance: %s\n", enable ? "enabled" : "disabled");
}

void isp_apply_ccm(uint8_t r, uint8_t g, uint8_t b, const ccm_t *ccm,
                   uint8_t *r_out, uint8_t *g_out, uint8_t *b_out)
{
    float r_f = ccm->matrix[0][0] * r + ccm->matrix[0][1] * g + ccm->matrix[0][2] * b + ccm->offset[0];
    float g_f = ccm->matrix[1][0] * r + ccm->matrix[1][1] * g + ccm->matrix[1][2] * b + ccm->offset[1];
    float b_f = ccm->matrix[2][0] * r + ccm->matrix[2][1] * g + ccm->matrix[2][2] * b + ccm->offset[2];

    /* 限制范围 */
    if (r_f < 0) r_f = 0; if (r_f > 255) r_f = 255;
    if (g_f < 0) g_f = 0; if (g_f > 255) g_f = 255;
    if (b_f < 0) b_f = 0; if (b_f > 255) b_f = 255;

    *r_out = (uint8_t)r_f;
    *g_out = (uint8_t)g_f;
    *b_out = (uint8_t)b_f;
}

uint8_t isp_apply_gamma(uint8_t value, const gamma_t *gamma)
{
    return gamma->curve[value];
}

int isp_calc_brightness(const uint8_t *yuyv, int width, int height)
{
    long sum = 0;
    int count = 0;

    for (int i = 0; i < width * height * 2; i += 2) {
        sum += yuyv[i];
        count++;
    }

    return (int)(sum / count);
}

void isp_calc_wb_stats(const uint8_t *yuyv, int width, int height,
                       float *r_avg, float *g_avg, float *b_avg)
{
    long r_sum = 0, g_sum = 0, b_sum = 0;
    int count = 0;

    for (int i = 0; i < width * height * 2; i += 4) {
        uint8_t y0 = yuyv[i];
        uint8_t u = yuyv[i + 1];
        uint8_t y1 = yuyv[i + 2];
        uint8_t v = yuyv[i + 3];

        /* YUYV to RGB (简化转换) */
        int r = y0 + 1.402 * (v - 128);
        int g = y0 - 0.344 * (u - 128) - 0.714 * (v - 128);
        int b = y0 + 1.772 * (u - 128);

        /* 限制范围 */
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;

        r_sum += r;
        g_sum += g;
        b_sum += b;
        count++;
    }

    if (count > 0) {
        *r_avg = (float)r_sum / count / 255.0f;
        *g_avg = (float)g_sum / count / 255.0f;
        *b_avg = (float)b_sum / count / 255.0f;
    }
}

int isp_update_ae(isp_context_t *ctx, const uint8_t *yuyv, int width, int height)
{
    if (!ctx->ae_enabled) return 0;

    int brightness = isp_calc_brightness(yuyv, width, height);
    ctx->ae.current_brightness = brightness;

    int diff = ctx->ae.target_brightness - brightness;
    float factor = 1.0f + (float)diff / 255.0f;

    /* 平滑调整 */
    ctx->ae.exposure_factor = ctx->ae.exposure_factor * 0.9f + factor * 0.1f;

    /* 计算新的曝光值 */
    int new_exposure = (int)(ctx->ae.exposure_factor * 0x0400);
    if (new_exposure < ctx->ae.min_exposure) new_exposure = ctx->ae.min_exposure;
    if (new_exposure > ctx->ae.max_exposure) new_exposure = ctx->ae.max_exposure;

    return new_exposure;
}

void isp_update_awb(isp_context_t *ctx, const uint8_t *yuyv, int width, int height)
{
    if (!ctx->awb_enabled) return;

    float r_avg, g_avg, b_avg;
    isp_calc_wb_stats(yuyv, width, height, &r_avg, &g_avg, &b_avg);

    /* 计算增益 */
    float r_gain = ctx->awb.target_r / (r_avg + 0.001f);
    float g_gain = ctx->awb.target_g / (g_avg + 0.001f);
    float b_gain = ctx->awb.target_b / (b_avg + 0.001f);

    /* 平滑调整 */
    ctx->awb.r_gain = ctx->awb.r_gain * 0.9f + r_gain * 0.1f;
    ctx->awb.g_gain = ctx->awb.g_gain * 0.9f + g_gain * 0.1f;
    ctx->awb.b_gain = ctx->awb.b_gain * 0.9f + b_gain * 0.1f;

    /* 限制增益范围 */
    if (ctx->awb.r_gain < 0.5f) ctx->awb.r_gain = 0.5f;
    if (ctx->awb.r_gain > 2.0f) ctx->awb.r_gain = 2.0f;
    if (ctx->awb.g_gain < 0.5f) ctx->awb.g_gain = 0.5f;
    if (ctx->awb.g_gain > 2.0f) ctx->awb.g_gain = 2.0f;
    if (ctx->awb.b_gain < 0.5f) ctx->awb.b_gain = 0.5f;
    if (ctx->awb.b_gain > 2.0f) ctx->awb.b_gain = 2.0f;
}

void isp_process_frame(isp_context_t *ctx, const uint8_t *input, uint8_t *output,
                       int width, int height)
{
    /* 处理YUYV格式 */
    for (int i = 0; i < width * height * 2; i += 4) {
        uint8_t y0 = input[i];
        uint8_t u = input[i + 1];
        uint8_t y1 = input[i + 2];
        uint8_t v = input[i + 3];

        /* YUYV to RGB */
        int r = y0 + 1.402 * (v - 128);
        int g = y0 - 0.344 * (u - 128) - 0.714 * (v - 128);
        int b = y0 + 1.772 * (u - 128);

        /* 限制范围 */
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;

        /* 应用白平衡 */
        if (ctx->awb_enabled) {
            r = (int)(r * ctx->awb.r_gain);
            g = (int)(g * ctx->awb.g_gain);
            b = (int)(b * ctx->awb.b_gain);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
        }

        /* 应用色彩校正 */
        uint8_t r_ccm, g_ccm, b_ccm;
        isp_apply_ccm(r, g, b, &ctx->ccm, &r_ccm, &g_ccm, &b_ccm);

        /* 应用Gamma校正 */
        r_ccm = isp_apply_gamma(r_ccm, &ctx->gamma);
        g_ccm = isp_apply_gamma(g_ccm, &ctx->gamma);
        b_ccm = isp_apply_gamma(b_ccm, &ctx->gamma);

        /* RGB to YUYV */
        uint8_t y_new = (uint8_t)(0.299f * r_ccm + 0.587f * g_ccm + 0.114f * b_ccm);
        uint8_t u_new = (uint8_t)(-0.147f * r_ccm - 0.289f * g_ccm + 0.436f * b_ccm + 128);
        uint8_t v_new = (uint8_t)(0.615f * r_ccm - 0.515f * g_ccm - 0.100f * b_ccm + 128);

        output[i] = y_new;
        output[i + 1] = u_new;
        output[i + 2] = y_new; /* 简化：两个像素使用相同的Y */
        output[i + 3] = v_new;
    }
}
