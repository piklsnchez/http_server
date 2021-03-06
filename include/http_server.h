#ifndef __HTTP_SERVER_H
#include "socket.h"
#include "hash_map.h"

struct http_server_request{
  char method[10];
  char version[10];
  char resource[55];
};

struct http_server{
  struct socket*              sock;
  struct http_server_request* request;
  char* _buffer;
  void                        (*start)(struct http_server* this);
  void*                       (*processRequest)(struct http_server* this);
  struct http_server_request* (*parseRequest)(struct http_server* this);
  struct hash_map*            (*parseHeaders)(struct http_server* this);
  char*                       (*parseBody)(struct http_server* this);
  void                        (*doGet)(struct http_server* this);
  void                        (*doPost)(struct http_server* this);
  void                        (*doHead)(struct http_server* this);
  void                        (*doWebsocket)(struct http_server* this, struct hash_map* headers);
  char*                       (*toString)(struct http_server* this);
  void                        (*delete)();
};

struct http_server*         http_server_new();
struct http_server*         http_server_new1(struct socket* sock);
void                        http_server_start(struct http_server* this);
void*                       http_server_processRequest(struct http_server* this);
struct http_server_request* http_server_parseRequest(struct http_server* this);
struct hash_map*            http_server_parseHeaders(struct http_server* this);
char*                       http_server_parseBody(struct http_server* this);
void                        http_server_doGet(struct http_server* this);
void                        http_server_doPost(struct http_server* this);
void                        http_server_doHead(struct http_server* this);
void                        http_server_doWebsocket(struct http_server* this, struct hash_map* headers);
void                        http_server_delete(struct http_server* this);
char*                       http_server_toString(struct http_server* this);

char* trimLeadingSpaces(char* trim);
#endif
