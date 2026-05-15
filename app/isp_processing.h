/*
 * isp_processing.h - ISP图像处理模块
 * 色彩校正、Gamma校正、自动曝光/白平衡
 */

#ifndef ISP_PROCESSING_H
#define ISP_PROCESSING_H

#include <stdint.h>

/* 色彩校正矩阵 (3x3) */
typedef struct {
    float matrix[3][3];  /* 色彩校正矩阵 */
    float offset[3];     /* 偏移量 */
} ccm_t;

/* Gamma校正曲线 */
typedef struct {
    uint8_t curve[256];  /* Gamma查找表 */
    float gamma;         /* Gamma值 */
} gamma_t;

/* 自动曝光参数 */
typedef struct {
    int target_brightness;  /* 目标亮度 (0-255) */
    int current_brightness; /* 当前亮度 */
    float exposure_factor;  /* 曝光调整因子 */
    int min_exposure;       /* 最小曝光 */
    int max_exposure;       /* 最大曝光 */
} ae_params_t;

/* 自动白平衡参数 */
typedef struct {
    float r_gain;           /* 红色增益 */
    float g_gain;           /* 绿色增益 */
    float b_gain;           /* 蓝色增益 */
    float target_r;         /* 目标红色比例 */
    float target_g;         /* 目标绿色比例 */
    float target_b;         /* 目标蓝色比例 */
} awb_params_t;

/* ISP上下文 */
typedef struct {
    ccm_t ccm;              /* 色彩校正矩阵 */
    gamma_t gamma;          /* Gamma校正 */
    ae_params_t ae;         /* 自动曝光 */
    awb_params_t awb;       /* 自动白平衡 */
    int ae_enabled;         /* AE使能 */
    int awb_enabled;        /* AWB使能 */
} isp_context_t;

/* 函数声明 */

/**
 * 初始化ISP模块
 * @param ctx ISP上下文
 * @return 0成功，-1失败
 */
int isp_init(isp_context_t *ctx);

/**
 * 设置色彩校正矩阵
 * @param ctx ISP上下文
 * @param ccm 色彩校正矩阵
 */
void isp_set_ccm(isp_context_t *ctx, const ccm_t *ccm);

/**
 * 设置Gamma值
 * @param ctx ISP上下文
 * @param gamma Gamma值 (0.1-5.0)
 */
void isp_set_gamma(isp_context_t *ctx, float gamma);

/**
 * 使能/禁用自动曝光
 * @param ctx ISP上下文
 * @param enable 1使能，0禁用
 */
void isp_enable_ae(isp_context_t *ctx, int enable);

/**
 * 使能/禁用自动白平衡
 * @param ctx ISP上下文
 * @param enable 1使能，0禁用
 */
void isp_enable_awb(isp_context_t *ctx, int enable);

/**
 * 处理一帧图像 (YUYV格式)
 * @param ctx ISP上下文
 * @param input 输入图像
 * @param output 输出图像
 * @param width 宽度
 * @param height 高度
 */
void isp_process_frame(isp_context_t *ctx, const uint8_t *input, uint8_t *output,
                       int width, int height);

/**
 * 计算图像平均亮度
 * @param yuyv YUYV图像
 * @param width 宽度
 * @param height 高度
 * @return 平均亮度 (0-255)
 */
int isp_calc_brightness(const uint8_t *yuyv, int width, int height);

/**
 * 计算白平衡统计
 * @param yuyv YUYV图像
 * @param width 宽度
 * @param height 高度
 * @param r_avg 红色平均值
 * @param g_avg 绿色平均值
 * @param b_avg 蓝色平均值
 */
void isp_calc_wb_stats(const uint8_t *yuyv, int width, int height,
                       float *r_avg, float *g_avg, float *b_avg);

/**
 * 更新自动曝光
 * @param ctx ISP上下文
 * @param yuyv YUYV图像
 * @param width 宽度
 * @param height 高度
 * @return 曝光调整量
 */
int isp_update_ae(isp_context_t *ctx, const uint8_t *yuyv, int width, int height);

/**
 * 更新自动白平衡
 * @param ctx ISP上下文
 * @param yuyv YUYV图像
 * @param width 宽度
 * @param height 高度
 */
void isp_update_awb(isp_context_t *ctx, const uint8_t *yuyv, int width, int height);

/**
 * 应用色彩校正
 * @param r 红色分量
 * @param g 绿色分量
 * @param b 蓝色分量
 * @param ccm 色彩校正矩阵
 * @param r_out 输出红色
 * @param g_out 输出绿色
 * @param b_out 输出蓝色
 */
void isp_apply_ccm(uint8_t r, uint8_t g, uint8_t b, const ccm_t *ccm,
                   uint8_t *r_out, uint8_t *g_out, uint8_t *b_out);

/**
 * 应用Gamma校正
 * @param value 输入值
 * @param gamma Gamma查找表
 * @return 校正后的值
 */
uint8_t isp_apply_gamma(uint8_t value, const gamma_t *gamma);

#endif /* ISP_PROCESSING_H */
