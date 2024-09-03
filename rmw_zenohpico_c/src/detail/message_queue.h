#ifndef RMW_ZENOHPICO_DETAIL__MESSAGE_QUEUE_H_
#define RMW_ZENOHPICO_DETAIL__MESSAGE_QUEUE_H_

#include "./attachment_helpers.h"
#include "rcutils/allocator.h"
#include "rmw/ret_types.h"
#include "zenoh-pico.h"

typedef struct {
  rmw_zp_attachment_data_t attachment_data;
  z_owned_slice_t payload;
} rmw_zp_message_t;

typedef struct {
  rmw_zp_message_t *messages;
  size_t capacity;
  size_t size;
  size_t idx_front;
  size_t idx_back;
} rmw_zp_message_queue_t;

rmw_ret_t rmw_zp_message_queue_init(rmw_zp_message_queue_t *message_queue, size_t capacity,
                                    rcutils_allocator_t *allocator);

rmw_ret_t rmw_zp_message_queue_fini(rmw_zp_message_queue_t *message_queue,
                                    rcutils_allocator_t *allocator);

rmw_ret_t rmw_zp_message_queue_pop_front(rmw_zp_message_queue_t *message_queue,
                                         rmw_zp_message_t *msg_data);

rmw_ret_t rmw_zp_message_queue_push_back(rmw_zp_message_queue_t *message_queue,
                                         const z_loaned_bytes_t *attachment,
                                         const z_loaned_bytes_t *payload);

#endif