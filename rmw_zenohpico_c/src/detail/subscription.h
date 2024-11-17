#ifndef RMW_ZENOHPICO_DETAIL__SUBSCRIPTION_H_
#define RMW_ZENOHPICO_DETAIL__SUBSCRIPTION_H_

#include <stdint.h>

#include "./attachment_helpers.h"
#include "./message_queue.h"
#include "./type_support.h"
#include "./wait_set.h"
#include "rmw/ret_types.h"
#include "rmw/types.h"
#include "zenoh-pico.h"

typedef struct {
  // An owned subscription.
  z_owned_subscriber_t sub;

  // Store the actual QoS profile used to configure this subscription.
  rmw_qos_profile_t adapted_qos_profile;

  // Type support
  rmw_zp_message_type_support_t* type_support;

  // Context for memory allocation for messages.
  rmw_context_t* context;

  rmw_zp_message_queue_t message_queue;
  z_owned_mutex_t message_queue_mutex;

  rmw_zp_wait_set_t* wait_set_data;
  z_owned_mutex_t condition_mutex;
} rmw_zp_subscription_t;

rmw_ret_t rmw_zp_subscription_init(rmw_zp_subscription_t* subscription,
                                   const rmw_qos_profile_t* qos_profile,
                                   rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_subscription_fini(rmw_zp_subscription_t* subscription,
                                   rcutils_allocator_t* allocator);

void rmw_zp_sub_data_handler(z_loaned_sample_t* sample, void* data);

rmw_ret_t rmw_zp_subscription_add_new_message(rmw_zp_subscription_t* subscription,
                                              const z_loaned_bytes_t* attachment,
                                              const z_loaned_bytes_t* payload);

rmw_ret_t rmw_zp_subscription_pop_next_message(rmw_zp_subscription_t* subscription,
                                               rmw_zp_message_t* msg_data);

bool rmw_zp_subscription_queue_has_data_and_attach_condition_if_not(
    rmw_zp_subscription_t* subscription, rmw_zp_wait_set_t* wait_set);

void rmw_zp_subscription_notify(rmw_zp_subscription_t* subscription);

bool rmw_zp_subscription_detach_condition_and_queue_is_empty(rmw_zp_subscription_t* subscription);

#endif