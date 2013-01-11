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
int RelSendSock ;
int RelCurrentSeq ;
struct addrinfo *RelSendInfo ;


struct RelUDPHost *RelFindHost(struct RelUDPHost *First, char *IP)
{
  struct RelUDPHost *Host ;
  for (Host = First->Next;Host!=NULL;Host=Host->Next) if (strcmp(Host->IP,IP)) break ;
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

  return (Host) ;
}
		
void RelDelHost(struct RelUDPHost *First, struct RelUDPHost *Host) 
{
  struct RelUDPHost *There ;
  
  for (There=First;(There!=NULL)&&(There->Next)!=Host;There=There->Next) ;
  
  if (There==NULL) {
    fprintf (stderr,"Unknown host in RelDelHost\n") ;
    return ;
  }
  There->Next = Host->Next; 
  free (Host) ;
}

int RelAddMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen)
{
  int i ;

  i = Buffer[0] ;
  if (i>=RELQLEN) return (-1) ; // Out of ring buffer???

  Host->Messages[i].len = Bufferlen ;
  memcpy (Host->Messages[i].Buffer,Buffer,Bufferlen) ;
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
}

void relworkqueue ()
{
  struct RelUDPHost *Host,*Next ;
  int Sent ;
  int i ;
  static int Count = 0 ;

  if (Count++<10) return ; // Only every 10th time check the queues...
  Count = 0 ;
  
  for (Host=RelFirstSend.Next;Host!=NULL;Host=Next) {
    Next = Host->Next ; // if Host gets deleted ;
    Sent = 0 ;
    for (i=0;i<RELQLEN;i++) 
      if (Host->Messages[i].len!=0) {
	sendto(RelSendSock,Host->Messages[i].Buffer,Host->Messages[i].len,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
	Sent=1 ;
      } ;
    if (Sent==1) Host->NotSeen++ ;
    if (Host->NotSeen>50) RelDelHost(&RelFirstSend,Host) ; // Seems to be gone for good...
  }
}

int relrecvfrom (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t *taplen)
{
  int numbytes ;
  struct RelUDPHost *Host ;
  char IP[INET_ADDRSTRLEN]; 
  unsigned char Buf[RELBUFLEN] ;

  if ((numbytes = recvfrom(Socket, Buf, RELBUFLEN , flag,
			   (struct sockaddr*)tap, taplen)) == -1) {
    fprintf(stderr,"Could not read from socket?\n");
    return(1);
  }
  
  inet_ntop(AF_INET,(const void*)&(tap->sin_addr),IP,INET_ADDRSTRLEN) ;

  if (numbytes<3) { // This is an acknowledgement...
    inet_ntop(AF_INET,(const void*)&(tap->sin_addr),IP,INET_ADDRSTRLEN) ;
    Host = RelFindHost(&RelFirstSend,IP) ;
    if (Host==NULL) { // This host is not registered, yet, so register it
      Host=RelAddHost(&RelFirstSend,IP) ;
    } else {
      RelDelMessage(Host,Buf[0]) ;
    } ;
    return (0) ; // was an ack, so nothing to do any more
  } ;

  // not an acknowledgement, so ack it

  sendto(RelSendSock,Buf,1,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
  
  // Check if Message has already been received

  Host = RelFindHost(&RelFirstRec,IP) ;
  if (Host==NULL) { // This host is not registered, yet, so register it
    Host=RelAddHost(&RelFirstSend,IP) ; // it will never be deleted, though
  } else {
    if (RelCmpMessage(Host,Buf,numbytes)) { // we already received it 
      return (0) ;
    } ;
  }
  RelAddMessage (Host,Buf,numbytes) ;

  // Return Message without sequence counter only
  memcpy (Buffer,&(Buf[1]),numbytes-1) ;
  
  return (numbytes-1) ;
}

int relsendto (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t taplen)
{
  int numbytes ;
  struct RelUDPHost *Host ;
  unsigned char Buf[RELBUFLEN] ;
  
  if (Bufferlen>(RELBUFLEN-1)) return (-1) ;

  memcpy (&(Buf[1]),Buffer,Bufferlen) ;

  RelCurrentSeq++ ; // Implement ringbuffer
  if (RelCurrentSeq>=RELQLEN) RelCurrentSeq=0 ;  

  Buf[0] = RelCurrentSeq ;
  
  if ((numbytes = sendto(Socket, Buf, Bufferlen+1, flag, (struct sockaddr*)tap, taplen)) != Bufferlen+1) {
    perror("relsendto nicht erfolgreich");
    return(-1);
  }

  for (Host=RelFirstSend.Next;Host!=NULL;Host=Host->Next) RelAddMessage(Host,Buf,Bufferlen+1) ;

  return (numbytes-1); 
}


void relinit (int SendSocket, struct addrinfo *SendInfo)
{
  RelSendSock = SendSocket ;
  RelSendInfo = SendInfo ;
}
