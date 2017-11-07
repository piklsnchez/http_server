#ifndef SOCKET_H
#define SOCKET_H
#include <netinet/in.h>
#include <sys/socket.h>

struct socket{
  int fd;
  struct sockaddr_in serverAddress;
  char* _buffer;
  int (*sConnect)     (struct socket* this, const char* host, int port);
  int (*sBind)        (struct socket* this);
  int (*sListen)      (struct socket* this);
  int (*sAccept)      (struct socket* this, struct sockaddr_in* address);
  char* (*sRead)      (struct socket* this);
  uint8_t (*sReadByte)(struct socket* this);
  char* (*sReadLine)  (struct socket* this);
  void (*sWrite)      (struct socket* this, char* message);
  void (*sWriteBinary)(struct socket* this, uint8_t* data, int len);
  char* (*toString)   (struct socket* this);
  void (*delete)      (struct socket* this);
};
struct socket* socket_new();
struct socket* socket_new1(int fd);
int socket_connect     (struct socket* this, const char* host, int port);
int socket_bind        (struct socket* this);
int socket_listen      (struct socket* this);
int socket_accept      (struct socket* this, struct sockaddr_in* address);
char* socket_read      (struct socket* this);
uint8_t socket_readByte(struct socket* this);
char* socket_readLine  (struct socket* this);
void socket_write      (struct socket* this, char* message);
void socket_writeBinary(struct socket* this, uint8_t* data, int len);
char* socket_toString  (struct socket* this);
void socket_delete     (struct socket* this);
#endif
