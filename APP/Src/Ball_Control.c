#include "Ball_Contral.h"

void BallContral_Init(BallContral_t *BallContral, Serial_t *Serial_K230, Serial_t *Serial_Emm, PID_Confg_t *PID_Confg)
{
    BallContral->Serial_K230 = Serial_K230;
    BallContral->Serial_Emm = Serial_Emm;

    Emm_Init(&BallContral->Emm_StepMotor, Serial_Emm);
    PID_Init(&BallContral->PID_StepMotor, PID_Confg);

    Emm_Pos_Control_Quick_Init(&BallContral->Emm_StepMotor);
}

void BallContral_Set_Target(BallContral_t *BallContral, PID_val Target)
{
    PID_Set_Target(&BallContral->PID_StepMotor, Target);
}

void BallContral_Run(BallContral_t *BallContral, PID_val Target){
    PID_val Emm = PID_Calculate(&BallContral->PID_StepMotor, Target);
    Emm_Pos_Run_Quick(&BallContral->Emm_StepMotor, Emm);
}

uint8_t BallContral_Get_is_Enable(BallContral_t *BallContral){
    return BallContral->is_Enable;
}

void BallContral_Start(BallContral_t *BallContral){
    BallContral->is_Enable = 1;
}

void BallContral_Stop(BallContral_t *BallContral){
    BallContral->is_Enable = 0;
}