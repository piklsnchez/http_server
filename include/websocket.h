#ifndef __WEBSOCKET_H
#define __WEBSOCKET_H
#include "socket.h"
#include "hash_map.h"
void     websocket_doWebsocket(struct socket*);
void     websocket_doUpgrade(struct socket*, struct hash_map*);
void     websocket_sendMessage(struct socket*, char*);
char*    websocket_toBase64(uint8_t*);
uint8_t* websocket_sha1(char*);
void     websocket_printBinary(uint8_t*);
char*    websocket_trimLeadingSpaces(char*);
#endif
