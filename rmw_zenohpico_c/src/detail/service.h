#ifndef RMW_ZENOHPICO_DETAIL__SERVICE_H_
#define RMW_ZENOHPICO_DETAIL__SERVICE_H_

#include "./message_queue.h"
#include "./query_map.h"
#include "./type_support.h"
#include "./wait_set.h"
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

  rmw_zp_message_queue_t message_queue;
  rmw_zp_query_map_t query_map;
  z_owned_mutex_t message_queue_mutex;

  rmw_zp_wait_set_t* wait_set_data;
  z_owned_mutex_t condition_mutex;
} rmw_zp_service_t;

rmw_ret_t rmw_zp_service_init(rmw_zp_service_t* service, const rmw_qos_profile_t* qos_profile,
                              rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_service_fini(rmw_zp_service_t* service, rcutils_allocator_t* allocator);

void rmw_zp_service_data_handler(const z_loaned_query_t* query, void* data);

rmw_ret_t rmw_zp_service_add_new_query(rmw_zp_service_t* service,
                                       const z_loaned_bytes_t* attachment,
                                       const z_loaned_bytes_t* payload,
                                       const z_loaned_query_t* query);

rmw_ret_t rmw_zp_service_pop_next_query(rmw_zp_service_t* service, rmw_zp_message_t* query_data);

rmw_ret_t rmw_zp_service_take_from_query_map(rmw_zp_service_t* service,
                                             const rmw_request_id_t* request_header,
                                             const z_loaned_query_t** query);

bool rmw_zp_service_queue_has_data_and_attach_condition_if_not(rmw_zp_service_t* service,
                                                               rmw_zp_wait_set_t* wait_set);

void rmw_zp_service_notify(rmw_zp_service_t* service);

bool rmw_zp_service_detach_condition_and_queue_is_empty(rmw_zp_service_t* service);

#endif