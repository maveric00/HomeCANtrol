/*
** tpm2.c -- Implementation of tpm2.net protocol for Gateway 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <netdb.h>
#include <ctype.h>
#define _XOPEN_SOURCE
#include <time.h>
#include <sys/time.h>

int TPM2FD ;
struct TPM2Packet TPM2Data ;

int InitTPM2(void)
{
  struct addrinfo hints;
  int rv;
  struct sockaddr_in RecAddr;
  int val ;
  
  val = (0==0) ;

  // Reserve Receive-Socket
  if ((TPM2FD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
    perror("CANGateway: TPM2 socket");
    exit(0);
  }
  
  memset(&RecAddr, 0, sizeof(struct sockaddr_in));
  RecAddr.sin_family      = AF_INET;
  RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
  RecAddr.sin_port        = htons(TPM2_NET_PORT);

  // Bind the socket to the Address
  if (bind(TPM2FD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
    // If address in use then try one lower - CANControl might be running
    close (TPM2FD) ;
    perror("CANGateway: TPM2FD socket in use");
    exit(0);
  }
    
  setsockopt(TPM2FD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;  
}

void TPM2_read (void)
{
  int i,j ;
  struct can_frame frame ;
  ULONG CANID ;
  int FrameSize ;
  int Line ;
  int port ;
  struct sockaddr_storage their_addr;
  struct sockaddr_in *tap ;
  size_t addr_len;
  int numbytes ;

  tap = (struct sockaddr_in*)&their_addr ;
  
  addr_len = sizeof their_addr;

  if ((numbytes = recvfrom(TPM2FD, &TPM2Data, sizeof(struct TPM2Packet), 0,
			   (struct sockaddr_in *)tap,(socklen_t*) &addr_len,&Source)) == -1) {
    perror("Lost UDP network (no TPM2 Data)");
  }
  if (numbytes == 0) return ;
  numbytes = numbytes - 6 ;
  if (TPMVerbose==1) {
    fprintf (logfd,"%s: ",LogTime()) ;
    fprintf (logfd,"Received TPM2-Data Block %d\n",TPM2Data.PacketNum) ;
  } ;

  if ((TPM2Data.StartByte!=TPM2_NET_BLOCK_START_BYTE )||(TPM2Data.Type!=TPM2_BLOCK_TYPE_DATA)) {
    fprintf (logfd,"Malformed TPM2-Data-Packet\n") ;
    return ;
  }
  
  if (Universe[0]==TPM2Data.PacketNum) {
    port = 0 ;
  } else if (Universe[1]==TPM2Data.PacketNum) {
    port = 1 ;
  } else {
    fprintf (logfd,"Wrong Packet Number in TPM2-Data\n") ;
    return ;
  } ;

  FrameSize= TPM2Data.FrameSize[0]*256+TPM2Data.FrameSize[1] ;
  if (FrameSize>ARTNET_DMX_LENGTH) {
    fprintf (logfd,"Too large Packet in TPM2-Data\n") ;
    return ;
  } ;
  
  for (i=0;i<ARTNET_DMX_LENGTH/3;) {
    if (CANBuffer[port][i].Add==0) break ; // Finished
    Line = UniverseLine[port] ;
    
    CANID = BuildCANId(0,0,0,253,Line,CANBuffer[port][i].Add,0) ;
    for (j=0;j<8;j+=2) {
      if ((CANBuffer[port][i].Port[j]!=0)||(CANBuffer[port][i].Port[j+1]!=0)) {
	frame.can_id = CANID|CAN_EFF_FLAG ;
	frame.can_dlc = 8 ;
	frame.data[0] = 41; // LOAD_TWO_LED ;
	frame.data[1] = j ;
	k = CANBuffer[port][i].Port[j]*3 ;
	if (k>numbytes) {
	  fprintf (stderr,"TPM2-Packet did not contain enough data\n") ;
	  return;
	} ;
	frame.data[2] = TPM2Data.Data[k] ;
	frame.data[3] = TPM2Data.Data[k+1] ;
	frame.data[4] = TPM2Data.Data[k+2] ;
	k = CANBuffer[port][i].Port[j+1]*3 ;
	if (k>numbytes) {
	  fprintf (stderr,"TPM2-Packet did not contain enough data\n") ;
	  return;
	} ;
	frame.data[5] = TPM2Data.Data[k] ;
	frame.data[6] = TPM2Data.Data[k+1] ;
	frame.data[7] = TPM2Data.Data[k+2] ;
	
	if (RouteIF0[Line]!=0) {
	  if ((numbytes = write(Can0SockFD, &frame, sizeof(struct can_frame))) < 0) {
	    perror("CANGateway: CAN raw socket write");
	    ReInitCAN () ;
	    return;
	  } ;
	  if (TPMVerbose==1) {
	    fprintf (logfd,"%s: ",LogTime()) ;
	    fprintf (logfd,"Sent TPM2-Data Block to CAN0\n") ;
	  } ;
	} ;
	if (RouteIF1[Line]!=0) {
	  if ((numbytes = write(Can1SockFD, &frame, sizeof(struct can_frame))) < 0) {
	    perror("CANGateway: CAN raw socket write");
	    ReInitCAN () ;
	    return;
	  } ;
	  if (TPMVerbose==1) {
	    fprintf (logfd,"%s: ",LogTime()) ;
	    fprintf (logfd,"Sent TPM2-Data Block to CAN0\n") ;
	  } ;
	} ;
      } ;
    } ;
  } ;
}
