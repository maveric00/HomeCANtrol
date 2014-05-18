/* EEProm routines*/
/* Layout: 512 Bytes of Data , valid data is marked with 0xbaca at beginn */
/* Maximum of 8 pages (4 kB), pages are erased if a new page is begun*/

#include "stdio.h"
#include "string.h"
#include "stm32f1xx.h"
#include "EEProm.h"

volatile uint8_t *EEProm ;
uint8_t *EEPromFlash ;
uint8_t EEPromDirty=0 ;
uint8_t EEPromBuffer[512] ;

void EEPromInit ()
{
  uint8_t *There ;
  
  EEPromFlash = EEProm = NULL ;

  for (There = (uint8_t*) 0x8003000;
       ((There[0]!=0xba)||(There[1]!=0xca))&&(There<0x8004000);
       There=There+0x200) ;

  if (There==0x8004000) return ; // no valid configuration, yet

  for (There = There+0x200;
       ((There[0]==0xba)&&(There[1]==0xca))&&(There<0x8004000);
       There=There+0x200) ;

  EEPromFlash = EEProm = There-0x200 ; // Point to last configuration
  return ;
}

void EEPromWriteByte(uint8_t Byte, int Address)
{
  int i ;

  if (EEPromDirty==0) {
    /* Copy content of Flash in Buffer */
    for (i=0;i<512;i++) EEPromBuffer[i]=EEPromFlash[i] ;
    EEProm = EEPromBuffer ;
    EEPromDirty = 1 ;
  } ;
  EEProm[Address] = Byte ;
} ;

void EEPromFlush (void)
{
  int i ;
  uint8_t *Data ;
  uint8_t *Flash ;

  if (EEPromDirty=0) return ; // Not modified
  
  if (EEPromFlash==0x8003D00) { // Last of Last Page
    Flash = (uint8_t*)0x8003000 ;
    FLASH_Unlock() ;
    FLASH_Erase_Page(0x8003800) ;
  } else if (EEPromFlash==0x8003600) {
    Flash = (uint8_t*)0x8003800 ;
    FLASH_Unlock() ;
    FLASH_Erase_Page(0x8003000) ;
  } else {
    Flash = EEPromFlash+0x200 ;
  } ;

  Data = EEProm ;

  for (i=0;i<128;i++) {
    if (FLASH_ProgramWord(*Flash, *(uint32_t*)(Data)) == FLASH_COMPLETE) {
      /* Check the written value */
      if (*(uint32_t*)*Flash != *(uint32_t*)(Data)) {
	/* Flash content doesn't match SRAM content */
	/* Delete EEProm Area to enable Bootstrapping */
	FLASH_Unlock() ;
	FLASH_Erase_Page(0x8003000) ;
	FLASH_Erase_Page(0x8003800) ;
	return;
      }
      /* Increment FLASH destination address */
      *FlashAddress += 4;
    } else {
      /* Error occurred while writing data in Flash memory */
      /* Delete EEProm Area to enable Bootstrapping */
      FLASH_Unlock() ;
      FLASH_Erase_Page(0x8003000) ;
      FLASH_Erase_Page(0x8003800) ;
      return (1);
    }
  }
}
  


    
  
  
  
