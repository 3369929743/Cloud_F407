#ifndef __BALL_CONTRAL_H__
#define __BALL_CONTRAL_H__

#include "Emm_v5.h"
#include "PID.h"

typedef struct BallContral_Struct{
    uint8_t is_Enable;

    Serial_t *Serial_K230;
    Serial_t *Serial_Emm;

    Emm_t Emm_StepMotor;

    PID_t PID_StepMotor;
}BallContral_t;

void BallContral_Init(BallContral_t *BallContral, Serial_t *Serial_K230, Serial_t *Serial_Emm, PID_Confg_t *PID_Confg);
void BallContral_Set_Target(BallContral_t *BallContral, PID_val Target);
void BallContral_Run(BallContral_t *BallContral, PID_val Target);
uint8_t BallContral_Get_is_Enable(BallContral_t *BallContral);
void BallContral_Start(BallContral_t *BallContral);
void BallContral_Stop(BallContral_t *BallContral);


#endif