/* RelUDP.c: Implementiert ein Multicast UDP mit Ack und Resend, wenn ein
   Empfaenger den Empfang nicht mehr bestaetigt hat 

   Dazu fügt der Senser jedem UDP-Paket eine Sequenznummer hinzu (um dem 
   Empfänger zu ermöglichen, einen ReSend zu erkennen). Der Empfänger
   quitiert jedes empfangene Paket, indem er die Sequenznummer zurückschickt.
   Wurde die Sequenznummer in der letzten Zeit schon einmal empfangen, dann
   wird die Nachricht ignoriert.

   Der Sender speichert jede gesendete Nachricht für jeden Host in einer Liste. 
   Wird eine Quittung empfangen, dann wird die Nachricht aus der Liste entfernt.
   Der Sender geht die Liste in regelmäßigen Abständen und nach jedem
   x.ten Senden durch durch und sendet die enthaltenen Nachrichten noch einmal.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "RelUDP.h"

#define FALSE (1==0)
#define TRUE (1==1) 

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

  printf ("Add host %s\n",IP) ;

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

  printf ("Del host %s\n",Host->IP) ;

  free (Host) ;
}

int RelAddMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen,int SendSocket, struct sockaddr *tap, socklen_t taplen)
{
  int i ;

  i = Buffer[0] ;
  if (i>=RELQLEN) return (-1) ; // Out of ring buffer???

  Host->Messages[i].len = Bufferlen ;
  memcpy (Host->Messages[i].Buffer,Buffer,Bufferlen) ;
  
  Host->SendSocket = SendSocket ;
  Host->tap = tap ;
  Host->taplen = taplen ;
  
  printf ("Add Message %d to  %s\n",i,Host->IP) ;

  return (0) ;
} ;

int RelCmpMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen)
{
  int i ;

  i = Buffer[0] ;
  if (i>=RELQLEN) return (FALSE) ; // Out of ring buffer???


  if ((Host->Messages[i].len==Bufferlen)&&(memcmp(Host->Messages[i].Buffer,Buffer,Bufferlen)==0)) return (TRUE) ;


  return (FALSE) ;
}


void RelDelMessage (struct RelUDPHost *Host, unsigned char SeqNr)
{
  Host->Messages[SeqNr].len = 0 ; // Mark as deleted
  Host->NotSeen = 0 ; // Host has sent an Ack
  printf ("Del Message %d to  %s\n",SeqNr,Host->IP) ;
}

void relworkqueue ()
{
  struct RelUDPHost *Host,*Next ;
  int i ;
  static int Count = 0 ;

  i = Count ;
  
  for (Host=RelFirstSend.Next;Host!=NULL;Host=Next) {
    Next = Host->Next ; // if Host gets deleted ;
    if (Host->Messages[i].len!=0) {
      printf ("ReSend %d to %s\n",i,Host->IP) ;
      sendto(Host->SendSocket,Host->Messages[i].Buffer,Host->Messages[i].len,0,Host->tap,Host->taplen) ;
      Host->NotSeen++ ;
    } ;
    if (Host->NotSeen>50) RelDelHost(&RelFirstSend,Host) ; // Seems to be gone for good...
  }
  if (RelaySend.NotSeen<50) {
    if (RelaySend.Messages[i].len!=0) {
      printf ("ReSend %d to Gateway\n",i) ;
      sendto(RelaySend.SendSocket,RelaySend.Messages[i].Buffer,RelaySend.Messages[i].len,0,RelaySend.tap,RelaySend.taplen) ;
      RelaySend.NotSeen++ ;
    } ;
  } ;

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
    printf ("Got Ack for %d from %s:%d\n",Buf[0],IP,Buf[1]); 
    Host = RelFindHost(&RelFirstSend,IP) ;
    if ((Host==NULL)&&(*Relay!=(int)Buf[1])) { // This host is not registered, yet, so register it if it is not us...
      if (Buf[1]!=99) { 
	Host=RelAddHost(&RelFirstSend,IP) ;
      } 
      printf ("in Ack\n") ;
    } ;
    if (Buf[1]==99) {
	RelaySend.NotSeen = 0 ;
	Host = &RelaySend ;
    } ;
    
    if (Host!=NULL) RelDelMessage(Host,Buf[0]) ;

    return (0) ; // was an ack, so nothing to do any more
  } ;

  // not an acknowledgement, so ack it if it not from us...
  
  if (*Relay!=(int)Buf[1]) {
    Ack[0] = Buf[0] ;
    if (Buf[1]!=99) {
      Ack[1] = *Relay ;
    } else {
      Ack[1] = 99 ;
    }
    printf ("Ack %d to %s:%d\n",Buf[0],IP,Buf[1]) ;
    sendto(RelSendSock,Ack,2,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
    
    // Check if Message has already been received
    
    printf ("Received Mesg %d from %s\n",Buf[0],IP); 
    
    Host = RelFindHost(&RelFirstRec,IP) ;
    if (Host==NULL) { // This host is not registered, yet, so register it
      Host=RelAddHost(&RelFirstRec,IP) ; // it will never be deleted, though
      printf ("in Received\n") ;
    } else {
      if (RelCmpMessage(Host,Buf,numbytes)) { // we already received it 
	printf ("Already got it from %s: %d %d %d %d %d\n",Host->IP,Buf[0],Buf[1],Buf[2],Buf[3],Buf[4]) ;
	return (0) ;
      } ;
    }
    RelAddMessage (Host,Buf,numbytes,Socket,(struct sockaddr*)tap,*taplen) ;
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

  if (Relay!=99) {
    SendCurrentSeq++ ; // Implement ringbuffer
    if (SendCurrentSeq>=RELQLEN) SendCurrentSeq=0 ;  
    Buf[0] = SendCurrentSeq ;
  } else {
    RelCurrentSeq++ ; // Implement ringbuffer
    if (RelCurrentSeq>=RELQLEN) RelCurrentSeq=0 ;  
    Buf[0] = RelCurrentSeq ;
  }; 


  Buf[1] = Relay ;

  if (Relay!=99) {
    printf ("Send Mesg %d to All\n",Buf[0]); 
  } else {
    printf ("Send Mesg %d to Relay\n",Buf[0]); 
  }; 

  if ((numbytes = sendto(Socket, Buf, Bufferlen+2, flag, (struct sockaddr*)tap, taplen)) != Bufferlen+2) {
    perror("relsendto nicht erfolgreich");
    return(-1);
  }

  if (Relay!=99) {
    for (Host=RelFirstSend.Next;Host!=NULL;Host=Host->Next) RelAddMessage(Host,Buf,Bufferlen+2,Socket,(struct sockaddr*)tap,taplen) ;
  } else {
    RelAddMessage(&RelaySend,Buf,Bufferlen+2,Socket,(struct sockaddr*)tap,taplen) ;
  } ;

  return (numbytes-2); 
}


void relinit (int SendSocket, struct addrinfo *SendInfo)
{
  RelSendSock = SendSocket ;
  RelSendInfo = SendInfo ;
}
