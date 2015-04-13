/* EEProm read routines, only for read access (Bootloader) */
/* Layout: 512 Bytes of Data , valid data is marked with 0xbaca at beginn */
/* Maximum of 8 pages (4 kB), pages are erased if a new page is begun*/

#include "stdio.h"
#include "string.h"
#include "stm32f10x.h"
#include "EEProm.h"

volatile uint8_t *EEProm ;

void EEPromInit ()
{
  uint8_t *There ;
  
  EEProm = NULL ;

  for (There = (uint8_t*) 0x8003000;
       ((There[0]!=0xba)||(There[1]!=0xca))&&((uint32_t)There<0x8004000);
       There=There+0x200) ;

  if ((uint32_t)There==0x8004000) return ; // no valid configuration, yet

  for (There = There+0x200;
       ((There[0]==0xba)&&(There[1]==0xca))&&((uint32_t)There<0x8004000);
       There=There+0x200) ;

  EEProm = There-0x200 ; // Point to last configuration
  return ;
}



    
  
  
  
