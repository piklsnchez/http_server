#include "http_server.h"
#include "websocket.h"
#include "hash_map.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>

const int CHUNK_SIZE = 1024;

struct http_server* http_server_new(){
  return http_server_new1(socket_new());
}

struct http_server* http_server_new1(struct socket* sock){
  struct http_server* this = (struct http_server*)malloc(sizeof(struct http_server));
  this->_buffer        = (char*)calloc(CHUNK_SIZE+1, sizeof(char));
  this->sock           = sock;
  this->request        = (struct http_server_request*)malloc(sizeof(struct http_server_request));
  this->delete         = http_server_delete;
  this->start          = http_server_start;
  this->processRequest = http_server_processRequest;
  this->parseRequest   = http_server_parseRequest;
  this->parseHeaders   = http_server_parseHeaders;
  this->parseBody      = http_server_parseBody;
  this->doGet          = http_server_doGet;
  this->doPost         = http_server_doPost;
  this->doHead         = http_server_doHead;
  this->doWebsocket    = http_server_doWebsocket;
  this->toString       = http_server_toString;
  return this;
}

void http_server_delete(struct http_server* this){
  this->sock->delete(this->sock);
  free(this->_buffer);
  free(this->request);
  free(this);
}

void http_server_start(struct http_server* this){
  int s = this->sock->sBind(this->sock);
  if(s < 0){
    fprintf(stderr, "%s:%d Couldn't bind: %s\n", __func__, __LINE__, strerror(errno));
    return;
  }
  s = this->sock->sListen(this->sock);
  if(s < 0){
    fprintf(stderr, "%s:%d Couldn't listen: %s\n", __func__, __LINE__, strerror(errno));
    return;
  }
  struct sockaddr_in clientAddress;
  while(true){
    int fd = this->sock->sAccept(this->sock, &clientAddress);
    if(fd < 0){
      fprintf(stderr, "%s:%d Couldn't accept\n", __func__, __LINE__);
      return;
    }
    //add client and pass to queue to process async; must client->delete after request
    struct http_server* client = http_server_new1(socket_new1(fd));
    pthread_t work_queue;
    pthread_create(&work_queue, NULL, client->processRequest, client);
  }
}

void* http_server_processRequest(struct http_server* this){
  //request is cached in http_server->request
  struct http_server_request* req = this->parseRequest(this);
  //allocated need to headers->delete
  struct hash_map* headers = this->parseHeaders(this);
  //do header stuff

  //need to copy this string it's an internal buffer
  char* body = this->parseBody(this);
  if(0 == strcmp("GET", req->method)){
    char* upgrade = headers->get(headers, "Upgrade");
    if(NULL != upgrade && strcmp("websocket", upgrade) == 0){
      this->doWebsocket(this, headers);
    } else {
      this->doGet(this);
    }
  } else if(0 == strcmp("POST", req->method)){
    this->doPost(this);
  } else if(0 == strcmp("HEAD", req->method)){
    this->doHead(this);
  } else {//bad method
  
  }
  this->delete(this);
  headers->delete(headers);
  return NULL;
}

/**
 * called once and in order
 */
struct http_server_request* http_server_parseRequest(struct http_server* this){
  printf("ENTER %s\n", __func__);
  char* request = this->sock->sReadLine(this->sock);
  //method 
  strncpy(this->request->method, strtok_r(request, " ", &request), sizeof(this->request->method));
  //resource 
  strncpy(this->request->resource, strtok_r(request, " ", &request), sizeof(this->request->resource));
  //version 
  strncpy(this->request->version, strtok_r(request, " ", &request), sizeof(this->request->version));
  printf("EXIT %s %s\n",__func__, (char*)this->request);
  return this->request;
}

/**
 * called once and in order
 * caller must call map->delete
 */
struct hash_map* http_server_parseHeaders(struct http_server* this){
  printf("ENTER %s\n", __func__);
  struct hash_map* headers = hash_map_new();
  for(char* h = this->sock->sReadLine(this->sock); strlen(h) > 0; h = this->sock->sReadLine(this->sock)){
    char* key   = strtok_r(h, ":", &h);
    char* value = trimLeadingSpaces(strtok_r(h, ":", &h));
    headers->put(headers, key, value);
  }
  printf("EXIT %s %s\n", __func__, headers->toString(headers));
  return headers;
}

/**
 * called once and in order
 * caller will need to copy this string
 */
char* http_server_parseBody(struct http_server* this){
  printf("ENTER %s\n", __func__);
  char* result = this->sock->sRead(this->sock);
  printf("EXIT %s %s\n",__func__, result);
  return result;
}

void http_server_doGet(struct http_server* this){
  printf("ENTER %s\n", __func__);
  char* url         = this->request->resource;
  char* page        = strtok_r(url, "?", &url);
  page++;//starts with a slash
  printf("page: %s\n", page);
  char* queryString = strtok_r(url, "?", &url);
  FILE* file        = fopen(page, "rb");
  if(NULL == file){
    char response[] = "HTTP/1.1 404 Not Found\r\n"
	              "Content-Length: 0\r\n\r\n";
    this->sock->sWrite(this->sock, response);
    return;
  }
  //chunked
  char response[] = "HTTP/1.1 200 Ok\r\n"
	            "Content-Type: text/html; charset=UTF-8\r\n"
		    "Transfer-Encoding: chunked\r\n\r\n";
  this->sock->sWrite(this->sock, response);
  char buf[CHUNK_SIZE+2];//trailing \r\n
  char chunkLen[10];
  int r = 0;
  do{
    r = fread(buf, 1, CHUNK_SIZE, file);
    buf[r]   = '\r';
    buf[r+1] = '\n';
    sprintf(chunkLen, "%X\r\n", r);
    //printf("r: %d, chunkLen: %s\n", r, chunkLen);
    this->sock->sWrite(this->sock, chunkLen);
    this->sock->sWriteBinary(this->sock, (uint8_t*)buf, r+2);
  } while(CHUNK_SIZE == r);
  sprintf(chunkLen, "%X\r\n\r\n", 0);
  this->sock->sWrite(this->sock, chunkLen);
  fclose(file);
  printf("EXIT %s\n", __func__);
}

void http_server_doPost(struct http_server* this){}

//TODO see if file exists and send proper return code
void http_server_doHead(struct http_server* this){
  printf("ENTER %s\n", __func__);
  char response[] = "HTTP/1.1 200 Ok\r\n\r\n";
  this->sock->sWrite(this->sock, response);
  printf("EXIT %s\n", __func__);
}

void http_server_doWebsocket(struct http_server* this, struct hash_map* headers){
  printf("ENTER %s\n", __func__);
  websocket_doUpgrade(this->sock, headers);
  websocket_doWebsocket(this->sock);
  printf("EXIT %s\n", __func__);
}

char* trimLeadingSpaces(char* str){
  while('\0' != str[0] && ' ' == str[0]){
    str++;
  }
  return str;
}

/**
 * not thread-safe
 * returns internal buffer
 */
char* http_server_toString(struct http_server* this){
  sprintf(this->_buffer, "sock: %s, request: %s\n", this->sock->toString(this->sock), (char*)this->request);
  return this->_buffer;
}
