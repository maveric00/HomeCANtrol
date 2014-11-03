/********************************************************************************
 Bootloader for STM32F103RCT6 boards
*********************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stm32f10x.h"
#include "main.h"
#include "flash.h"
#include "CANLib.h"
#include "EEProm.h"
#define PAGESIZE_IDENTIFIER 0x06
#define RWW_PAGES 0x78

CAN_InitTypeDef        CAN_InitStructure;

void delay_us(uint32_t time_us)
{
  SysTick->LOAD  = 72 * time_us-1;
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

void boot_jump_to_application(void) 
{
  /* De-Initialize all */
  CAN_DeInit(CAN1);
  GPIO_DeInit (GPIOD) ;
  RCC_DeInit() ;
  FLASH_Lock () ;

  /* Test if user code is programmed starting from address "ApplicationAddress" */
  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000) { 

    /* Set vector table base address */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, APPLICATION_ADDRESS);

    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);

    /* Jump to user application */
    (*(void(**)())(APPLICATION_ADDRESS + 4))();
  }
}

int main(void)
{

  CanTxMsg OutMessage ;
  CanRxMsg InMessage ;
  uint8_t next_message_number = -1;
  uint8_t message_number = -1;
  uint8_t BootLine,BoardLine ;
  uint8_t BootAdd,BoardAdd ;
  uint8_t BoardType ;
  uint8_t Counter ;
  uint8_t Mailbox ;
  uint32_t FlashAddress ;
  uint32_t FlashData ;
  uint8_t* FlashDataPointer ;
  
  BoardAdd = 0xff ; // um zu kennzeichnen, dass das Board noch nicht 
  BoardLine = 0xf ; // spezifiziert ist
  BootAdd = 1 ;
  BootLine = 0 ;
  BoardType = 0xfe ; // Raw STM32F103 board
  FlashDataPointer = (uint8_t*)&FlashData ;

  EEPromInit () ;
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
  
  if (EEprom!=NULL) {
    BoardAdd = EEprom[2]+(EEprom[3]<<8) ;
    BoardLine = EEprom[4] ;
    BootAdd = EEprom[5]+(EEprom[6]<<8) ;
    BootLine = EEprom[7] ;
    BoardType = EEprom[8] ;
  } ;

  /* CAN configuration */
  CAN_Config();
  
  SetFilter(BoardLine,BoardAdd) ;
  
  /* Message configuration */
  Init_TxMessage (&OutMessage) ;
  OutMessage.ExtId = BuildCANId (0,0,BoardLine,BoardAdd,BootLine,BootAdd,0) ;

  Counter = 0 ;
  
  delay_ms(2000+BoardAdd*10); // Address specific delay to avoid bus cluttering at power on
  
  /* Infinite loop */
  while(1) {
    uint8_t command;
    uint16_t page ;

    
    // wait until we receive a new message
    while (CAN_get_message(&InMessage) == NO_MESSAGE) {
      if (Counter<10) {
	OutMessage.Data[0] = UPDATE_REQ ;
	OutMessage.Data[1] = BoardType;
	OutMessage.Data[2] = Counter ;
	OutMessage.Data[3] = PAGESIZE_IDENTIFIER;
	
	// number of writeable pages
	OutMessage.Data[4] = 0;
	OutMessage.Data[5] = RWW_PAGES;
	OutMessage.DLC = 6 ;
	CAN_TransmitWait (&OutMessage) ;
	delay_ms(200) ;
	if (Counter++>5) {
	  // 1 sekunde gewartet, kein Update vorhanden, Applikation anspringen
	  if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000) { 
	    boot_jump_to_application () ;
	  } ;
	  Counter=0 ; // in case that no application available, carry on
	  delay_ms(2000+BoardAdd*10); // Address specific delay to avoid bus cluttering at power on
	} ;
      } ;
    } ;
    
    /* Hier muss ueberprueft werden, ob die Nachricht ueberhaupt fuer uns war... */
    Counter = 20 ;
    command = InMessage.Data[0] ;
    message_number = InMessage.Data[1] ;
    
    // check if the message is a request, otherwise reject it
    if ((command & ~COMMAND_MASK) != REQUEST)
      continue;
    
    if (command>5) {Counter = 0 ; continue ; } ;
    
    command &= COMMAND_MASK;
    
    // check message number
    if (command==IDENTIFY) {
      next_message_number=0 ;
    } else {
      next_message_number++;
      if (message_number != next_message_number) {
	// wrong message number => send NACK
	message_number = next_message_number;
	next_message_number--;
	OutMessage.Data[0] = WRONG_NUMBER_REPSONSE ;
	OutMessage.Data[1] = next_message_number+1 ;
	OutMessage.DLC = 2 ;
	CAN_TransmitWait (&OutMessage) ;
	continue;
      }
    } ;
    
    OutMessage.Data[1] = next_message_number+1 ;
    
    // process command
    switch (command) {
    case IDENTIFY:
      // version and command of the bootloader
      OutMessage.Data[0] = IDENTIFY|SUCCESSFULL_RESPONSE ;
      OutMessage.Data[2] = BoardType;
      OutMessage.Data[3] = PAGESIZE_IDENTIFIER;
      
      // number of writeable pages
      OutMessage.Data[4] = 0;
      OutMessage.Data[5] = RWW_PAGES;
      OutMessage.DLC = 6 ;
      CAN_TransmitWait (&OutMessage) ;
      break;
      
      // --------------------------------------------------------------------
      // set the current address in the page buffer
      
    case SET_ADDRESS:
      
      page = (InMessage.Data[2] << 8) | InMessage.Data[3];      
      FlashAddress = (page*2048+((InMessage.Data[4]<<8)+InMessage.Data[5])) ;
      if (FlashAddress<0x08000000) FlashAddress+=0x08004000 ;
      if (InMessage.DLC == 6 && (FlashAddress<(APPLICATION_ADDRESS+RWW_PAGES*2048))) {
	// If address is set, programming will occur, so unlock and erase flash - this takes some time...
	FLASH_Boot_Init () ;
	FLASH_Boot_Erase () ;
	OutMessage.Data[0] = SET_ADDRESS|SUCCESSFULL_RESPONSE ;
	OutMessage.DLC = 2 ;
	CAN_TransmitWait (&OutMessage) ;
      } else {
	goto error_response;
      }
      break;
      
      // --------------------------------------------------------------------
      // collect data
      
    case DATA:
      if (InMessage.DLC != 6 || (FlashAddress>(APPLICATION_ADDRESS+RWW_PAGES*2048))) {
	goto error_response;
      }
      FlashDataPointer[0]=InMessage.Data[2] ;
      FlashDataPointer[1]=InMessage.Data[3] ;
      FlashDataPointer[2]=InMessage.Data[4] ;
      FlashDataPointer[3]=InMessage.Data[5] ;
      if (FLASH_Boot_Write(&FlashAddress,&FlashData) ){
	goto error_response;
      }
      // copy data
      OutMessage.Data[4] = 0 ;
      OutMessage.Data[0] =  DATA | SUCCESSFULL_RESPONSE ;
      OutMessage.Data[2] = 0;
      OutMessage.Data[3] = 0 ;
      OutMessage.DLC = 5 ;
      CAN_TransmitWait (&OutMessage) ;
      break;
      
      // --------------------------------------------------------------------
      // start the flashed application program
      
    case START_APP:
      OutMessage.Data[0] =  START_APP | SUCCESSFULL_RESPONSE ;
      OutMessage.DLC = 2 ;
      Mailbox = CAN_TransmitWait (&OutMessage) ;
      // wait for the mcp2515 to send the message
      while (CAN_TransmitStatus(CAN1,Mailbox)==CAN_TxStatus_Pending) ;
      delay_ms(10);
      // start application
      if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000) { 
	boot_jump_to_application();
      } ;
      break;
      
      
    error_response:
    default:
      OutMessage.Data[0] = command|ERROR_RESPONSE ;
      OutMessage.DLC = 2 ;
      CAN_TransmitWait (&OutMessage) ;
      break;
    } ;
  }
}


