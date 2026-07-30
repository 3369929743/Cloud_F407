#include "Task_Ball_Contral.h"
#include "K230.h"
#include "Serial.h"
#include "Ball_Contral.h"
#include "SoftTimer.h"

static Serial_t Serial_K230;
static Serial_t Serial_Emm_Ball;
static SoftTimer_t SoftTimer_Ball_Contral;
static SoftTimer_t SoftTimer_K230;

static BallContral_t BallContral;

static PID_Confg_t PID_Ball_Confg = {
    .Kp = 0.05,
    .Ki = 0,
    .Kd = 0,
};

static Task_Ball_Contral_State_e Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;

void Task_Ball_Contral_Init(void){
    Serial_Init(&Serial_K230, Serial_3);
    Serial_Init(&Serial_Emm_Ball, Serial_2);
    BallContral_Init(&BallContral, &Serial_K230, &Serial_Emm_Ball, &PID_Ball_Confg);
    K230_Init(&Serial_K230, 0, 0);

    SoftTimer_Init(&SoftTimer_Ball_Contral, SOFTTIMER_MODE_PERIODIC, 10);
    SoftTimer_Init(&SoftTimer_K230, SOFTTIMER_MODE_PERIODIC, 200);

    SoftTimer_Start(&SoftTimer_Ball_Contral);
    SoftTimer_Start(&SoftTimer_K230);

    BallContral_Set_Target(&BallContral, 0);  //K230y轴坐标为实际值
    Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
}

void Task_Ball_Contral_Toggle(void){
    if(Task_Ball_Contral_State == TASK_BALL_CONTRAL_IDLE){
        Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
        BallContral_Start(&BallContral);
    }
    else{
        Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
        BallContral_Stop(&BallContral);
    }
}

void Task_Ball_Contral_Loop(void){
    if(!SoftTimer_Trigger(&SoftTimer_Ball_Contral)) return;

    switch(Task_Ball_Contral_State){
        case TASK_BALL_CONTRAL_IDLE:
            break;
        case TASK_BALL_CONTRAL_RUNNING:
            if(K230_GetFlag()){
                if(K230_Error_Update()){
                    SoftTimer_Reset(&SoftTimer_K230);
                }
            }
            if(SoftTimer_Trigger(&SoftTimer_K230)){
                BallContral_Stop(&BallContral);
                Task_Ball_Contral_State = TASK_BALL_CONTRAL_LOST;
            }
            else{
                BallContral_Run(&BallContral, K230_GetError_x());
            }
            break;
        case TASK_BALL_CONTRAL_LOST:
            BallContral_Stop(&BallContral);
            if(K230_GetFlag()){
                if(K230_Error_Update()){
                    SoftTimer_Reset(&SoftTimer_K230);
                }
                BallContral_Start(&BallContral);
                Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
            }
            break;
    }
}

void Task_Ball_Contral_Tick(void){
    SoftTimer_Update(&SoftTimer_Ball_Contral);
    SoftTimer_Update(&SoftTimer_K230);
}

void Task_Ball_Contral_Pop_Init(void){
    Ball_Contral_Emm_Quick_Init(&BallContral);
}

void Task_Ball_Contral_Pop_Ready(void){
    Ball_Contral_Pop_Run(&BallContral, -8000);
}

void Task_Ball_Contral_Pop_Restore(void){
    Ball_Contral_Pop_Run(&BallContral, 8000);
}
