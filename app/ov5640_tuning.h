/*
 * ov5640_tuning.h - OV5640 图像质量调优模块
 * 通过I2C读写OV5640寄存器，调整图像参数
 */

#ifndef OV5640_TUNING_H
#define OV5640_TUNING_H

#include <stdint.h>
#include <stddef.h>

/* OV5640 I2C地址 */
#define OV5640_I2C_ADDR 0x3C

/* 图像参数结构体 */
typedef struct {
    uint16_t exposure;      /* 曝光值 (0-65535) */
    uint16_t gain;          /* 增益 (0-1023) */
    uint16_t wb_r;          /* 白平衡红色增益 */
    uint16_t wb_g;          /* 白平衡绿色增益 */
    uint16_t wb_b;          /* 白平衡蓝色增益 */
    uint8_t  sharpness;     /* 锐度 (0-255) */
    uint8_t  denoise;       /* 降噪等级 (0-255) */
    uint8_t  saturation;    /* 饱和度 (0-255) */
    uint8_t  brightness;    /* 亮度 (0-255) */
    uint8_t  contrast;      /* 对比度 (0-255) */
} ov5640_params_t;

/* 调优上下文 */
typedef struct {
    int i2c_fd;             /* I2C文件描述符 */
    ov5640_params_t params; /* 当前参数 */
    ov5640_params_t default_params; /* 默认参数 */
} ov5640_tuning_t;

/* 函数声明 */

/**
 * 初始化OV5640调优模块
 * @param ctx 调优上下文
 * @param i2c_dev I2C设备路径 (如 "/dev/i2c-0")
 * @return 0成功，-1失败
 */
int ov5640_tuning_init(ov5640_tuning_t *ctx, const char *i2c_dev);

/**
 * 关闭OV5640调优模块
 * @param ctx 调优上下文
 */
void ov5640_tuning_close(ov5640_tuning_t *ctx);

/**
 * 读取OV5640寄存器
 * @param ctx 调优上下文
 * @param reg 寄存器地址 (16位)
 * @param value 读取的值
 * @return 0成功，-1失败
 */
int ov5640_read_reg(ov5640_tuning_t *ctx, uint16_t reg, uint8_t *value);

/**
 * 写入OV5640寄存器
 * @param ctx 调优上下文
 * @param reg 寄存器地址 (16位)
 * @param value 写入的值
 * @return 0成功，-1失败
 */
int ov5640_write_reg(ov5640_tuning_t *ctx, uint16_t reg, uint8_t value);

/**
 * 设置曝光值
 * @param ctx 调优上下文
 * @param exposure 曝光值 (0-65535)
 * @return 0成功，-1失败
 */
int ov5640_set_exposure(ov5640_tuning_t *ctx, uint16_t exposure);

/**
 * 设置增益
 * @param ctx 调优上下文
 * @param gain 增益值 (0-1023)
 * @return 0成功，-1失败
 */
int ov5640_set_gain(ov5640_tuning_t *ctx, uint16_t gain);

/**
 * 设置白平衡
 * @param ctx 调优上下文
 * @param r 红色增益
 * @param g 绿色增益
 * @param b 蓝色增益
 * @return 0成功，-1失败
 */
int ov5640_set_white_balance(ov5640_tuning_t *ctx, uint16_t r, uint16_t g, uint16_t b);

/**
 * 设置锐度
 * @param ctx 调优上下文
 * @param sharpness 锐度值 (0-255)
 * @return 0成功，-1失败
 */
int ov5640_set_sharpness(ov5640_tuning_t *ctx, uint8_t sharpness);

/**
 * 设置降噪等级
 * @param ctx 调优上下文
 * @param denoise 降噪等级 (0-255)
 * @return 0成功，-1失败
 */
int ov5640_set_denoise(ov5640_tuning_t *ctx, uint8_t denoise);

/**
 * 设置饱和度
 * @param ctx 调优上下文
 * @param saturation 饱和度 (0-255)
 * @return 0成功，-1失败
 */
int ov5640_set_saturation(ov5640_tuning_t *ctx, uint8_t saturation);

/**
 * 设置亮度
 * @param ctx 调优上下文
 * @param brightness 亮度 (0-255)
 * @return 0成功，-1失败
 */
int ov5640_set_brightness(ov5640_tuning_t *ctx, uint8_t brightness);

/**
 * 设置对比度
 * @param ctx 调优上下文
 * @param contrast 对比度 (0-255)
 * @return 0成功，-1失败
 */
int ov5640_set_contrast(ov5640_tuning_t *ctx, uint8_t contrast);

/**
 * 恢复默认参数
 * @param ctx 调优上下文
 * @return 0成功，-1失败
 */
int ov5640_restore_defaults(ov5640_tuning_t *ctx);

/**
 * 应用参数到OV5640
 * @param ctx 调优上下文
 * @param params 要应用的参数
 * @return 0成功，-1失败
 */
int ov5640_apply_params(ov5640_tuning_t *ctx, const ov5640_params_t *params);

/**
 * 读取当前参数
 * @param ctx 调优上下文
 * @param params 读取的参数
 * @return 0成功，-1失败
 */
int ov5640_read_params(ov5640_tuning_t *ctx, ov5640_params_t *params);

/**
 * 打印当前参数
 * @param ctx 调优上下文
 */
void ov5640_print_params(const ov5640_tuning_t *ctx);

/**
 * 批量调优测试
 * @param ctx 调优上下文
 * @param test_name 测试名称
 * @param capture_func 采集回调函数
 * @param save_func 保存回调函数
 * @return 0成功，-1失败
 */
int ov5640_batch_tuning(ov5640_tuning_t *ctx, const char *test_name,
                        int (*capture_func)(void *user_data, uint8_t *buf, size_t size),
                        int (*save_func)(const char *filename, const uint8_t *buf, size_t size),
                        void *user_data);

#endif /* OV5640_TUNING_H */
