#ifndef __EMM_V5_H__
#define __EMM_V5_H__

#include "stdbool.h"
#include <stdint.h>

typedef struct Serial_Struct Serial_t;

typedef struct {
    Serial_t *Serial;     // 串口结构体指针
    uint8_t Addr;       // 地址
    uint8_t Dir;        // 方向
    uint16_t Vel;       // 速度
    uint8_t Acc;        // 加速度
    int32_t Pul;       // 脉冲数
    uint8_t raF;           // 相对/绝对标志，0：相对上一输入目标位置进行相对位置运动, 1:相对坐标零点进行绝对位置运动, 2:相对当前实时位置进行相对位置运动
    bool snF;           // 多机同步运动标志，0：不同步，1：同步
}Emm_t;

void Emm_Init(Emm_t *Emm, Serial_t *Serial);
void Emm_Set_Addr(Emm_t *Emm, uint8_t Addr);
void Emm_SetSpeed(Emm_t *Emm, uint16_t Vel);
void Emm_SetAcc(Emm_t *Emm, uint8_t Acc);
void Emm_SetMode(Emm_t *Emm, uint8_t raF, bool snF);
void Emm_Pos_Run(Emm_t *Emm, int32_t Pul);
void Emm_Pos_Control_Quick_Init(Emm_t *Emm);
void Emm_Pos_Run_Quick(Emm_t *Emm, int32_t Pul);

#endif
