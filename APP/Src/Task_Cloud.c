#include "Task_Cloud.h"
#include "Cloud.h"
#include "BSP_Config.h"
#include "Emm_v5.h"
#include "Serial.h"
#include "K230.h"
#include "SoftTimer.h"
#include "DO_Device.h"

Serial_t Serial_K230;
Serial_t Serial_Yaw;
Serial_t Serial_Pitch;
DO_Device_t Laser;

static Cloud_t Cloud;

static SoftTimer_t SoftTimer_Cloud;
static SoftTimer_t SoftTimer_K230;

static Task_Cloud_State_e Task_Cloud_State = TASK_CLOUD_IDLE;

Cloud_Confg_t Cloud_Confg={
    .Serial_Vision = &Serial_K230,
    .Serial_Yaw = &Serial_Yaw,
    .Serial_Pitch = &Serial_Pitch,
    .PID_Yaw_Confg = {0.12, 0, 0 },
    .PID_Pitch_Confg = {0.06, 0, 0 },
};

void Laser_Write_Pin(uint8_t Level){
  HAL_GPIO_WritePin(GPIO_Laser_GPIO_Port, GPIO_Laser_Pin, Level);
}

void Task_Cloud_Init(void)
{
    Serial_Init(&Serial_K230, Serial_3);
    Serial_Init(&Serial_Yaw, Serial_1);
    Serial_Init(&Serial_Pitch, Serial_2);
    K230_Init(&Serial_K230, K230_OFFSET_X, K230_OFFSET_Y);
    Cloud_Init(&Cloud, &Cloud_Confg);
    SoftTimer_Init(&SoftTimer_Cloud, SOFTTIMER_MODE_PERIODIC, 10);
    SoftTimer_Start(&SoftTimer_Cloud);
    SoftTimer_Init(&SoftTimer_K230, SOFTTIMER_MODE_PERIODIC, 200);
    SoftTimer_Start(&SoftTimer_K230);
    Cloud_Set_Target(&Cloud, 0, 0);
    DO_Device_Init(&Laser, 0, Laser_Write_Pin);
    Task_Cloud_State = TASK_CLOUD_IDLE;
}

void Task_Cloud_Pith_Init(void){
    Emm_Pos_Run_Quick(&Cloud.Emm_Pitch, -800);
}

void Task_Cloud_Toggle(void){
    if(Task_Cloud_State == TASK_CLOUD_IDLE){
        Task_Cloud_State = TASK_CLOUD_RUNNING;
        Cloud_Start(&Cloud);
    }
    else{
        Task_Cloud_State = TASK_CLOUD_IDLE;
        Cloud_Stop(&Cloud);
    }
}

void Task_Cloud_Loop(void)
{
    if(!SoftTimer_Trigger(&SoftTimer_Cloud)) return;
    switch(Task_Cloud_State){
        case TASK_CLOUD_IDLE:
            break;
        case TASK_CLOUD_RUNNING:
            if(K230_GetFlag()){
                if(K230_Error_Update()){
                    if(!DO_Device_GetState(&Laser)){
                        DO_Device_ON(&Laser);
                    }
                    SoftTimer_Reset(&SoftTimer_K230);
                }
            }
            if(SoftTimer_Trigger(&SoftTimer_K230)){
                Cloud_Stop(&Cloud);
                Task_Cloud_State = TASK_CLOUD_LOST;
            }
            else{
                Cloud_Run(&Cloud, K230_GetError_x(), K230_GetError_y());
            }
            break;
        case TASK_CLOUD_LOST:
            Cloud_Stop(&Cloud);
            if(K230_GetFlag()){
                if(K230_Error_Update()){
                    SoftTimer_Reset(&SoftTimer_K230);
                }
                Cloud_Start(&Cloud);
                Task_Cloud_State = TASK_CLOUD_RUNNING;
            }
            break;
    }
}

void Task_Cloud_Exit(void)
{
    
}

void Task_Cloud_Tick(void){
    SoftTimer_Update(&SoftTimer_K230);
    SoftTimer_Update(&SoftTimer_Cloud);
    DO_Device_Tick(&Laser);
}
