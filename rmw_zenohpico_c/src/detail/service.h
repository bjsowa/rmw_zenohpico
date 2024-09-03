#ifndef RMW_ZENOHPICO_DETAIL__SERVICE_H_
#define RMW_ZENOHPICO_DETAIL__SERVICE_H_

#include "./message_queue.h"
#include "./type_support.h"
#include "rmw/rmw.h"
#include "zenoh-pico.h"

typedef struct {
  const char* keyexpr_c_str;
  z_view_keyexpr_t keyexpr;

  z_owned_queryable_t qable;

  // Store the actual QoS profile used to configure this service.
  // The QoS is reused for getting requests and sending responses.
  rmw_qos_profile_t adapted_qos_profile;

  // Type support
  rmw_zp_service_type_support_t* type_support;

  rmw_context_t* context;

  rmw_zp_message_queue_t query_queue;
  z_owned_mutex_t query_queue_mutex;

  // ? sequence_to_query_map;
  z_owned_mutex_t sequence_to_query_map_mutex;

} rmw_zp_service_t;

rmw_ret_t rmw_zp_service_init(rmw_zp_service_t* service, const rmw_qos_profile_t* qos_profile,
                              rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_service_fini(rmw_zp_service_t* service, rcutils_allocator_t* allocator);

void rmw_zp_service_data_handler(const z_loaned_query_t* query, void* service_data);

#endif