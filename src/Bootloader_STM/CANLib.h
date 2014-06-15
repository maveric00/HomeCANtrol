/* Function declarations */
uint32_t BuildCANId (uint8_t Prio, uint8_t Repeat, uint8_t FromLine, uint16_t FromAdd, uint8_t ToLine, uint16_t ToAdd, uint8_t Group) ;
void SetOutMessage (CanRxMsg * RxMessage, CanTxMsg * TxMessage,uint8_t BoardLine,uint16_t BoardAdd) ;
void Init_TxMessage(CanTxMsg *TxMessage) ;
void SetFilter(uint8_t BoardLine,uint16_t BoardAdd) ;
void CAN_Config(void) ;
uint8_t CAN_TransmitWait(CAN_TypeDef* CANx, CanTxMsg* TxMessage)  ;
tCommand CAN_get_message (CanRxMsg* RxMessage) ;

