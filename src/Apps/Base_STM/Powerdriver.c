/* Powerdriver.c implements a Soft-PWM for the two MOSFET pins of the base-board (TIM1 is used for WS2812 output,
   so no direct PWM possible) */

#include "stm32f10x.h"

uint16_t PowerPWM[2]=0 ;

const uint16_t PWMTable[] = { 0,1,2,3,4,5,6,7,9,10,11,12,14,15,17,18,
			      20,21,23,24,26,28,30,32,34,36,38,40,42,44,46,49,
			      51,54,56,59,62,65,68,71,74,77,80,84,87,91,95,99,
			      102,107,111,115,120,124,129,134,139,144,149,155,161,166,172,179,
			      185,192,198,205,213,220,228,235,244,252,261,269,279,288,298,308,
			      318,329,340,351,363,375,387,400,413,426,440,454,469,484,500,516,
			      533,550,568,586,605,624,644,664,686,707,730,753,777,801,827,853,
			      879,907,936,965,995,1026,1059,1092,1126,1161,1197,1234,1273,1312,1353,1395,
			      1439,1483,1529,1577,1626,1676,1728,1781,1836,1893,1951,2011,2073,2137,2203,2271,
			      2341,2413,2487,2564,2642,2724,2807,2893,2982,3074,3168,3265,3365,3468,3575,3684,
			      3797,3913,4033,4156,4283,4414,4549,4688,4831,4979,5131,5288,5449,5616,5787,5964,
			      6146,6333,6526,6725,6931,7142,7360,7584,7815,8053,8299,8552,8812,9081,9357,9642,
			      9936,10238,10550,10871,11202,11543,11894,12256,12629,13014,13410,13818,14238,14671,15117,15577,
			      16051,16539,17042,17560,18094,18644,19211,19795,20397,21017,21656,22314,22993,23691,24411,25153,
			      25918,26705,27517,28353,29215,30102,31017,31959,32930,33931,34962,36024,37118,38246,39407,40605,
			      41838,43109,44418,45767,47157,48590,50065,51586,53153,54767,56430,58144,59910,61729,63604,65535} ;

void PowerInit (void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  /* TIM2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  
  /* GPIOC clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
  /* GPIO Init */
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // reset Enable Input of 74hct244 (to enable output)
  GPIOC->BRR = 0x00000100 ;
  
  /* Enable the TIM2 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 2; // 36MHz / 65536 / 2 = 274 Hz
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_Pulse = 0;

  TIM_OC1Init(TIM2, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_Pulse = 0;

  TIM_OC2Init(TIM2, &TIM_OCInitStructure);

  TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

  /* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_Update, ENABLE);

  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE);

}

void TIM2_IRQHandler(void)
{
  if (TIM2->SR&TIM_IT_CC1) 
  {
    TIM2->SR = (uint16_t)~TIM_IT_CC1 ;
    // Reset PA11
    GPIOA->BRR = 0x00000800 ;
  }
  else if (TIM2->SR&TIM_IT_CC2) 
  {
    TIM2->SR = (uint16_t)~TIM_IT_CC2 ;
    GPIOA->BRR = 0x00000400 ;
  }
  else if (TIM2->SR&TIM_IT_Update)
  {
    TIM2->SR = (uint16_t)~TIM_IT_Update ;
    if (PowerPWM[0]>0) GPIOA->BSRR = 0x00000400 ;
    if (PowerPWM[1]>0) GPIOA->BSRR = 0x00000800 ;
  }
}

void PowerSet (uint8_t Power1, uint8_t Power2)
{
  PowerPWM[0] = PWMTable[Power1] ;
  PowerPWM[1] = PWMTable[Power2] ;
  
  TIM_SetCompare1 (TIM2,PowerPWM[0]) ;
  TIM_SetCompare2 (TIM2,PowerPWM[1]) ;
}
