#include "Ball_Contral.h"

/**
  * @brief  初始化球体控制模块
  * @param  BallContral: 球体控制结构体指针
  * @param  Serial_K230: K230串口通信结构体指针
  * @param  Serial_Emm: Emm步进电机串口通信结构体指针
  * @param  PID_Confg: PID配置参数结构体指针
  * @retval 无
  */
void BallContral_Init(BallContral_t *BallContral, Serial_t *Serial_K230, Serial_t *Serial_Emm, PID_Confg_t *PID_Confg)
{
    /* 保存串口通信句柄 */
    BallContral->Serial_K230 = Serial_K230;
    BallContral->Serial_Emm = Serial_Emm;

    /* 初始化Emm步进电机和PID控制器 */
    Emm_Init(&BallContral->Emm_StepMotor, Serial_Emm);
    PID_Init(&BallContral->PID_StepMotor, PID_Confg);

    /* 初始化Emm位置快速控制模式 */
    Emm_Pos_Control_Quick_Init(&BallContral->Emm_StepMotor);
}

void Ball_Contral_Emm_Quick_Init(BallContral_t *BallContral){
    Emm_Pos_Control_Quick_Init(&BallContral->Emm_StepMotor);
}

void Ball_Contral_Pop_Run(BallContral_t *BallContral, int32_t Pulse){
    Emm_Pos_Run_Quick(&BallContral->Emm_StepMotor, Pulse);
}

void BallContral_Set_Target(BallContral_t *BallContral, PID_val Target)
{
    PID_Set_Target(&BallContral->PID_StepMotor, Target);
}

void BallContral_Run(BallContral_t *BallContral, PID_val Target){
    PID_val Emm = PID_Calculate(&BallContral->PID_StepMotor, Target);
    Emm_Pos_Run_Quick(&BallContral->Emm_StepMotor, -Emm);
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