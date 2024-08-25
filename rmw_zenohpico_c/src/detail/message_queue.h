#ifndef RMW_ZENOHPICO_DETAIL__MESSAGE_QUEUE_H_
#define RMW_ZENOHPICO_DETAIL__MESSAGE_QUEUE_H_

#include "rcutils/allocator.h"
#include "rmw/ret_types.h"
#include "zenoh-pico.h"

typedef struct rmw_zenohpico_message_queue_s {
  z_moved_slice_t *messages;
  size_t capacity;
  size_t size;
  size_t idx;
} rmw_zenohpico_message_queue_t;

rmw_ret_t rmw_zenohpico_message_queue_init(rmw_zenohpico_message_queue_t *message_queue,
                                           size_t capacity, rcutils_allocator_t *allocator);

rmw_ret_t rmw_zenohpico_message_queue_fini(rmw_zenohpico_message_queue_t *message_queue,
                                           rcutils_allocator_t *allocator);

#endif