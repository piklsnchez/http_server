#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]){
  struct http_server* server = http_server_new();
  server->start(server);
  server->delete(server);
}
