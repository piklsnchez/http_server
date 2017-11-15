#include "hash_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const int HASH_MAP_BUFFER_LEN = 1024;

struct hash_map* hash_map_new(){
  struct hash_map* this = (struct hash_map*)malloc(sizeof(struct hash_map));
  this->_buffer         = (char*)calloc(HASH_MAP_BUFFER_LEN, sizeof(char));
  this->delete          = hash_map_delete;
  this->put             = hash_map_put;
  this->get             = hash_map_get;
  this->toString        = hash_map_toString;
  this->_node           = NULL;
  return this;
}

void hash_map_delete(struct hash_map* this){
  struct hash_map_node* current = this->_node;
  struct hash_map_node* deleteMe;
  while(current != NULL){
    free(current->key);
    free(current->value);
    deleteMe = current;
    current  = current->next;
    free(deleteMe);
  }
  free(this->_buffer);
  free(this);
}

/**
 * copy key and value into map
 */
struct hash_map* hash_map_put(struct hash_map* this, char* key, char* value){
  struct hash_map_node* new_head = (struct hash_map_node*)malloc(sizeof(struct hash_map_node));
  struct hash_map_node* head     = this->_node;
  new_head->key                  = strcpy(calloc(strlen(key) + 1, sizeof(char)), key);
  new_head->value                = strcpy(calloc(strlen(value) + 1, sizeof(char)), value);
  new_head->next                 = head;
  this->_node                    = new_head;
  return this;
}

char* hash_map_get(struct hash_map* this, char* key){
  struct hash_map_node* current = this->_node;
  while(current != NULL){
    if(current->key != NULL){
      if(0 == strcmp(current->key, key)){
        break;
      }
    }
    current = current->next;
  }
  return current == NULL ? NULL : current->value;
}

/**
 * not thread-safe
 */
char* hash_map_toString(struct hash_map* this){
  struct hash_map_node* current = this->_node;
  int len = 0;
  while(current != NULL && len < HASH_MAP_BUFFER_LEN){
    strncat(this->_buffer, current->key, HASH_MAP_BUFFER_LEN - len);
    len = strlen(this->_buffer);
    if(HASH_MAP_BUFFER_LEN - len <= 0) break;
    this->_buffer[len] = '|';
    len++;
    if(HASH_MAP_BUFFER_LEN - len <= 0) break;
    strncat(this->_buffer, current->value, HASH_MAP_BUFFER_LEN - len); 
    len = strlen(this->_buffer);
    if(HASH_MAP_BUFFER_LEN - len <= 0) break;
    this->_buffer[len] = '\n';
    len++;
    current = current->next;
  }
  this->_buffer[len] = '\0';
  return this->_buffer;
}
