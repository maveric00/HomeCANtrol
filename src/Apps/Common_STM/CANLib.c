// BuildCANId baut aus verschiedenen Elementen (Line & Addresse von Quelle und Ziel 
// sowie Repeat-Flag und Gruppen-Flag) den CAN Identifier auf
#include "stdio.h"
#include "string.h"
#include "stm32f10x.h"

#include "CANLib.h"


uint32_t BuildCANId (uint8_t Prio, uint8_t Repeat, uint8_t FromLine, uint16_t FromAdd, uint8_t ToLine, uint16_t ToAdd, uint8_t Group)
{
  return (((uint32_t)(Group&0x1))<<1|((uint32_t)ToAdd)<<2|((uint32_t)(ToLine&0xf))<<10|
	  ((uint32_t)FromAdd)<<14|((uint32_t)(FromLine&0xf))<<22|
	  ((uint32_t)(Repeat&0x1))<<26|((uint32_t)(Prio&0x3))<<27) ;
}

// GetSourceAddress bestimmt aus dem CAN-Identifier der eingehenden Nachricht 
// die Line & Addresse

inline void GetSourceAddress (uint32_t CANId, uint8_t *FromLine, uint16_t *FromAdd)
{
  *FromLine = (uint8_t)((CANId>>22)&0xf) ;
  *FromAdd = (uint16_t) ((CANId>>14)&0xff) ;
}

// Bereite eine TxMessage basierend auf einer RxMessage und unserer Adresse vor

void SetOutMessage (CanRxMsg * RxMessage, CanTxMsg * TxMessage,uint8_t BoardLine,uint16_t BoardAdd)
{
  uint8_t SendLine ;
  uint16_t SendAdd ;
  
  GetSourceAddress(RxMessage->ExtID,&SendLine,&SendAdd) ;
  TxMessage->ExtID = BuildCANId (0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;
  TxMessage->Data[0] = RxMessage->Data[0]|SUCCESSFULL_RESPONSE ;
  TxMessage->IDE = CAN_Id_Extended ;
  TxMessage->DLC = 1 ;
  TxMessage->RTR = 0 ;
}

// Nullt eine TxMessage

void Init_TxMessage(CanTxMsg *TxMessage)
{
  uint8_t ubCounter = 0;

  TxMessage->StdId = 0x00;
  TxMessage->ExtId = 0x00;
  TxMessage->IDE = CAN_Id_Extended;
  TxMessage->DLC = 0;
  TxMessage->RTR = 0;
  for (ubCounter = 0; ubCounter < 8; ubCounter++)
  {
    TxMessage->Data[ubCounter] = 0x00;
  }
}


void Convert_ID(const uint32_t id, uint16_t *high, uint16_t *low)
{
  *high = (id<<3)>>16 ;
  *low = (0XFFFF)&((id<<3)|4);
}

void SetFilter(uint8_t BoardLine,uint16_t BoardAdd)
{
  CAN_FilterInitTypeDef  InitStruct;
  uint32_t FilterID ;

  FilterID = ((uint32_t)BoardAdd)<<2|((uint32_t)BoardLine)<<10 ;

  // Filter for board adress
  InitStruct.CAN_FilterNumber = 0;
  InitStruct.CAN_FilterMode = CAN_FilterMode_IdMask;
  InitStruct.CAN_FilterScale = CAN_FilterScale_32bit;
  Convert_ID(FilterID,InitStruct.CAN_FilterIdHigh,&InitStruct.CAN_FilterIdLow);
  Convert_ID(0x3FFC,&InitStruct.CAN_FilterMaskIdHigh,&InitStruct.CAN_FilterMaskIdLow) ;
  InitStruct.CAN_FilterFIFOAssignment = 0;
  InitStruct.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&InitStruct);

  // Filter for broadcast adress

  FilterID = ((uint32_t)0xff)<<2|((uint32_t)BoardLine)<<10 ;
  InitStruct.CAN_FilterNumber = 1;
  InitStruct.CAN_FilterMode = CAN_FilterMode_IdMask;
  InitStruct.CAN_FilterScale = CAN_FilterScale_32bit;
  Convert_ID(FilterID,InitStruct.CAN_FilterIdHigh,&InitStruct.CAN_FilterIdLow);
  Convert_ID(0x3FFC,&InitStruct.CAN_FilterMaskIdHigh,&InitStruct.CAN_FilterMaskIdLow) ;
  InitStruct.CAN_FilterFIFOAssignment = 1;
  InitStruct.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&InitStruct);
}


void CAN_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* GPIO clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

  /* Configure CAN pin: RX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Configure CAN pin: TX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  GPIO_PinRemapConfig(GPIO_Remap2_CAN1 , ENABLE);
  
  /* CANx Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
  
  /* CAN register init */
  CAN_DeInit(CAN1);
  CAN_StructInit(&CAN_InitStructure);

  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = ENABLE;
  CAN_InitStructure.CAN_AWUM = ENABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = ENABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  
  /* CAN Baudrate = 250kBps*/
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
  CAN_InitStructure.CAN_BS1 = CAN_BS1_12tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS2_5tq;
  CAN_InitStructure.CAN_Prescaler = 8;
  CAN_Init(CAN1, &CAN_InitStructure);
}

uint8_t CAN_send_message(CanTxMsg* TxMessage) 
{
  uint8_t Mailbox ;
  while ((Mailbox=CAN_Transmit(CAN1,TxMessage))==CAN_TxStatus_NoMailBox) ;
  return (Mailbox) ;
} 

tCommand CAN_get_message (CanRxMsg* RxMessage)
{
  if (CAN_MessagePending(CAN1,CAN_FIFO0)>0) {
    CAN_Receive(CAN1,CAN_FIFO0,RxMessage) ;
    return (REQUEST) ;
  } ;
  if (CAN_MessagePending(CAN1,CAN_FIFO1)>0) {
    CAN_Receive(CAN1,CAN_FIFO1,RxMessage) ;
    return (REQUEST) ;
  } ;
  return (NO_MESSAGE)
}

void delay_us(uint32_t time_us)
{
  SysTick->LOAD  = 72 * time_us-20;
  SysTick->VAL   = 0;                                          /* Load 
								  the SysTick Counter Value */
  SysTick->CTRL  |= SysTick_CTRL_ENABLE_Msk;                   /* Enable 
								  SysTick Timer */
  
  do{ } while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG)==0);
  SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;                  /* Disable SysTick Timer */
  SysTick->VAL   = 0;                                          /* Load the SysTick Counter Value */
}

void delay_ms(uint32_t time_ms)
{
  while (time_ms>0) {
    delay_us(1000);
    time_ms--;
  }
}
