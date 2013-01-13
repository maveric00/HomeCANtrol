/* RelUDP.c: Implementiert ein Multicast UDP mit Ack und Resend, wenn ein
   Empfaenger den Empfang nicht mehr bestaetigt hat 
*/

#define RELQLEN 97
#define RELBUFLEN 40

struct RelMsg {
  int len ;
  unsigned char Buffer[RELBUFLEN] ;
} ;

struct RelUDPHost {
  struct RelUDPHost *Next ;
  char IP[INET_ADDRSTRLEN] ;
  int NotSeen ;
  int SendSocket ;
  struct sockaddr *tap ;
  socklen_t taplen ;
  struct RelMsg Messages[RELQLEN] ; 
} ;

struct RelUDPHost *RelFindHost(struct RelUDPHost *First, char *IP);
struct RelUDPHost *RelAddHost (struct RelUDPHost *First, char *IP);
void RelDelHost(struct RelUDPHost *First, struct RelUDPHost *Host) ;
int RelAddMessage (struct RelUDPHost *Host, unsigned char *Buffer, size_t Bufferlen,int SendSocket, struct sockaddr *tap, socklen_t taplen);
void RelDelMessage (struct RelUDPHost *Host, unsigned char SeqNr);
int relrecvfrom (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t *taplen, int *Relay) ;
int relsendto (int Socket,unsigned char *Buffer, size_t Bufferlen, int flag, struct sockaddr_in *tap, socklen_t taplen,int Relay) ;
void relworkqueue (void) ;
void relinit (int SendSocket, struct addrinfo *SendInfo) ;
