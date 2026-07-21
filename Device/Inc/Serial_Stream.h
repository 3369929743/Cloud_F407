#ifndef __SERIAL_STREAM_H__
#define __SERIAL_STREAM_H__

#include <stdint.h>
#include "FIFO.h"
#include "Serial.h"

typedef struct Serial_Struct Serial_t;
typedef struct FIFO_Struct FIFO_t;

typedef struct Serial_Stream_Confg_Struct{
    Serial_t *Serial;
    FIFO_t *FIFO;
    uint8_t *FIFO_Buff;
    uint8_t *Rx_Temp_Buff;
    Serial_Num_e Serial_Num;
    uint16_t FIFO_Size;
    uint16_t Rx_Temp_Buff_Size;
} Serial_Stream_Confg_t;

typedef struct Serial_Stream_Struct{
    Serial_t *Serial;
    FIFO_t *FIFO;
    uint8_t *Rx_Temp_Buff;
    uint8_t Serial_Stream_Flag;
    Serial_Num_e Serial_Num;
    uint16_t Rx_Size;
} Serial_Stream_t;

void Serial_Stream_Init(Serial_Stream_t *Serial_Stream, Serial_Stream_Confg_t *Serial_Stream_Confg);
uint8_t Serial_Stream_ReadByte(Serial_Stream_t *Serial_Stream, uint8_t *Byte);
uint16_t Serial_Stream_ReadArray(Serial_Stream_t *Serial_Stream, uint8_t *Array, uint16_t Size);

#endif