/**
   Flash-Routine
  */
#include "stdio.h"
#include "string.h"
#include "stm32f10x.h"
#include "flash.h"

void FLASH_Boot_Init(void)
{ 
  FLASH_Unlock(); 
  
  /* Clear pending flags (if any) */  
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR | FLASH_FLAG_PGERR );
}

/* Erase complete flash */
uint32_t FLASH_Boot_Erase()
{
  uint32_t i = 0;
  FLASH_Status FLASHStatus=FLASH_COMPLETE ;

  /* Get the sector where start the user flash area */
  for(i = 0; (i < (FLASH_SIZE-(APPLICATION_ADDRESS-0x8000000))/PAGE_SIZE)&&(FLASHStatus==FLASH_COMPLETE); i ++) {
    FLASHStatus = FLASH_ErasePage(APPLICATION_ADDRESS + (PAGE_SIZE * i));
  }
  
  return (0);
}

/* Write Data to Flash */

uint32_t FLASH_Boot_Write(__IO uint32_t* FlashAddress, uint32_t Data)
{
  if (FLASH_ProgramWord(*FlashAddress, Data) == FLASH_COMPLETE) {
    /* Check the written value */
    if (*(uint32_t*)*FlashAddress != Data) {
      /* Flash content doesn't match SRAM content */
      return(2);
    }
    /* Increment FLASH destination address */
    *FlashAddress += 4;
  } else {
    /* Error occurred while writing data in Flash memory */
    return (1);
  }
  return (0);
}
