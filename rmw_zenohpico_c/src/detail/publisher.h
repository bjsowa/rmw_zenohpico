#ifndef RMW_ZENOHPICO_DETAIL__PUBLISHER_H_
#define RMW_ZENOHPICO_DETAIL__PUBLISHER_H_

#include "./type_support.h"
#include "rmw/init.h"
#include "rmw/ret_types.h"
#include "rmw/types.h"
#include "rosidl_runtime_c/type_hash.h"
#include "rosidl_typesupport_microxrcedds_c/message_type_support.h"
#include "zenoh-pico.h"

typedef struct rmw_zenohpico_publisher_s {
  // An owned publisher.
  z_owned_publisher_t pub;

  // Store the actual QoS profile used to configure this publisher.
  rmw_qos_profile_t adapted_qos_profile;

  // Type support fields
  const void* type_support_impl;
  const char* typesupport_identifier;
  const rosidl_type_hash_t* type_hash;
  rmw_zenohpico_type_support_t* type_support;

  // Context for memory allocation for messages.
  rmw_context_t* context;

  uint8_t pub_gid[RMW_GID_STORAGE_SIZE];

  z_owned_mutex_t sequence_number_mutex;
  size_t sequence_number;
} rmw_zenohpico_publisher_t;

rmw_ret_t rmw_zenohpico_publisher_init(rmw_zenohpico_publisher_t* publisher,
                                       const rmw_qos_profile_t* qos_profile);

rmw_ret_t rmw_zenohpico_publisher_fini(rmw_zenohpico_publisher_t* publisher);

size_t rmw_zenohpico_publisher_get_next_sequence_number(rmw_zenohpico_publisher_t* publisher);

#endif