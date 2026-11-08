#ifndef EVENTS_H
#define EVENTS_H

#include <event2/event_struct.h>
#include <stdint.h>
#include <stdlib.h>
typedef enum {
  START = 0,
  STOP = 1,
  ACK = 2,
  NACK = 3,
  ADDRESS = 4,
  DATA = 5
} event_name_t;

typedef struct{
  event_name_t name;
  uint32_t sample_idx;
  uint8_t value;
} event_t;

typedef struct{
  event_t *events;
  uint32_t count;
  uint32_t capacity;
} events_list_t;

int push_event(events_list_t *list, event_name_t name, uint32_t sample_idx, uint8_t value){
  if(list->count == list->capacity){
    uint32_t new_capacity;
    if(list->capacity == 0){
      new_capacity = 64;
    }else{
      new_capacity = list->capacity * 2;
    }

    event_t *new_events = realloc(list->events, new_capacity * sizeof(*new_events));
    if(!new_events) return -1;
    list->events = new_events;
    list->capacity = new_capacity;
  }
  
  list->events[list->count++] = (event_t){name, sample_idx, value};
  return 0;
 }
 
void free_events(events_list_t *list){
  free(list->events);
  *list = (events_list_t){0};
}

#endif