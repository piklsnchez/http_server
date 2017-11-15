#include "websocket.h"
#include "socket.h"
#include <nettle/base64.h>
#include <nettle/sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

const char MAGIC[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char SEP[]   = ":\0";
const char REPLY_FORMAT[] = "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Accept: %s\r\n\r\n";

void websocket_doUpgrade(struct socket* sock, struct hash_map* headers){
  printf("ENTER websocket_doUpgrade %s %s\n", sock->toString(sock), headers->toString(headers));
  char str[255];
  strcpy(str, headers->get(headers, "Sec-WebSocket-Key"));
  strcat(str, MAGIC);
  char result[255];
  uint8_t* sha1_str = websocket_sha1(str);
  char* base64_str  = websocket_toBase64(sha1_str);
  sprintf(result, REPLY_FORMAT, base64_str);
  printf("result: %s\n", result);
  sock->sWrite(sock, result);

  free(sha1_str);
  free(base64_str);
  printf("EXIT doUpgrade\n");
}

void websocket_doWebsocket(struct socket* sock){
  printf("ENTER websocket_doWebsocket %s\n", sock->toString(sock));
  while(true){
    sock->sReadByte(sock);
    uint8_t b = sock->sReadByte(sock);
    if(b < 128){//FIN
      break;
    }
    int     payload_len = b ^ 128;
    uint8_t payload[payload_len];
    int     mask_len    = 4;
    uint8_t mask[mask_len];
    //pull in mask
    for(int i = 0; i < mask_len; i++){
      mask[i] = sock->sReadByte(sock);
    }
    //pull in message
    for(int i = 0; i < payload_len; i++){
      payload[i] = sock->sReadByte(sock);
    }
    char result[payload_len+1];//null terminate
    for(int i = 0; i < payload_len; i++){
      result[i] = payload[i] ^ mask[i % 4];
    }
    result[payload_len] = '\0';
    websocket_sendMessage(sock, result);
  }
  printf("EXIT websocket_doWebsocket\n");
}

/* bits 9 - 15 are message length if less than 126 bit 8 should be 0 */
void websocket_sendMessage(struct socket* sock, char* msg){
  printf("ENTER websocket_sendMessage %s\n", msg);
  int len = strlen(msg);
    uint8_t b[3] = {(uint8_t)129};
    int b_len = 2;
  if(len < 126){
    b[1] = (uint8_t)len;
  } else {
    b[1] = (uint8_t)127;
    b[2] = (uint8_t)len;
    b_len = 3;
  }
  sock->sWriteBinary(sock, b, b_len);
  sock->sWrite(sock, msg);
  printf("EXIT websocket_sendMessage\n");
}

/**
 * caller must free
 */
char* websocket_toBase64(uint8_t* b){
  printf("ENTER websocket_toBase64 "); websocket_printBinary(b); putchar('\n');
  struct base64_encode_ctx ctx;
  base64_encode_init(&ctx);
  int len           = strlen((char*)b);
  uint8_t* result   = (uint8_t*)calloc(BASE64_ENCODE_LENGTH(len) + BASE64_ENCODE_FINAL_LENGTH + 1, sizeof(uint8_t));
  int encoded_bytes = base64_encode_update(&ctx, result, len, b);
  encoded_bytes    += base64_encode_final(&ctx, result + encoded_bytes);
  printf("EXIT websocket_toBase64 %s\n", (char*)result);
  return (char*)result;
}

/**
 * caller must free
 */
uint8_t* websocket_sha1(char* data){
  printf("ENTER websocket_sha1 %s\n", data);
  struct sha1_ctx ctx;
  uint8_t* digest = (uint8_t*)calloc(SHA1_DIGEST_SIZE, sizeof(uint8_t));
  sha1_init(&ctx);
  sha1_update(&ctx, strlen(data), (uint8_t*)data);
  sha1_digest(&ctx, SHA1_DIGEST_SIZE, digest);
  printf("EXIT websocket_sha1 "); websocket_printBinary(digest); putchar('\n');
  return digest;
}

void websocket_printBinary(uint8_t* data){
  int len = strlen((char*)data);
  for(int i = 0; i < len; i++){
    printf("%02x", data[i]);
  }
}

char* websocket_trimLeadingSpaces(char* str){
  while('\0' != str[0] && ' ' == str[0]){
    str++;
  }
  return str;
}
