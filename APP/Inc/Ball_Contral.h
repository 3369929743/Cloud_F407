#ifndef __BALL_CONTRAL_H__
#define __BALL_CONTRAL_H__

#include "Emm_v5.h"
#include "PID.h"

/**
 * @brief Ball position-loop parameters.
 *
 * VisionUnitsPerCm must be calibrated on the real image. For example, if
 * 5 cm occupies 42 pixels, set it to 42 / 5 = 8.4 vision units/cm.
 */
typedef struct {
    float VisionUnitsPerCm;
    float ControlPeriodS;
    float ReferenceSpeedCmS;
    float VelocityFilterAlpha;
    float VisionDelayS;
    float VelocityGain;
    float PositionToleranceCm;
    float VelocityToleranceCmS;
    uint16_t StableSamples;
    int32_t MaxPulsePerCycle;
} BallContral_Motion_Confg_t;

typedef struct BallContral_Struct{
    uint8_t is_Enable;
    uint8_t VisionValid;
    uint8_t is_Reached;

    Serial_t *Serial_K230;
    Serial_t *Serial_Emm;

    Emm_t Emm_StepMotor;

    PID_t PID_StepMotor;

    BallContral_Motion_Confg_t Motion;
    float VisionZero;
    float TargetCm;
    float ReferenceCm;
    float PositionCm;
    float PreviousPositionCm;
    float VelocityCmS;
    float TimeSinceVisionS;
    uint16_t StableCount;
}BallContral_t;

void BallContral_Init(BallContral_t *BallContral, Serial_t *Serial_K230, Serial_t *Serial_Emm, PID_Confg_t *PID_Confg);
void BallContral_ConfigMotion(BallContral_t *BallContral, const BallContral_Motion_Confg_t *Config);

/* Legacy vision-unit API. New code should use BallContral_GotoCm(). */
void BallContral_Set_Target(BallContral_t *BallContral, PID_val Target);
void BallContral_Run(BallContral_t *BallContral, PID_val Target);

void BallContral_GotoCm(BallContral_t *BallContral, float TargetCm);
void BallContral_AdvanceTime(BallContral_t *BallContral, float DeltaTimeS);
void BallContral_UpdateVision(BallContral_t *BallContral, float VisionPosition);
void BallContral_Control(BallContral_t *BallContral);
void BallContral_SetVisionZero(BallContral_t *BallContral, float VisionPosition);
uint8_t BallContral_IsReached(const BallContral_t *BallContral);
float BallContral_GetPositionCm(const BallContral_t *BallContral);
float BallContral_GetVelocityCmS(const BallContral_t *BallContral);

uint8_t BallContral_Get_is_Enable(BallContral_t *BallContral);
void BallContral_Start(BallContral_t *BallContral);
void BallContral_Stop(BallContral_t *BallContral);
void Ball_Contral_Emm_Quick_Init(BallContral_t *BallContral);
void Ball_Contral_Pop_Run(BallContral_t *BallContral, int32_t Pulse);

#endif
