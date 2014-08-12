/* Flash-Routinen fuer den ST32F103-Bootloader */




typedef  void (*pFunction)(void);

#define APPLICATION_ADDRESS   0x8004000
#define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
#define FLASH_SIZE                        (0x40000)  /* 256 KBytes */

/* Function definition */
void FLASH_Boot_Init(void) ;
uint32_t FLASH_Boot_Erase() ;
uint32_t FLASH_Boot_Write(__IO uint32_t* FlashAddress, uint32_t* Data) ;
