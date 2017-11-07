#include "http_server.h"
#include "hash_map.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

const int CHUNK_SIZE = 1024;

struct http_server* http_server_new(){
  return http_server_new1(socket_new());
}

struct http_server* http_server_new1(struct socket* sock){
  struct http_server* this = (struct http_server*)malloc(sizeof(struct http_server));
  this->request      = (struct http_server_request*)malloc(sizeof(struct http_server_request));
  this->delete       = http_server_delete;
  this->start        = http_server_start;
  this->parseRequest = http_server_parseRequest;
  this->parseHeaders = http_server_parseHeaders;
  this->parseBody    = http_server_parseBody;
  this->doGet        = http_server_doGet;
  this->doPost       = http_server_doPost;
  return this;
}

void http_server_delete(struct http_server* this){
  this->sock->delete(this->sock);
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
    struct socket* clientSock = socket_new(fd);
    //request is cached in http_server->request
    struct http_server_request* req = this->parseRequest(this);
    //allocated need to headers->delete
    struct hash_map* headers = this->parseHeaders(this);
    //do header stuff
    headers->delete(headers);

    //need to copy this string it's an internal buffer
    char* body = this->parseBody(this);
    if(0 == strcmp("GET", req->method)){
      this->doGet(this, clientSock);
    } else if(0 == strcmp("POST", req->method)){
      this->doPost(this, clientSock);
    }
  }
}

/**
 * called once and in order
 */
struct http_server_request* http_server_parseRequest(struct http_server* this){
  char request[255];
  strncpy(request, this->sock->sReadLine(this->sock), sizeof(request));
  char* tmp = request;
  //method 
  strncpy(this->request->method, strtok_r(request, " ", &tmp), sizeof(this->request->method));
  //version 
  strncpy(this->request->version, strtok_r(request, " ", &tmp), sizeof(this->request->version));
  //resource 
  strncpy(this->request->resource, strtok_r(request, " ", &tmp), sizeof(this->request->resource));
  return this->request;
}

/**
 * called once and in order
 * caller must call map->delete
 */
struct hash_map* http_server_parseHeaders(struct http_server* this){
  struct hash_map* headers = hash_map_new();
  for(char* h = this->sock->sReadLine(this->sock); strlen(h) > 0; h = this->sock->sReadLine(this->sock)){
    char* key   = strtok_r(h, ":", &h);
    char* value = trimLeadingSpaces(strtok_r(h, ":", &h));
    headers->put(headers, key, value);
  }
  return headers;
}

/**
 * called once and in order
 * caller will need to copy this string
 */
char* http_server_parseBody(struct http_server* this){
  return this->sock->sRead(this->sock);
}

void http_server_doGet(struct http_server* this, struct socket* client){
  char* url         = this->request->resource;
  char* scheme      = strtok_r(url, "://", &url);
  char* host        = strtok_r(url, "/", &url);
  char* page        = strtok_r(url, "?", &url);
  char* queryString = strtok_r(url, "?", &url);
  DIR* dir = opendir("./");
  if(NULL == dir){
    fprintf(stderr, "Couldn't open directory\n");
    return;
  }
  //TODO subfolders
  struct dirent* entry;
  while(NULL != (entry = readdir(dir))){
    if(0 == strcmp(page, entry->d_name)){
      FILE* file = fopen(page, "r");
      //chunked
      char buf[CHUNK_SIZE];
      for(int r = fread(buf, sizeof(char), CHUNK_SIZE, file); r == CHUNK_SIZE; r = fread(buf, sizeof(char), CHUNK_SIZE, file)){
        this->sock->sWrite(this->sock, buf); 
      }      
      fclose(file);
    }
  }
  closedir(dir);
}

void http_server_doPost(struct http_server* this, struct socket* client){}

char* trimLeadingSpaces(char* str){
  while('\0' != str[0] && ' ' == str[0]){
    str++;
  }
  return str;
}
