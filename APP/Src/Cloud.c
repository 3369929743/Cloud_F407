#include "Cloud.h"

/**
  * @brief  云台系统初始化
  * @param  Cloud: 云台控制器结构体指针
  * @param  Cloud_Confg: 云台配置参数(串口/PID配置)
  * @retval 无
  * @note   初始化串口、电机驱动和PID控制器
  */
void Cloud_Init(Cloud_t *Cloud, Cloud_Confg_t *Cloud_Confg)
{
    /* 保存串口句柄 */
    Cloud->Serial_Vision = Cloud_Confg->Serial_Vision;
    Cloud->Emm_Yaw.Serial = Cloud_Confg->Serial_Yaw;
    Cloud->Emm_Pitch.Serial = Cloud_Confg->Serial_Pitch;
    
    /* 初始化电机(Yaw/Pitch轴) */
    Emm_Init(&Cloud->Emm_Yaw, Cloud_Confg->Serial_Yaw);
    Emm_Init(&Cloud->Emm_Pitch, Cloud_Confg->Serial_Pitch);

    /* 初始化PID控制器 */
    PID_Init(&Cloud->PID_Yaw, &Cloud_Confg->PID_Yaw_Confg);
    PID_Init(&Cloud->PID_Pitch, &Cloud_Confg->PID_Pitch_Confg);

    /* 初始化位置快速控制 */
    Emm_Pos_Control_Quick_Init(&Cloud->Emm_Yaw);
    Emm_Pos_Control_Quick_Init(&Cloud->Emm_Pitch);
}

/**
  * @brief  设置云台目标角度
  * @param  Cloud: 云台控制器结构体指针
  * @param  Yaw: 偏航轴目标值
  * @param  Pitch: 俯仰轴目标值
  * @retval 无
  */
void Cloud_Set_Target(Cloud_t *Cloud, PID_val Yaw, PID_val Pitch)
{
    PID_Set_Target(&Cloud->PID_Yaw, Yaw);
    PID_Set_Target(&Cloud->PID_Pitch, Pitch);
}

/**
  * @brief  云台运行控制
  * @param  Cloud: 云台控制器结构体指针
  * @param  Yaw: 偏航轴实际值(反馈)
  * @param  Pitch: 俯仰轴实际值(反馈)
  * @retval 无
  * @note   执行PID计算并驱动电机，需在周期性任务中调用
  */
void Cloud_Run(Cloud_t *Cloud, PID_val Yaw, PID_val Pitch)
{
    /* PID计算: 根据误差输出控制量 */
    PID_val Emm_x = PID_Calculate(&Cloud->PID_Yaw, -Yaw);
    PID_val Emm_y = PID_Calculate(&Cloud->PID_Pitch, Pitch);

    // if(Emm_x <= 3 && Emm_x >= -3) Emm_x = 0;
    // if(Emm_y <= 3 && Emm_y >= -3) Emm_y = 0;

    // if(Emm_x >= 500) Emm_x = 500;
    // if(Emm_x <= -500) Emm_x = -500;
    // if(Emm_y >= 500) Emm_y = 500;
    // if(Emm_y <= -500) Emm_y = -500;

    /* 驱动电机执行 */
    Emm_Pos_Run_Quick(&Cloud->Emm_Yaw, -Emm_x);
    Emm_Pos_Run_Quick(&Cloud->Emm_Pitch, -Emm_y);
}

uint8_t Cloud_Get_isEnable(Cloud_t *Cloud){
  return Cloud->isEnable;
}

void Cloud_Start(Cloud_t *Cloud){
    Cloud->isEnable = 1;
}

void Cloud_Stop(Cloud_t *Cloud){
    Cloud->isEnable = 0;
}
