#include "queue.h"
#include <stdio.h>
#include <string.h>

struct queue* queue_new(){
  struct this* this = (struct queue*)malloc(sizeof(struct queue*));
  return this;
}
void queue_enqueue(struct queue* this, void* func){
  
  func;
}

