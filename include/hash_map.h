#ifndef __HASH_MAP_H
#define __HASH_MAP_H

struct hash_map_node {
  char* key;
  char* value;
  struct hash_map_node* next;
};

struct hash_map {
  struct hash_map_node* _node;
  char*            _buffer;
  struct hash_map* (*put)     (struct hash_map* this, char* key, char* value);
  char*            (*get)     (struct hash_map* this, char* key);
  char*            (*toString)(struct hash_map* this);
  void             (*delete)  (struct hash_map* this);
};

struct hash_map* hash_map_new();
void             hash_map_delete  (struct hash_map* this);
struct hash_map* hash_map_put     (struct hash_map* this, char* key, char* value);
char*            hash_map_get     (struct hash_map* this, char* data);
char*            hash_map_toString(struct hash_map* this);

#endif
