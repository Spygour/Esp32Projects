#ifndef portIsr_h
#define portIsr_h

#include "stdint.h"
#include "stdbool.h"
#include "freertos/queue.h"
extern QueueHandle_t portIsr_door_info_queue;

typedef struct 
{
  bool current_state;
  int64_t last_time_changed; 
  bool previous_state;
}door_info_t;

extern void Port_InitIsr(void);

#endif