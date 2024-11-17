#ifndef RMW_ZENOHPICO_DETAIL__CLIENT_H_
#define RMW_ZENOHPICO_DETAIL__CLIENT_H_

#include <stdint.h>

#include "./attachment_helpers.h"
#include "./message_queue.h"
#include "./type_support.h"
#include "./wait_set.h"
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

  rmw_zp_wait_set_t* wait_set_data;
  z_owned_mutex_t condition_mutex;

  rmw_zp_message_queue_t reply_queue;
  z_owned_mutex_t reply_queue_mutex;

  // rmw_zenoh uses Zenoh queries to implement clients.  It turns out that in Zenoh, there is no
  // way to cancel a query once it is in-flight via the z_get() zenoh-c API. Thus, if an
  // rmw_zenoh_cpp user does rmw_create_client(), rmw_send_request(), rmw_destroy_client(), but the
  // query comes in after the rmw_destroy_client(), rmw_zenoh_cpp could access already-freed memory.
  //
  // The next 3 variables are used to avoid that situation.  Any time a query is initiated via
  // rmw_send_request(), num_in_flight_ is incremented.  When the Zenoh calls the callback for the
  // query reply, num_in_flight_ is decremented.  When rmw_destroy_client() is called, is_shutdown_
  // is set to true.  If num_in_flight_ is 0, the data associated with this structure is freed.
  // If num_in_flight_ is *not* 0, then the data associated with this structure is maintained.
  // In the situation where is_shutdown_ is true, and num_in_flight_ drops to 0 in the query
  // callback, the query callback will free up the structure.
  //
  // There is one case which is not handled by this, which has to do with timeouts.  The query
  // timeout is currently set to essentially infinite.  Thus, if a query is in-flight but never
  // returns, the memory in this structure will never be freed.  There isn't much we can do about
  // that at this time, but we may want to consider changing the timeout so that the memory can
  // eventually be freed up.
  z_owned_mutex_t in_flight_mutex;
  bool is_shutdown;
  size_t num_in_flight;
} rmw_zp_client_t;

rmw_ret_t rmw_zp_client_init(rmw_zp_client_t* client, const rmw_qos_profile_t* qos_profile,
                             rcutils_allocator_t* allocator);

rmw_ret_t rmw_zp_client_fini(rmw_zp_client_t* client, rcutils_allocator_t* allocator);

size_t rmw_zp_client_get_next_sequence_number(rmw_zp_client_t* client);
void rmw_zp_client_increment_queries_in_flight(rmw_zp_client_t* client);
void rmw_zp_client_decrement_queries_in_flight(rmw_zp_client_t* client, bool* queries_in_flight);

void rmw_zp_client_data_handler(z_loaned_reply_t* reply, void* data);
void rmw_zp_client_data_dropper(void* data);

rmw_ret_t rmw_zp_client_add_new_reply(rmw_zp_client_t* client, const z_loaned_bytes_t* attachment,
                                      const z_loaned_bytes_t* payload);

rmw_ret_t rmw_zp_client_pop_next_reply(rmw_zp_client_t* client, rmw_zp_message_t* reply_data);

bool rmw_zp_client_queue_has_data_and_attach_condition_if_not(rmw_zp_client_t* client,
                                                              rmw_zp_wait_set_t* wait_set);

void rmw_zp_client_notify(rmw_zp_client_t* client);

bool rmw_zp_client_detach_condition_and_queue_is_empty(rmw_zp_client_t* client);

#endif