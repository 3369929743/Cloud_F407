#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdint.h>

typedef struct FIFO_Struct{
    uint8_t *Data;
    uint16_t Size;
    uint16_t Mask;
    uint16_t Head;
    uint16_t Tail;
}FIFO_t;

void FIFO_Init(FIFO_t *FIFO, uint8_t *Data, uint16_t Size);
void FIFO_Enter(FIFO_t *FIFO, uint8_t Data);
void FIFO_Clear(FIFO_t *FIFO);
uint16_t FIFO_EnterArray(FIFO_t *FIFO, uint8_t *Data, uint16_t Size);
uint8_t FIFO_Exit(FIFO_t *FIFO, uint8_t *Data);
uint8_t FIFO_ExitArray(FIFO_t *FIFO, uint8_t *Data, uint16_t Size);
uint8_t FIFO_isEmpty(FIFO_t *FIFO);
uint16_t FIFO_Get_Count(FIFO_t *FIFO);

#endif /* __FIFO_H__ */
