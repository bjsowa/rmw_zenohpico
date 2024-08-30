#include "./message_queue.h"

#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_message_queue_init(rmw_zp_message_queue_t *message_queue, size_t capacity,
                                    rcutils_allocator_t *allocator) {
  message_queue->capacity = capacity;
  message_queue->size = message_queue->idx_front = message_queue->idx_back = 0;

  message_queue->messages =
      allocator->allocate(capacity * sizeof(z_owned_slice_t), allocator->state);

  if (message_queue->messages == NULL) {
    RMW_SET_ERROR_MSG("Failed to allocate message queue");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_queue_fini(rmw_zp_message_queue_t *message_queue,
                                    rcutils_allocator_t *allocator) {
  if (message_queue->messages != NULL) {
    allocator->deallocate(message_queue->messages, allocator->state);
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_queue_pop_front(rmw_zp_message_queue_t *message_queue,
                                         z_owned_slice_t *msg_data) {
  if (message_queue->size == 0) {
    RMW_SET_ERROR_MSG("Trying to pop messages from empty queue");
    return RMW_RET_ERROR;
  }

  z_moved_slice_t *msg_moved = z_move(message_queue->messages[message_queue->idx_front]);
  if (msg_data == NULL) {
    z_drop(msg_moved);
  } else {
    z_take(msg_data, msg_moved);
  }

  message_queue->size--;
  message_queue->idx_front++;
  if (message_queue->idx_front == message_queue->capacity) {
    message_queue->idx_front = 0;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_message_queue_push_back(rmw_zp_message_queue_t *message_queue,
                                         z_moved_slice_t *payload) {
  if (message_queue->size == message_queue->capacity) {
    RMW_SET_ERROR_MSG("Trying to push messages to a queue that is full");
    return RMW_RET_ERROR;
  }

  z_take(&message_queue->messages[message_queue->idx_back], payload);

  message_queue->size++;
  message_queue->idx_back++;
  if (message_queue->idx_back == message_queue->capacity) {
    message_queue->idx_back = 0;
  }

  return RMW_RET_OK;
}
