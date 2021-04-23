/* RelUDP.c: Implementiert ein Multicast UDP mit Ack und Resend, wenn ein
   Empfaenger den Empfang nicht mehr bestaetigt hat 

   Dazu f�gt der Senser jedem UDP-Paket eine Sequenznummer hinzu (um dem 
   Empf�nger zu erm�glichen, einen ReSend zu erkennen). Der Empf�nger
   quitiert jedes empfangene Paket, indem er die Sequenznummer zur�ckschickt.
   Wurde die Sequenznummer in der letzten Zeit schon einmal empfangen, dann
   wird die Nachricht ignoriert.

   Der Sender speichert jede gesendete Nachricht f�r jeden Host in einer Liste. 
   Wird eine Quittung empfangen, dann wird die Nachricht aus der Liste entfernt.
   Der Sender geht die Liste in regelm��igen Abst�nden und nach jedem
   x.ten Senden durch durch und sendet die enthaltenen Nachrichten noch einmal.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "RelUDP.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"

#define FALSE (1==0)
#define TRUE (1==1) 

/*
  Message composition:
  Byte 1: Sequence number
  Byte 2: Origin (0: CANControl, 1...98: Gateways, 99: Traffic relayed from CC
  Byte 3: CANID0
  Byte 4: CANID1
  Byte 5: CANID2
  Byte 6: CANID3
  Byte 7: LEN
  Byte 8: CAN Command
  Byte 9..15: Command data 
 */

// Global

struct RelUDPHost RelFirstSend ;
struct RelUDPHost RelFirstRec ;
struct RelUDPHost RelaySend ;
struct RelUDPHost RelayRec ;
int RelSendSock ;
int SendCurrentSeq ;
int RelCurrentSeq ;
struct addrinfo *RelSendInfo ;


struct RelUDPHost *RelFindHost(struct RelUDPHost *First, char *IP)
{
  struct RelUDPHost *Host ;
  for (Host = First->Next;Host!=NULL;Host=Host->Next) {
    if (strcmp(Host->IP,IP)==0) break ;
  } ;
  return (Host) ;
}

struct RelUDPHost *RelAddHost (struct RelUDPHost *First, char *IP)
{
  struct RelUDPHost *Host ;
  struct RelUDPHost *There ;
  int i ;
  
  Host = malloc(sizeof(struct RelUDPHost)) ;
  if (Host==NULL) {
    perror("Out of mem in RelAddHost") ;
  } ;
  Host->Next = NULL ;
  strcpy (Host->IP,IP) ;
  Host->NotSeen = 0 ;
  for (i=0;i<RELQLEN;i++) Host->Messages[i].len = 0 ;

  for (There=First;There->Next!=NULL;There=There->Next) ; // Link in list
  
  There->Next = Host ;

#ifdef DEBUG
  printf ("Add host %s\n",IP) ;
#endif

  return (Host) ;
}
		
void RelDelHost(struct RelUDPHost *First, struct RelUDPHost *Host) 
{
  struct RelUDPHost *There ;
  
  for (There=First;(There!=NULL)&&(There->Next)!=Host;There=There->Next) ;
  
  if (There==NULL) {
    return ;
  }
  There->Next = Host->Next; 

#ifdef DEBUG
  printf ("Del host %s\n",Host->IP) ;
#endif

  free (Host) ;
}

int RelAddMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen,int SendSocket, struct sockaddr *tap, socklen_t taplen)
{
  int i ;

  i = Buffer[0]&0x7F ; // Mask out repeat flag

  if (i>=RELQLEN) return (-1) ; // Out of ring buffer???

  Host->Messages[i].len = Bufferlen ;
  memcpy (Host->Messages[i].Buffer,Buffer,Bufferlen) ;
  Host->Messages[i].Time = Now ; // Mark receiving time

  Host->SendSocket = SendSocket ;
  Host->tap = tap ;
  Host->taplen = taplen ;

#ifdef DEBUG  
  printf ("Add Message %d to  %s\n",i,Host->IP) ;
#endif

  return (0) ;
} ;

int RelCmpMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen)
{
  int i ;

  i = Buffer[0] ;

  if ((i&0x80)==0) return (FALSE) ; // Message was not repeated, must be added ;

  i&=0x7F ; // Mask out repeat flag

  if (i>=RELQLEN) return (FALSE) ; // Out of ring buffer 


  if ((Host->Messages[i].len==Bufferlen)&&(memcmp(Host->Messages[i].Buffer,Buffer,Bufferlen)==0)) return (TRUE) ;


  return (FALSE) ;
}


void RelDelMessage (struct RelUDPHost *Host, unsigned char SeqNr)
{
  Host->Messages[SeqNr].len = 0 ; // Mark as deleted
  Host->NotSeen = 0 ; // Host has sent an Ack
#ifdef DEBUG
  printf ("Del Message %d to  %s\n",SeqNr,Host->IP) ;
#endif
}

void relworkqueue ()
{
  struct RelUDPHost *Host,*Next ;
  int i,j ;
  static int Count = 0 ;
  unsigned char Buf[RELBUFLEN] ;

  i = Count ;
  
  for (Host=RelFirstSend.Next;Host!=NULL;Host=Next) {
    Next = Host->Next ; // if Host gets deleted ;
    if (Host->Messages[i].len!=0) {
#ifdef DEBUG
      printf ("ReSend %d to %s\n",i,Host->IP) ;
#endif
      for (j=0;j<RELBUFLEN;j++) Buf[j]=Host->Messages[i].Buffer[j] ;
      Buf[0]|=0x80 ; // Mark it as re-send

      sendto(Host->SendSocket,Buf,Host->Messages[i].len,0,Host->tap,Host->taplen) ;
      Host->NotSeen++ ;
    } ;
    if (Host->NotSeen>50) RelDelHost(&RelFirstSend,Host) ; // Seems to be gone for good...
  }
  if (RelaySend.NotSeen<50) {
    if (RelaySend.Messages[i].len!=0) {
#ifdef DEBUG
      printf ("ReSend %d to Gateway\n",i) ;
#endif
      for (j=0;j<RELBUFLEN;j++) Buf[j]=RelaySend.Messages[i].Buffer[j] ;
      Buf[0]|=0x80 ; // Mark it as re-send
      sendto(RelaySend.SendSocket,Buf,RelaySend.Messages[i].len,0,RelaySend.tap,RelaySend.taplen) ;
      RelaySend.NotSeen++ ;
    } ;
  } ;

  // Go through all queues and delete "old" Messages (received more than 2 seconds ago)
  for (Host=RelFirstRec.Next;Host!=NULL;Host=Next) 
    for (j=0;j<RELQLEN;j++) 
      if (Host->Messages[j].Time.tv_sec<(Now.tv_sec-2)) Host->Messages[j].len=0 ;
  
  Count++ ;
  if (Count>=RELQLEN) Count = 0 ;
}

int relrecvfrom (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t *taplen, int *Relay)
{
  int numbytes ;
  struct RelUDPHost *Host ;
  char IP[INET_ADDRSTRLEN]; 
  unsigned char Buf[RELBUFLEN] ;
  unsigned char Ack[RELBUFLEN] ;

  if ((numbytes = recvfrom(Socket, Buf, RELBUFLEN , flag,
			   (struct sockaddr*)tap, taplen)) == -1) {
    return(1);
  }
  

  inet_ntop(AF_INET,(const void*)&(tap->sin_addr),IP,INET_ADDRSTRLEN) ;



  if (numbytes<5) { // This is an acknowledgement...
#ifdef DEBUG
    printf ("Got Ack for %d from %s:%d\n",Buf[0],IP,Buf[1]); 
#endif
    Host = RelFindHost(&RelFirstSend,IP) ;
    if ((Host==NULL)&&(*Relay!=(int)Buf[1])) { // This host is not registered, yet, so register it if it is not us...
      if (Buf[1]!=99) { 
	Host=RelAddHost(&RelFirstSend,IP) ;
      } 
#ifdef DEBUG
      printf ("in Ack\n") ;
#endif
    } ;
    if (Buf[1]==99) {
      RelaySend.NotSeen = 0 ;
      Host = &RelaySend ;
    } ;
    
    if (Host!=NULL) RelDelMessage(Host,Buf[0]) ;
    
    return (0) ; // was an ack, so nothing to do any more
  } ;
  
  // not an acknowledgement, so ack it if it not from us...

  // Check if no system message or bootstrap, if yes, don't store it...  
  if ((Buf[7]>TIME)&&(Buf[7]<SUCCESSFULL_RESPONSE)) {    
    if (*Relay!=(int)Buf[1]) {
      Ack[0] = Buf[0]&0x7F ;   // Ack original Message 
      if (Buf[1]!=99) {
	Ack[1] = *Relay ;
      } else {
	Ack[1] = 99 ;
      }
#ifdef DEBUG
      printf ("Ack %d to %s:%d\n",Buf[0],IP,Buf[1]) ;
#endif
      sendto(RelSendSock,Ack,2,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
      
      // Check if Message has already been received
      
#ifdef DEBUG
      printf ("Received Mesg %d from %s\n",Buf[0],IP); 
#endif
      

      // This makes sense, as repeated time infos are not critical but are repeated
      // often enough that a 1:3 chance exists that the ringbuffer still contains a 
      // time info message with the same sequence number for the same CAN channel...
      // Byte 7 in the buffer is the CAN Command
      Host = RelFindHost(&RelFirstRec,IP) ;
      if (Host==NULL) { // This host is not registered, yet, so register it
	Host=RelAddHost(&RelFirstRec,IP) ; // it will never be deleted, though
#ifdef DEBUG
	printf ("in Received\n") ;
#endif
      } else {
	if (RelCmpMessage(Host,Buf,numbytes)) { // we already received it 
#ifdef DEBUG
	  int h ;
	  printf ("Already got it from %s: ",Host->IP) ;
	  for (h=0;h<15;h++) printf("%d ",Buf[h]) ;
	  printf ("\n") ;
#endif
	  return (0) ;
	} ;
      }
      RelAddMessage (Host,Buf,numbytes,Socket,(struct sockaddr*)tap,*taplen) ;
#ifdef DEBUG
      {
	int h ;
	printf ("Add message to receive queue from %s: ",Host->IP) ;
	for (h=0;h<15;h++) printf("%d ",Buf[h]) ;
	printf ("\n") ;
      } ;
#endif
    } ;
  } ;

  // Return Message without sequence counter only
  memcpy (Buffer,&(Buf[2]),numbytes-2) ;
  
  *Relay = (int)Buf[1]; 
  
  return (numbytes-2) ;
}

int relsendto (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t taplen,int Relay)
{
  int numbytes ;
  struct RelUDPHost *Host ;
  unsigned char Buf[RELBUFLEN] ;
  
  if (Bufferlen>(RELBUFLEN-2)) return (-1) ;

  memcpy (&(Buf[2]),Buffer,Bufferlen) ;

  if ((Buf[7]>TIME)&&(Buf[7]<SUCCESSFULL_RESPONSE)) {    
    if (Relay!=99) {
      SendCurrentSeq++ ; // Implement ringbuffer
      if (SendCurrentSeq>=RELQLEN) SendCurrentSeq=0 ;  
      Buf[0] = SendCurrentSeq ;
    } else {
      RelCurrentSeq++ ; // Implement ringbuffer
      if (RelCurrentSeq>=RELQLEN) RelCurrentSeq=0 ;  
      Buf[0] = RelCurrentSeq ;
    }; 
  } ;

  Buf[1] = Relay ;

#ifdef DEBUG
  if (Relay!=99) {
    printf ("Send Mesg %d to All\n",Buf[0]); 
  } else {
    printf ("Send Mesg %d to Relay\n",Buf[0]); 
  }; 
#endif

  if ((numbytes = sendto(Socket, Buf, Bufferlen+2, flag, (struct sockaddr*)tap, taplen)) != Bufferlen+2) {
    perror("relsendto nicht erfolgreich");
    return(-1);
  }

  if ((Buf[7]>TIME)&&(Buf[7]<SUCCESSFULL_RESPONSE)) {    
    if (Relay!=99) {
      for (Host=RelFirstSend.Next;Host!=NULL;Host=Host->Next) RelAddMessage(Host,Buf,Bufferlen+2,Socket,(struct sockaddr*)tap,taplen) ;
    } else {
      RelAddMessage(&RelaySend,Buf,Bufferlen+2,Socket,(struct sockaddr*)tap,taplen) ;
    } ;
  } ;

  return (numbytes-2); 
}


void relinit (int SendSocket, struct addrinfo *SendInfo)
{
  RelSendSock = SendSocket ;
  RelSendInfo = SendInfo ;
}
