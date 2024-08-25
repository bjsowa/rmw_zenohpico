#include "./message_queue.h"

#include "rmw/error_handling.h"

rmw_ret_t rmw_zenohpico_message_queue_init(rmw_zenohpico_message_queue_t *message_queue,
                                           size_t capacity, rcutils_allocator_t *allocator) {
  message_queue->capacity = capacity;
  message_queue->size = message_queue->idx = 0;

  message_queue->messages =
      allocator->allocate(capacity * sizeof(z_moved_slice_t), allocator->state);

  if (message_queue->messages == NULL) {
    RMW_SET_ERROR_MSG("Failed to allocate message queue");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zenohpico_message_queue_fini(rmw_zenohpico_message_queue_t *message_queue,
                                           rcutils_allocator_t *allocator) {
  if (message_queue->messages != NULL) {
    allocator->deallocate(message_queue->messages, allocator->state);
  }

  return RMW_RET_OK;
}