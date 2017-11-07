#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include "socket.h"
#include <string.h>

const int SOCKET_PORT        = 1234;
const int SOCKET_BACKLOG_LEN = 4;
const int SOCKET_BUFFER_LEN  = 1024;

struct socket* socket_new(){
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(fd < 0){
    fprintf(stderr, "Error opening socket\n");
    exit(-1);
  }
  return socket_new1(fd);
}

struct socket* socket_new1(int fd){
  struct socket* this = (struct socket*)malloc(sizeof(struct socket));
  this->fd                            = fd;
  this->serverAddress.sin_family      = AF_INET;
  this->serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  this->serverAddress.sin_port        = htons(SOCKET_PORT);
  this->_buffer      = (char*)calloc(SOCKET_BUFFER_LEN+1, sizeof(char));
  this->sConnect     = socket_connect;
  this->sBind        = socket_bind;
  this->sListen      = socket_listen;
  this->sAccept      = socket_accept;
  this->sRead        = socket_read;
  this->sReadByte    = socket_readByte;
  this->sReadLine    = socket_readLine;
  this->sWrite       = socket_write;
  this->sWriteBinary = socket_writeBinary;
  this->toString     = socket_toString;
  this->delete       = socket_delete;
  return this;
}

int socket_connect(struct socket* this, const char* hostName, int port){
  struct hostent* host = gethostbyname(hostName);
  if(NULL == host){
    fprintf(stderr, "host not found\n");
    exit(0);
  }
  struct sockaddr_in serverAddress;
  memset((struct sockaddr*)&serverAddress, '\0', sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  memmove((char*)&serverAddress.sin_addr.s_addr, (char*)host->h_addr, host->h_length);
  serverAddress.sin_port = htons(port);
  return connect(this->fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
}

int socket_bind(struct socket* this){
  return bind(this->fd, (struct sockaddr*)&this->serverAddress, sizeof(this->serverAddress));
}

int socket_listen(struct socket* this){
  return listen(this->fd, SOCKET_BACKLOG_LEN);
}

int socket_accept(struct socket* this, struct sockaddr_in* address){
  socklen_t len = sizeof(*address);
  return accept(this->fd, (struct sockaddr*)address, &len);
}

/**
 * not thread-safe
 */
char* socket_read(struct socket* this){
  int r = read(this->fd, this->_buffer, SOCKET_BUFFER_LEN);
  this->_buffer[r] = '\0';
  return this->_buffer;
}

/**
 * not thread-safe
 */
uint8_t socket_readByte(struct socket* this){
  uint8_t result;
  read(this->fd, &result, 1);
  //printf("read: %02x\n", result);
  return result;
}

/**
 * not thread-safe
 */
char* socket_readLine(struct socket* this){
  printf("ENTER socket_readLine %s\n", this->toString(this));
  char c;
  int len = 0;
  memset(this->_buffer, '\0', SOCKET_BUFFER_LEN * sizeof(char) + 1);
  while(true){
    int r = read(this->fd, &c, 1);
    if(r < 0){
      fprintf(stderr, "Couldn't read\n");
      return NULL;
    }
    if(c == '\r') continue;
    if(r == 0 || c == '\n'){
      break;
    }
    this->_buffer[len++] = c;
  }
  printf("EXIT readLine %s\n", this->_buffer);
  return this->_buffer;
}

void socket_write(struct socket* this, char* message){
  int s = write(this->fd, message, strlen(message));
  if(s < 0){
    fprintf(stderr, "Couldn't write\n");
  }
}

void socket_writeBinary(struct socket* this, uint8_t* data, int len){
  int s = write(this->fd, data, len);
  if(s < 0){
    fprintf(stderr, "Couldn't write\n");
  }
}

/**
 * not thread-safe
 */
char* socket_toString(struct socket* this){
  sprintf(this->_buffer, "fd: %d, s_addr: %d, sin_port: %d", this->fd, this->serverAddress.sin_addr.s_addr, ntohs(this->serverAddress.sin_port));
  return this->_buffer;
}

void socket_delete(struct socket* this){
  free(this->_buffer);
  close(this->fd);
  free(this);
}
