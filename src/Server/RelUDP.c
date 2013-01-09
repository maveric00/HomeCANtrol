/* RelUDP.c: Implementiert ein Multicast UDP mit Ack und Resend, wenn ein
   Empfaenger den Empfang nicht mehr bestaetigt hat 
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

struct RelUDPHost FirstRec ;
int RelSendSock ;
struct addrinfo *RelSendInfo ;


struct RelUDPHost *RelFindHost(char *IP)
{
  struct RelUDPHost *Host ;
  for (Host = FirstRec.Next;Host!=NULL;Host=Host->Next) if (strcmp(Host->IP,IP)) break ;
  return (Host) ;
}

struct RelUDPHost *RelAddHost (char *IP)
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

  for (There=&FirstRec;There->Next!=NULL;There=There->Next) ; // Link in list
  
  There->Next = Host ;

  return (Host) ;
}
		
void RelDelHost(struct RelUDPHost *Host) 
{
  struct RelUDPHost *There ;
  
  for (There=&FirstRec;(There!=NULL)&&(There->Next)!=Host;There=There->Next) ;
  
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
  for (i=0;i<RELQLEN;i++) if (Host->Messages[i].len==0) break ;

  if (i==RELQLEN) return (-1) ;
  if (Bufferlen>RELBUFLEN) return (-1) ;

  Host->Messages[i].len = Bufferlen ;
  memcpy (Host->Messages[i].Buffer,Buffer,Bufferlen) ;
  return (0) ;
} ;

int RelDelMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen)
{
  int i ;
  
  for (i=0;i<RELQLEN;i++) 
    if ((Bufferlen==Host->Messages[i].len)&&(memcmp(Buffer,Host->Messages[i].Buffer,Bufferlen)==0)) break ;

  if (i==RELQLEN) return (1) ;

  Host->Messages[i].len = 0 ; // Mark as deleted
  Host->NotSeen = 0 ; // Host has sent an Ack
  return (0) ;
}

int relrecvfrom (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t *taplen)
{
  int numbytes ;
  int i,Sum ;
  struct RelUDPHost *Host ;
  char IP[INET_ADDRSTRLEN]; 

  if ((numbytes = recvfrom(Socket, Buffer, Bufferlen , flag,
			   (struct sockaddr*)tap, taplen)) == -1) {
    fprintf(stderr,"Could not read from socket?\n");
    return(1);
  }

  Sum = 0 ;
  for (i=0;i<numbytes-1;i++) Sum+=Buffer[i]; 

  if ((Sum&0xff)==Buffer[numbytes-1]) { // This is an acknowledgement...
    inet_ntop(AF_INET,(const void*)&(tap->sin_addr),IP,INET_ADDRSTRLEN) ;
    Host = RelFindHost(IP) ;
    if (Host==NULL) { // This host is not registered, yet, so register it
      Host=RelAddHost(IP) ;
    } else {
      if (RelDelMessage(Host,Buffer,numbytes-1)==0) { // was really an acknowledgement...
	// nothing to be processed
	return (0) ;
      } ;
    } ;
  } ;

  // not an acknowledgement, so ack it

  Sum = 0 ;
  for (i=0;i<numbytes;i++) Sum+=Buffer[i];
  Buffer[i] = Sum&0xff ;

  sendto(RelSendSock,Buffer,numbytes+1,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
  
  return (numbytes) ;
}

int relsendto (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t taplen)
{
  int numbytes ;
  struct RelUDPHost *Host ;
  
  if (Bufferlen>RELBUFLEN) return (-1) ;

  Buffer[Bufferlen] = 0 ;

  if ((numbytes = sendto(Socket, Buffer, Bufferlen, flag, (struct sockaddr*)tap, taplen)) != Bufferlen) {
    perror("relsendto nicht erfolgreich");
    return(-1);
  }

  for (Host=FirstRec.Next;Host!=NULL;Host=Host->Next) RelAddMessage(Host,Buffer,Bufferlen) ;

  return (numbytes); 
}

void relworkqueue ()
{
  struct RelUDPHost *Host,*Next ;
  int Sent ;
  int i ;
  static int Count = 0 ;

  if (Count++<10) return ; // Only every 10th time check the queues...

  Count = 0 ;
  
  for (Host=FirstRec.Next;Host!=NULL;Host=Next) {
    Next = Host->Next ; // if Host gets deleted ;
    Sent = 0 ;
    for (i=0;i<RELQLEN;i++) 
      if (Host->Messages[i].len!=0) {
	sendto(RelSendSock,Host->Messages[i].Buffer,Host->Messages[i].len,0,RelSendInfo->ai_addr,RelSendInfo->ai_addrlen) ;
	Sent=1 ;
      } ;
    if (Sent==1) Host->NotSeen++ ;
    if (Host->NotSeen>50) RelDelHost(Host) ; // Seems to be gone for good...
  }
}

void relinit (int SendSocket, struct addrinfo *SendInfo)
{
  RelSendSock = SendSocket ;
  RelSendInfo = SendInfo ;
}
