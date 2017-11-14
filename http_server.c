#include "http_server.h"
#include "hash_map.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

const int CHUNK_SIZE = 1024;

struct http_server* http_server_new(){
  return http_server_new1(socket_new());
}

struct http_server* http_server_new1(struct socket* sock){
  struct http_server* this = (struct http_server*)malloc(sizeof(struct http_server));
  this->_buffer      = (char*)calloc(CHUNK_SIZE+1, sizeof(char));
  this->sock         = sock;
  this->request      = (struct http_server_request*)malloc(sizeof(struct http_server_request));
  this->delete       = http_server_delete;
  this->start        = http_server_start;
  this->parseRequest = http_server_parseRequest;
  this->parseHeaders = http_server_parseHeaders;
  this->parseBody    = http_server_parseBody;
  this->doGet        = http_server_doGet;
  this->doPost       = http_server_doPost;
  this->doHead       = http_server_doHead;
  this->toString     = http_server_toString;
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
    fprintf(stderr, "Couldn't bind\n");
    return;
  }
  s = this->sock->sListen(this->sock);
  if(s < 0){
    fprintf(stderr, "Couldn't listen\n");
    return;
  }
  struct sockaddr_in clientAddress;
  while(true){
    int fd = this->sock->sAccept(this->sock, &clientAddress);
    if(fd < 0){
      fprintf(stderr, "Couldn't accept\n");
      return;
    }
    struct socket* clientSock = socket_new1(fd);
    //request is cached in http_server->request
    struct http_server_request* req = this->parseRequest(this, clientSock);
    //allocated need to headers->delete
    struct hash_map* headers = this->parseHeaders(this, clientSock);
    //do header stuff
    headers->delete(headers);

    //need to copy this string it's an internal buffer
    char* body = this->parseBody(this, clientSock);
    if(0 == strcmp("GET", req->method)){
      this->doGet(this, clientSock);
    } else if(0 == strcmp("POST", req->method)){
      this->doPost(this, clientSock);
    } else if(0 == strcmp("HEAD", req->method)){
      this->doHead(this, clientSock);
    } else {//bad method
    
    }
  }
}

/**
 * called once and in order
 */
struct http_server_request* http_server_parseRequest(struct http_server* this, struct socket* client){
  printf("ENTER http_server_parseRequest %s %s\n", this->toString(this), client->toString(client));
  char* request = client->sReadLine(client);
  //method 
  strncpy(this->request->method, strtok_r(request, " ", &request), sizeof(this->request->method));
  //resource 
  strncpy(this->request->resource, strtok_r(request, " ", &request), sizeof(this->request->resource));
  //version 
  strncpy(this->request->version, strtok_r(request, " ", &request), sizeof(this->request->version));

  return this->request;
}

/**
 * called once and in order
 * caller must call map->delete
 */
struct hash_map* http_server_parseHeaders(struct http_server* this, struct socket* client){
  printf("ENTER http_server_parseHeaders %s %s\n", this->toString(this), client->toString(client));
  struct hash_map* headers = hash_map_new();
  for(char* h = client->sReadLine(client); strlen(h) > 0; h = client->sReadLine(client)){
    char* key   = strtok_r(h, ":", &h);
    char* value = trimLeadingSpaces(strtok_r(h, ":", &h));
    headers->put(headers, key, value);
  }
  printf("EXIT parseHeaders %s\n", headers->toString(headers));
  return headers;
}

/**
 * called once and in order
 * caller will need to copy this string
 */
char* http_server_parseBody(struct http_server* this, struct socket* client){
  printf("ENTER http_server_parseBody %s %s\n", this->toString(this), client->toString(client));
  char* result = client->sRead(client);
  printf("EXIT parseBody %s\n", result);
  return result;
}

void http_server_doGet(struct http_server* this, struct socket* client){
  printf("ENTER http_server_doGet %s %s\n", this->toString(this), client->toString(client));
  char* url         = this->request->resource;
  char* page        = strtok_r(url, "?", &url);
  page++;//starts with a slash
  printf("page: %s\n", page);
  char* queryString = strtok_r(url, "?", &url);
  FILE* file = fopen(page, "r");
  if(NULL == file){
    char response[] = "HTTP/1.1 404 Not Found\r\n"
	              "Content-Length: 0\r\n\r\n";
    client->sWrite(client, response);
    return;
  }
  //chunked
  char response[] = "HTTP/1.1 200 Ok\r\n"
	            "Content-Type: text/plan; charset=UTF-8\r\n"
		    "Transfer-Encoding: chunked\r\n"
		    "\r\n";
  client->sWrite(client, response);
  char buf[CHUNK_SIZE+3];//trailing \r\n
  char chunkLen[10];
  int r = 0;
  do{
    r = fread(buf, 1, CHUNK_SIZE, file);
    buf[r]   = '\r';
    buf[r+1] = '\n';
    buf[r+2] = '\0';
    sprintf(chunkLen, "%X\r\n", r);
    printf("r: %d, chunkLen: %s\n", r, chunkLen);
    client->sWrite(client, chunkLen);
    client->sWrite(client, buf);
  } while(CHUNK_SIZE == r);
  sprintf(chunkLen, "%X\r\n\r\n", 0);
  client->sWrite(client, chunkLen);
  fclose(file);
  printf("EXIT doGet\n");
}

void http_server_doPost(struct http_server* this, struct socket* client){}

//TODO see if file exists and send proper return code
void http_server_doHead(struct http_server* this, struct socket* client){
  printf("ENTER http_server_doHead %s %s\n", this->toString(this), client->toString(client));
  char response[] = "HTTP/1.1 200 Ok\r\n\r\n";
  client->sWrite(client, response);
  printf("EXIT doHead\n");
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
