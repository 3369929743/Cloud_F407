#ifndef __DO_DEVICE_H__
#define __DO_DEVICE_H__

#include <stdint.h>

#define DO_LEVEL_LOW 0
#define DO_LEVEL_HIGH 1

#define DO_STATE_OFF 0
#define DO_STATE_ON 1

#define DO_BLINK_FOREVER 0xFFFF

typedef struct DO_Device_Struct{

    uint8_t Default_Level;     // 默认状态

    uint8_t is_On; // DO_Device当前状态标志，1表示使能，0表示失能
    uint8_t is_Blinking; // DO_Device是否正在使能标志，1表示正在使能，0表示未使能

    uint16_t On_Time;          // 使能时间计数器
    uint16_t Off_Time;         // 失能时间计数器
    uint16_t Blink_Count;      // 使能次数计数器

    uint16_t Tick_Time; 

    void (*DO_Write_Pin)(uint8_t Level); // DO_Device引脚写入函数指针
} DO_Device_t;

void DO_Device_Init(DO_Device_t *Device, uint8_t Init_Level, void (*DO_Write_Pin)(uint8_t Level));
void DO_Device_ON(DO_Device_t *Device);
void DO_Device_OFF(DO_Device_t *Device);
void DO_Device_BLINK(DO_Device_t *Device, uint16_t On_Time, uint16_t Off_Time, uint16_t Count);
void DO_Device_Tick(DO_Device_t *Device);
uint8_t DO_Device_GetState(DO_Device_t *Device);

#endif