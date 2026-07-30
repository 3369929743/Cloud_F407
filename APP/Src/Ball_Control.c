#include "Ball_Contral.h"

static float BallContral_Abs(float Value)
{
    return (Value >= 0.0f) ? Value : -Value;
}

static float BallContral_Clamp(float Value, float Min, float Max)
{
    if (Value > Max) return Max;
    if (Value < Min) return Min;
    return Value;
}

static int32_t BallContral_RoundToInt(float Value)
{
    return (int32_t)((Value >= 0.0f) ? (Value + 0.5f) : (Value - 0.5f));
}

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

    /* Safe defaults. The task layer should replace VisionUnitsPerCm with the
       measured value from the real camera. */
    BallContral->Motion.VisionUnitsPerCm = 1.0f;
    BallContral->Motion.ControlPeriodS = 0.01f;
    BallContral->Motion.ReferenceSpeedCmS = 8.0f;
    BallContral->Motion.VelocityFilterAlpha = 0.80f;
    BallContral->Motion.VisionDelayS = 0.06f;
    BallContral->Motion.VelocityGain = 0.02f;
    BallContral->Motion.PositionToleranceCm = 0.5f;
    BallContral->Motion.VelocityToleranceCmS = 1.0f;
    BallContral->Motion.StableSamples = 15;
    BallContral->Motion.MaxPulsePerCycle = 200;

    BallContral->is_Enable = 0;
    BallContral->VisionValid = 0;
    BallContral->is_Reached = 0;
    BallContral->VisionZero = 0.0f;
    BallContral->TargetCm = 0.0f;
    BallContral->ReferenceCm = 0.0f;
    BallContral->PositionCm = 0.0f;
    BallContral->PreviousPositionCm = 0.0f;
    BallContral->VelocityCmS = 0.0f;
    BallContral->TimeSinceVisionS = BallContral->Motion.ControlPeriodS;
    BallContral->StableCount = 0;

    /* 初始化Emm位置快速控制模式 */
    Emm_Pos_Control_Quick_Init(&BallContral->Emm_StepMotor);
}

void BallContral_ConfigMotion(BallContral_t *BallContral, const BallContral_Motion_Confg_t *Config)
{
    if ((BallContral == 0) || (Config == 0)) return;

    BallContral->Motion = *Config;
    if (BallContral->Motion.VisionUnitsPerCm <= 0.0f) {
        BallContral->Motion.VisionUnitsPerCm = 1.0f;
    }
    if (BallContral->Motion.ControlPeriodS <= 0.0f) {
        BallContral->Motion.ControlPeriodS = 0.01f;
    }
    BallContral->Motion.VelocityFilterAlpha = BallContral_Clamp(
        BallContral->Motion.VelocityFilterAlpha, 0.0f, 0.99f);
    if (BallContral->Motion.StableSamples == 0U) {
        BallContral->Motion.StableSamples = 1U;
    }
    if (BallContral->Motion.MaxPulsePerCycle < 0) {
        BallContral->Motion.MaxPulsePerCycle = -BallContral->Motion.MaxPulsePerCycle;
    }
}

void Ball_Contral_Emm_Quick_Init(BallContral_t *BallContral){
    Emm_Pos_Control_Quick_Init(&BallContral->Emm_StepMotor);
}

void Ball_Contral_Pop_Run(BallContral_t *BallContral, int32_t Pulse){
    Emm_Pos_Run_Quick(&BallContral->Emm_StepMotor, Pulse);
}

void BallContral_Set_Target(BallContral_t *BallContral, PID_val Target)
{
    BallContral_GotoCm(BallContral, (float)Target / BallContral->Motion.VisionUnitsPerCm);
}

void BallContral_Run(BallContral_t *BallContral, PID_val Target){
    BallContral_UpdateVision(BallContral, (float)Target);
    BallContral_Control(BallContral);
}

/**
 * @brief Set a final ball position in centimetres relative to the visual zero.
 * @note The internal reference moves towards this target with a speed limit,
 *       so a 0 -> +5 cm or +5 -> -5 cm command is not a hard position step.
 */
void BallContral_GotoCm(BallContral_t *BallContral, float TargetCm)
{
    if (BallContral == 0) return;

    if (TargetCm != BallContral->TargetCm) {
        BallContral->TargetCm = TargetCm;
        BallContral->StableCount = 0;
        BallContral->is_Reached = 0;
        PID_Clear(&BallContral->PID_StepMotor);
    }
}

void BallContral_AdvanceTime(BallContral_t *BallContral, float DeltaTimeS)
{
    if ((BallContral == 0) || (DeltaTimeS <= 0.0f)) return;
    BallContral->TimeSinceVisionS += DeltaTimeS;
}

/**
 * @brief Feed one new visual measurement to the state estimator.
 * @warning Call once per NEW camera frame. Reusing an old frame would make the
 *          estimated velocity incorrectly converge to zero.
 */
void BallContral_UpdateVision(BallContral_t *BallContral, float VisionPosition)
{
    float PositionCm;
    float RawVelocity;
    float Alpha;
    float PositionError;

    if (BallContral == 0) return;

    PositionCm = (VisionPosition - BallContral->VisionZero) /
                 BallContral->Motion.VisionUnitsPerCm;

    if (!BallContral->VisionValid) {
        BallContral->PositionCm = PositionCm;
        BallContral->PreviousPositionCm = PositionCm;
        BallContral->VelocityCmS = 0.0f;
        BallContral->ReferenceCm = PositionCm;
        BallContral->VisionValid = 1;
    } else {
        float SamplePeriodS = BallContral->TimeSinceVisionS;
        if (SamplePeriodS < BallContral->Motion.ControlPeriodS) {
            SamplePeriodS = BallContral->Motion.ControlPeriodS;
        }
        RawVelocity = (PositionCm - BallContral->PreviousPositionCm) / SamplePeriodS;
        Alpha = BallContral->Motion.VelocityFilterAlpha;
        BallContral->VelocityCmS = Alpha * BallContral->VelocityCmS +
                                   (1.0f - Alpha) * RawVelocity;
        BallContral->PositionCm = PositionCm;
        BallContral->PreviousPositionCm = PositionCm;
    }
    BallContral->TimeSinceVisionS = 0.0f;

    PositionError = BallContral_Abs(BallContral->TargetCm - BallContral->PositionCm);
    if ((PositionError <= BallContral->Motion.PositionToleranceCm) &&
        (BallContral_Abs(BallContral->VelocityCmS) <= BallContral->Motion.VelocityToleranceCmS) &&
        (BallContral_Abs(BallContral->TargetCm - BallContral->ReferenceCm) <=
         BallContral->Motion.PositionToleranceCm)) {
        if (BallContral->StableCount < BallContral->Motion.StableSamples) {
            BallContral->StableCount++;
        }
        if (BallContral->StableCount >= BallContral->Motion.StableSamples) {
            BallContral->is_Reached = 1;
        }
    } else {
        BallContral->StableCount = 0;
        BallContral->is_Reached = 0;
    }
}

/**
 * @brief Run one position-control cycle.
 *
 * Position PID produces the basic tube inclination command. The explicit
 * velocity feedback brakes the ball before it reaches the target. A short
 * prediction compensates the configured camera/processing delay.
 */
void BallContral_Control(BallContral_t *BallContral)
{
    float ReferenceStep;
    float ReferenceError;
    float PredictedPositionCm;
    float MeasurementAgeS;
    float PidOutput;
    float VelocityBrake;
    float Output;
    float MaxPulse;

    if ((BallContral == 0) || !BallContral->is_Enable || !BallContral->VisionValid) return;

    ReferenceStep = BallContral->Motion.ReferenceSpeedCmS *
                    BallContral->Motion.ControlPeriodS;
    ReferenceError = BallContral->TargetCm - BallContral->ReferenceCm;
    BallContral->ReferenceCm += BallContral_Clamp(ReferenceError,
                                                  -ReferenceStep,
                                                  ReferenceStep);

    /* Do not extrapolate an old frame without bound while approaching the
       vision-loss timeout. */
    MeasurementAgeS = BallContral_Clamp(BallContral->TimeSinceVisionS,
                                        0.0f, 0.10f);
    PredictedPositionCm = BallContral->PositionCm + BallContral->VelocityCmS *
                          (BallContral->Motion.VisionDelayS + MeasurementAgeS);

    /* Keep PID gains compatible with the old vision-unit based controller. */
    PID_Set_Target(&BallContral->PID_StepMotor,
                   BallContral->ReferenceCm * BallContral->Motion.VisionUnitsPerCm);
    PidOutput = PID_Calculate(&BallContral->PID_StepMotor,
                              PredictedPositionCm * BallContral->Motion.VisionUnitsPerCm);
    VelocityBrake = BallContral->Motion.VelocityGain *
                    BallContral->VelocityCmS * BallContral->Motion.VisionUnitsPerCm;
    Output = PidOutput - VelocityBrake;

    MaxPulse = (float)BallContral->Motion.MaxPulsePerCycle;
    if (MaxPulse > 0.0f) {
        Output = BallContral_Clamp(Output, -MaxPulse, MaxPulse);
    }

    Emm_Pos_Run_Quick(&BallContral->Emm_StepMotor,
                      -BallContral_RoundToInt(Output));
}

void BallContral_SetVisionZero(BallContral_t *BallContral, float VisionPosition)
{
    if (BallContral == 0) return;

    BallContral->VisionZero = VisionPosition;
    BallContral->PositionCm = 0.0f;
    BallContral->PreviousPositionCm = 0.0f;
    BallContral->VelocityCmS = 0.0f;
    BallContral->TimeSinceVisionS = BallContral->Motion.ControlPeriodS;
    BallContral->ReferenceCm = 0.0f;
    BallContral->TargetCm = 0.0f;
    BallContral->StableCount = 0;
    BallContral->is_Reached = 0;
    BallContral->VisionValid = 1;
    PID_Clear(&BallContral->PID_StepMotor);
}

uint8_t BallContral_IsReached(const BallContral_t *BallContral)
{
    return (BallContral != 0) ? BallContral->is_Reached : 0U;
}

float BallContral_GetPositionCm(const BallContral_t *BallContral)
{
    return (BallContral != 0) ? BallContral->PositionCm : 0.0f;
}

float BallContral_GetVelocityCmS(const BallContral_t *BallContral)
{
    return (BallContral != 0) ? BallContral->VelocityCmS : 0.0f;
}

uint8_t BallContral_Get_is_Enable(BallContral_t *BallContral){
    return BallContral->is_Enable;
}

void BallContral_Start(BallContral_t *BallContral){
    if (BallContral->VisionValid) {
        BallContral->ReferenceCm = BallContral->PositionCm;
    }
    PID_Clear(&BallContral->PID_StepMotor);
    BallContral->is_Enable = 1;
}

void BallContral_Stop(BallContral_t *BallContral){
    BallContral->is_Enable = 0;
    BallContral->StableCount = 0;
    BallContral->is_Reached = 0;
    PID_Clear(&BallContral->PID_StepMotor);
}
