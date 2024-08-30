#ifndef RMW_ZENOHPICO_DETAIL__CLIENT_H_
#define RMW_ZENOHPICO_DETAIL__CLIENT_H_

#include <stdint.h>

#include "./type_support.h"
#include "rmw/rmw.h"
#include "zenoh-pico.h"

typedef struct {
  const char* keyexpr_c_str;
  z_view_keyexpr_t keyexpr;

  // Store the actual QoS profile used to configure this client.
  // The QoS is reused for sending requests and getting responses.
  rmw_qos_profile_t adapted_qos_profile;

  // Type support
  rmw_zp_service_type_support_t* type_support;

  rmw_context_t* context;

  uint8_t client_gid[RMW_GID_STORAGE_SIZE];

  z_owned_mutex_t sequence_number_mutex;
  size_t sequence_number;
} rmw_zp_client_t;

rmw_ret_t rmw_zp_client_init(rmw_zp_client_t* client, const rmw_qos_profile_t* qos_profile);

rmw_ret_t rmw_zp_client_fini(rmw_zp_client_t* client);

#endif