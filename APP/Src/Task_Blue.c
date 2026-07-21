#include "Task_Blue.h"
#include "Serial_Stream.h"

static Serial_t Serial_Blue;
static FIFO_t FIFO_Blue;
static Serial_Stream_t Serial_Stream_Blue;
static uint8_t Blue_FIFO_Buff[128];    
static uint8_t Blue_RxTemp_Buff[64];
static uint8_t Blue_User_Buff[32];
static Serial_Stream_Confg_t Serial_Stream_Blue_Confg ={
  .Serial = &Serial_Blue,
  .FIFO = &FIFO_Blue,
  .FIFO_Buff = Blue_FIFO_Buff,
  .Rx_Temp_Buff = Blue_RxTemp_Buff,
  .Serial_Num = Serial_2,
  .FIFO_Size = sizeof(Blue_FIFO_Buff),
  .Rx_Temp_Buff_Size = sizeof(Blue_RxTemp_Buff),
};

void Task_Blue_Init(void){
  Serial_Stream_Init(&Serial_Stream_Blue, &Serial_Stream_Blue_Confg);
}

void Task_Blue_Loop(void){
    
}
