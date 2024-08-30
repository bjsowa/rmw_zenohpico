#ifndef RMW_ZENOHPICO_DETAIL__SUBSCRIPTION_H_
#define RMW_ZENOHPICO_DETAIL__SUBSCRIPTION_H_

#include <stdint.h>

#include "./attachment_helpers.h"
#include "./message_queue.h"
#include "./type_support.h"
#include "rmw/ret_types.h"
#include "rmw/types.h"
#include "zenoh-pico.h"

typedef struct {
  // An owned subscription.
  z_owned_subscriber_t sub;

  // Store the actual QoS profile used to configure this subscription.
  rmw_qos_profile_t adapted_qos_profile;

  // Type support fields
  const void* type_support_impl;
  const char* typesupport_identifier;
  const rosidl_type_hash_t* type_hash;
  rmw_zp_type_support_t* type_support;

  // Context for memory allocation for messages.
  rmw_context_t* context;

  rmw_zp_message_queue_t message_queue;
  z_owned_mutex_t message_queue_mutex;
} rmw_zp_subscription_t;

rmw_ret_t rmw_zp_subscription_init(rmw_zp_subscription_t* subscription,
                                   const rmw_qos_profile_t* qos_profile,
                                   rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_subscription_fini(rmw_zp_subscription_t* subscription,
                                   rcutils_allocator_t* allocator);

void rmw_zp_sub_data_handler(const z_loaned_sample_t* sample, void* data);

rmw_ret_t rmw_zp_subscription_add_new_message(rmw_zp_subscription_t* subscription,
                                              rmw_zp_attachment_data_t* attachment_data,
                                              z_moved_slice_t* payload);

rmw_ret_t rmw_zp_subscription_pop_next_message(rmw_zp_subscription_t* subscription,
                                               z_owned_slice_t* msg_data);

#endif