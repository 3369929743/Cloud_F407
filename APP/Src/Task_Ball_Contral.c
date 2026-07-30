#include "Task_Ball_Contral.h"
#include "K230.h"
#include "Serial.h"
#include "Ball_Contral.h"
#include "SoftTimer.h"

/* Calibrate before tuning the control gains:
 * VisionUnitsPerCm = visual displacement / actual displacement in cm.
 * Example: a 5 cm move spanning 42 pixels gives 8.4 pixels/cm. */
#define BALL_VISION_UNITS_PER_CM       10.0f
#define BALL_CONTROL_PERIOD_MS         10U
#define BALL_CONTROL_PERIOD_S          0.010f

static Serial_t Serial_K230;
static Serial_t Serial_Emm_Ball;
static SoftTimer_t SoftTimer_Ball_Contral;
static SoftTimer_t SoftTimer_K230;

static BallContral_t BallContral;

static PID_Confg_t PID_Ball_Confg = {
    .Kp = 0.08f,
    .Ki = 0.0f,
    .Kd = 0.0f,
    .OutMax = 200.0f,
    .OutMin = -200.0f,
    .Alpha = 0.2f,
};

static const BallContral_Motion_Confg_t Ball_Motion_Confg = {
    .VisionUnitsPerCm = BALL_VISION_UNITS_PER_CM,
    .ControlPeriodS = BALL_CONTROL_PERIOD_S,
    .ReferenceSpeedCmS = 8.0f,
    .VelocityFilterAlpha = 0.80f,
    .VisionDelayS = 0.06f,
    .VelocityGain = 0.02f,
    .PositionToleranceCm = 0.5f,
    .VelocityToleranceCmS = 1.0f,
    .StableSamples = 15U,
    .MaxPulsePerCycle = 200,
};

static Task_Ball_Contral_State_e Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
static Task_Ball_Sequence_State_e Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_OFF;
static uint8_t Ball_Vision_Has_Value = 0U;
static float Ball_Last_Vision_Position = 0.0f;

/**
  * @brief  更新小球运动序列状态
  * @note   实现小球的自动测试序列：中心 → +5cm → -5cm → 完成
  *         仅在小球到达当前位置时才会切换到下一个状态
  * @retval 无
  */
static void Task_Ball_Contral_Sequence_Update(void)
{
    /* 小球未到达目标位置，不切换状态 */
    if (!BallContral_IsReached(&BallContral)) return;

    switch (Task_Ball_Sequence_State) {
        case TASK_BALL_SEQUENCE_CENTER:
            /* 从中心位置移动到 +5cm */
            Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_POSITIVE_5CM;
            BallContral_GotoCm(&BallContral, 5.0f);
            break;

        case TASK_BALL_SEQUENCE_POSITIVE_5CM:
            /* 从 +5cm 移动到 -5cm */
            Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_NEGATIVE_5CM;
            BallContral_GotoCm(&BallContral, -5.0f);
            break;

        case TASK_BALL_SEQUENCE_NEGATIVE_5CM:
            /* 序列执行完毕 */
            Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_DONE;
            break;

        default:
            break;
    }
}

void Task_Ball_Contral_Init(void)
{
    Serial_Init(&Serial_K230, Serial_3);
    Serial_Init(&Serial_Emm_Ball, Serial_2);
    BallContral_Init(&BallContral, &Serial_K230, &Serial_Emm_Ball, &PID_Ball_Confg);
    BallContral_ConfigMotion(&BallContral, &Ball_Motion_Confg);
    K230_Init(&Serial_K230, 0, 0);

    SoftTimer_Init(&SoftTimer_Ball_Contral, SOFTTIMER_MODE_PERIODIC,
                   BALL_CONTROL_PERIOD_MS);
    SoftTimer_Init(&SoftTimer_K230, SOFTTIMER_MODE_PERIODIC, 200U);
    SoftTimer_Start(&SoftTimer_Ball_Contral);
    SoftTimer_Start(&SoftTimer_K230);

    BallContral_GotoCm(&BallContral, 0.0f);
    Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
    Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_OFF;
    Ball_Vision_Has_Value = 0U;
}

void Task_Ball_Contral_Toggle(void)
{
    if (Task_Ball_Contral_State == TASK_BALL_CONTRAL_IDLE) {
        Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
        BallContral_Start(&BallContral);
    } else {
        Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
        Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_OFF;
        BallContral_Stop(&BallContral);
    }
}

void Task_Ball_Contral_Loop(void)
{
    uint8_t VisionUpdated = 0U;

    if (!SoftTimer_Trigger(&SoftTimer_Ball_Contral)) return;
    BallContral_AdvanceTime(&BallContral, BALL_CONTROL_PERIOD_S);

    /* Keep receiving vision while idle as well, so Set_Zero() is available
       before the controller is started. */
    if (K230_GetFlag() && K230_Error_Update()) {
        SoftTimer_Reset(&SoftTimer_K230);
        Ball_Last_Vision_Position = (float)K230_GetError_x();
        Ball_Vision_Has_Value = 1U;
        VisionUpdated = 1U;
        BallContral_UpdateVision(&BallContral, Ball_Last_Vision_Position);
    }

    switch (Task_Ball_Contral_State) {
        case TASK_BALL_CONTRAL_IDLE:
            break;

        case TASK_BALL_CONTRAL_RUNNING:
            if (SoftTimer_Trigger(&SoftTimer_K230)) {
                BallContral_Stop(&BallContral);
                Task_Ball_Contral_State = TASK_BALL_CONTRAL_LOST;
            } else {
                /* Stable-sample counting and sequence switching only happen on
                   a new frame; the motor loop can still run every 10 ms. */
                if (VisionUpdated) {
                    Task_Ball_Contral_Sequence_Update();
                }
                BallContral_Control(&BallContral);
            }
            break;

        case TASK_BALL_CONTRAL_LOST:
            BallContral_Stop(&BallContral);
            if (VisionUpdated) {
                BallContral_Start(&BallContral);
                Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
            }
            break;

        default:
            Task_Ball_Contral_State = TASK_BALL_CONTRAL_IDLE;
            BallContral_Stop(&BallContral);
            break;
    }
}

void Task_Ball_Contral_Tick(void)
{
    SoftTimer_Update(&SoftTimer_Ball_Contral);
    SoftTimer_Update(&SoftTimer_K230);
}

void Task_Ball_Contral_Goto(float TargetCm)
{
    Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_OFF;
    BallContral_GotoCm(&BallContral, TargetCm);
    BallContral_Start(&BallContral);
    Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
}

uint8_t Task_Ball_Contral_Is_Arrived(void)
{
    return BallContral_IsReached(&BallContral);
}

uint8_t Task_Ball_Contral_Set_Zero(void)
{
    if (!Ball_Vision_Has_Value) return 0U;

    BallContral_SetVisionZero(&BallContral, Ball_Last_Vision_Position);
    return 1U;
}

void Task_Ball_Contral_Start_Sequence(void)
{
    Task_Ball_Sequence_State = TASK_BALL_SEQUENCE_CENTER;
    BallContral_GotoCm(&BallContral, 0.0f);
    BallContral_Start(&BallContral);
    Task_Ball_Contral_State = TASK_BALL_CONTRAL_RUNNING;
}

Task_Ball_Sequence_State_e Task_Ball_Contral_Get_Sequence_State(void)
{
    return Task_Ball_Sequence_State;
}

void Task_Ball_Contral_Pop_Init(void)
{
    Ball_Contral_Emm_Quick_Init(&BallContral);
}

void Task_Ball_Contral_Pop_Ready(void)
{
    Ball_Contral_Pop_Run(&BallContral, -8000);
}

void Task_Ball_Contral_Pop_Restore(void)
{
    Ball_Contral_Pop_Run(&BallContral, 8000);
}