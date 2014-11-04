/* Definitions for EEProm emulation - read only for Bootloader */

extern uint8_t *EEProm ;
void EEPromInit () ;
void EEPromWriteByte(uint8_t Byte, int Address);
void EEPromFlush (void) ;

