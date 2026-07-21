#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdint.h>

#define TRACE_SENSOR_NUM          8U
#define TRACE_FILTER_ALPHA_MAX    256U

/* Pin_Data的第0位对应Weight[0]，置位表示检测到黑线 */
typedef enum {
    TRACE_STATUS_TRACKING = 0,     /* 正常循迹状态，传感器检测到黑线 */
    TRACE_STATUS_CROSS_LINE,       /* 十字路口状态，所有传感器都检测到黑线 */
    TRACE_STATUS_LOST_LEFT,        /* 丢失黑线，正在向左搜索 */
    TRACE_STATUS_LOST_RIGHT,       /* 丢失黑线，正在向右搜索 */
    TRACE_STATUS_LOST_UNKNOWN      /* 丢失黑线，搜索方向未知 */
} Trace_Status_t;

typedef struct {
    /* 负值表示偏左，正值表示偏右；顺序与Pin_Data的位0..7对应 */
    int8_t Weight[TRACE_SENSOR_NUM];
    /* 丢失黑线后搜索时使用的绝对误差值 */
    int16_t Lost_Error;
    /* 0表示最大平滑，1表示直接跟随原始误差 */
    float Filter_Alpha;
    /* 最后一次误差在此范围内时不决定恢复方向 */
    uint8_t Lost_Deadband;
} Trace_Config_t;

typedef struct Trace_struct {
    struct {
        int8_t Weight[TRACE_SENSOR_NUM];
        int16_t Lost_Error;
        uint16_t Filter_Alpha;
        uint8_t Lost_Deadband;
    } Config;
    int16_t Cur_Error;               /* 当前滤波后的误差值 */
    int16_t Pre_Error;               /* 上一次滤波后的误差值 */
    int16_t Raw_Error;               /* 当前原始误差值（未经滤波） */
    uint8_t Pin_Data;                /* 传感器引脚数据，每一位代表一个传感器的检测状态 */
    uint8_t Active_Count;            /* 当前检测到黑线的传感器数量 */
    uint16_t Lost_Count;             /* 丢失黑线后的计数器 */
    Trace_Status_t Status;           /* 当前循迹状态 */
    uint8_t Has_Valid_Error;         /* 是否有有效误差值的标志位 */

    uint8_t (*Get_Pin_Data)(void);    /* 获取传感器引脚数据的函数指针 */
} Trace_t;


void Trace_GetDefaultConfig(Trace_Config_t *Config);
void Trace_Init(Trace_t *Trace, const Trace_Config_t *Config);
void Trace_Reset(Trace_t *Trace);
int16_t Trace_Update(Trace_t *Trace);

int16_t Trace_GetError(const Trace_t *Trace);
int16_t Trace_GetRawError(const Trace_t *Trace);
Trace_Status_t Trace_GetStatus(const Trace_t *Trace);
uint8_t Trace_GetActiveCount(const Trace_t *Trace);
void Trace_SetDataCallback(Trace_t *Trace, uint8_t (*Get_Pin_Data)(void));


#endif