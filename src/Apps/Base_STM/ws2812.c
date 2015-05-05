/*****************************************************
  WS2812b control
******************************************************/


#include <stdio.h>
#include "stm32f10x.h"
#include "ws2812.h"
#include "../Common_STM/CANLib.h"

// Buffer for LED
rgb_t WSRGB[MAXWSNUM];	
int WSDimmer ;
int CurrentWSNum ;

static uint16_t wstimerVals[WS_DMA_LEN+1];	// buffer for timer/dma, one byte per bit + reset pulse
volatile uint8_t ledBusy = 0;		        // = 1 while dma is sending data to leds

static void WSstartDMA(void);

//-------------------------------------------------------------------------------------------------------------

static uint16_t *rgb2pwm(uint16_t *buf, const uint8_t color)
{
  register uint8_t mask = 0x80;
  
  do {
    if (color & mask) {
      *buf = WS_ONE;
    } else {
      *buf = WS_ZERO;
    } ;
    buf++;
    mask >>= 1;
  } while (mask);
  
  return buf;
}

void WSupdate(void) 
{
  register uint32_t i;
  register rgb_t *r;
  uint16_t *bufp = wstimerVals;
  int c;
  
  for (i = 0; i < CurrentWSNum; i++) {
    r = (rgb_t *)&WSRGB[i];
    c = ((int)r->G * WSDimmer) / 100;
    bufp = rgb2pwm(bufp, (const uint8_t)c);
    c = ((int)r->R * WSDimmer) / 100;
    bufp = rgb2pwm(bufp, (const uint8_t)c);
    c = ((int)r->B * WSDimmer) / 100;
    bufp = rgb2pwm(bufp, (const uint8_t)c);
  }

  // append reset pulse (50us low level)
  for (i = 0; i < WS_RESET_LEN; i++) *bufp++ = 0;
  
  WSstartDMA();		// send it to RGB stripe
}


void WSinit(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef timbaseinit;
  TIM_OCInitTypeDef timocinit;
  NVIC_InitTypeDef nvic_init;
  int i;

  // clear buffer
  
  WSDimmer = 100 ;

  for (i = 0; i < (WS_DMA_LEN - WS_RESET_LEN); i++) wstimerVals[i] = WS_ZERO;
  
  for (; i < WS_DMA_LEN; i++) wstimerVals[i] = 0;
  
  for (i = 0; i < MAXWSNUM; i++) {
    WSRGB[i].B = 0;
    WSRGB[i].G = 0;
    WSRGB[i].R = 0;
  } ;

  ledBusy = 0 ;
  
  // GPIO
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // reset Enable Input of 74hct244 (to enable output)
  GPIOC->BRR = GPIO_Pin_9 ;
  
  // Timer
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
  
  TIM_TimeBaseStructInit(&timbaseinit);
  timbaseinit.TIM_ClockDivision = TIM_CKD_DIV1;
  timbaseinit.TIM_CounterMode = TIM_CounterMode_Up;
  timbaseinit.TIM_Period = WS_TIM_FREQ / WS_OUT_FREQ;
  timbaseinit.TIM_Prescaler = 1 ;
  TIM_TimeBaseInit(TIM1, &timbaseinit);
  
  TIM_OCStructInit(&timocinit);
  timocinit.TIM_OCMode = TIM_OCMode_PWM1;
  timocinit.TIM_OCPolarity = TIM_OCPolarity_High;
  timocinit.TIM_OutputState = TIM_OutputState_Enable;
  timocinit.TIM_Pulse = 0;
  TIM_OC1Init(TIM1, &timocinit);
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig(TIM1, ENABLE);

  TIM_CCxCmd(TIM1, TIM_Channel_1, TIM_CCx_Enable);
  //TIM_Cmd(TIM1, ENABLE);

  // DMA
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_DeInit(DMA1_Channel2) ;

  //  TIM_DMACmd(TIM1, TIM_DMA_CC1, ENABLE);
  DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

  // NVIC for DMA
  nvic_init.NVIC_IRQChannel = DMA1_Channel2_IRQn;
  nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
  nvic_init.NVIC_IRQChannelSubPriority = 1;
  nvic_init.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic_init);

  WSstartDMA();
}




// transfer framebuffer data to the timer
static void WSstartDMA(void)
{
  DMA_InitTypeDef dma_init;
  dma_init.DMA_BufferSize = (WS_RESET_LEN);
  dma_init.DMA_DIR = DMA_DIR_PeripheralDST;
  dma_init.DMA_M2M = DMA_M2M_Disable;
  dma_init.DMA_MemoryBaseAddr = (uint32_t) &(wstimerVals[0]);
  dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable ;
  dma_init.DMA_Mode = DMA_Mode_Normal;
  dma_init.DMA_PeripheralBaseAddr = (uint32_t) &(TIM1->CCR1); 
  dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma_init.DMA_Priority = DMA_Priority_Medium ;

  if (ledBusy)		// last DMA is not finished
    return;
  
  ledBusy = 1;
  dma_init.DMA_BufferSize = WS_RESET_LEN+CurrentWSNum*3*8 ;

  DMA_Cmd(DMA1_Channel2, DISABLE);
  while (DMA1_Channel2->CCR & DMA_CCR1_EN);

  DMA_Init(DMA1_Channel2, &dma_init);
  TIM_DMACmd(TIM1, TIM_DMA_CC1, ENABLE);
  DMA_Cmd(DMA1_Channel2, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
}

// gets called when dma transfer has completed
void DMA1_Channel2_IRQHandler(void)
{
  TIM_Cmd(TIM1, DISABLE);
  DMA_ClearITPendingBit(DMA1_IT_TC2);
  DMA_Cmd(DMA1_Channel2, DISABLE);
  // need to disable this, otherwise some glitches can occur (first bit gets lost)
  TIM_DMACmd(TIM1, TIM_DMA_CC1, DISABLE);
  
  ledBusy = 0;			// get ready for next transfer
}


#define NR_TEST_PATTERNS	12

void WStest(void)
{
  uint32_t i,j;
  uint8_t patterns[NR_TEST_PATTERNS][3] = {
    {0xf0,0x00,0x00},
    {0x00,0xf0,0x00},
    {0x00,0x00,0xf0},
    {0xf0,0xf0,0x00},
    {0xf0,0x00,0xf0},
    {0x00,0xf0,0xf0},
    {0xf0,0xf0,0xf0},
    {0x40,0x40,0x40},
    {0xf0,0xf0,0xf0},
    {0x40,0x40,0x40},
    {0xf0,0xf0,0xf0},
    {0x00,0x00,0x00},
  };
  
  for (i = 0; i < CurrentWSNum; i++) {
    WSRGB[i].R = 0;
    WSRGB[i].G = 0;
    WSRGB[i].B = 0;
  }
  
  for(j=0; j<NR_TEST_PATTERNS; j++)
    {
      for (i = 0; i < CurrentWSNum; i++)
	{
	  WSRGB[i].R = patterns[j][0] * 0.5F;  
	  WSRGB[i].G = patterns[j][1] * 0.5F;
	  WSRGB[i].B = patterns[j][2] * 0.5F;
	}
      while (ledBusy) ;
      WSupdate();
      delay_ms(60);
    }
  
}
