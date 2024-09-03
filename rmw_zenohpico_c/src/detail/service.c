#include "./service.h"

#include "./attachment_helpers.h"
#include "./qos.h"
#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_service_init(rmw_zp_service_t* service, const rmw_qos_profile_t* qos_profile,
                              rcutils_allocator_t* allocator) {
  service->adapted_qos_profile = *qos_profile;

  if (rmw_zp_adapt_qos_profile(&service->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_init(&service->message_queue, service->adapted_qos_profile.depth,
                                allocator) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (rmw_zp_query_map_init(&service->query_map, service->adapted_qos_profile.depth, allocator) !=
      RMW_RET_OK) {
    goto fail_init_query_map;
  }

  if (z_mutex_init(&service->message_queue_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    goto fail_init_message_queue_mutex;
  }

  return RMW_RET_OK;

  z_drop(z_move(service->message_queue_mutex));
fail_init_message_queue_mutex:
  rmw_zp_query_map_fini(&service->query_map, allocator);
fail_init_query_map:
  rmw_zp_message_queue_fini(&service->message_queue, allocator);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_service_fini(rmw_zp_service_t* service, rcutils_allocator_t* allocator) {
  rmw_ret_t ret = RMW_RET_OK;

  if (z_drop(z_move(service->message_queue_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  if (rmw_zp_query_map_fini(&service->query_map, allocator) < 0) {
    ret = RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_fini(&service->message_queue, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  return ret;
}

void rmw_zp_service_data_handler(const z_loaned_query_t* query, void* data) {
  rmw_zp_service_t* service_data = data;
  if (service_data == NULL) {
    // TODO: report error
    return;
  }

  const z_loaned_bytes_t* attachment = z_query_attachment(query);
  if (!_z_bytes_check(attachment)) {
    // TODO: report error
    return;
  }

  const z_loaned_bytes_t* payload = z_query_payload(query);

  if (rmw_zp_service_add_new_query(service_data, attachment, payload, query) != RMW_RET_OK) {
    // TODO: report error
  }
}

rmw_ret_t rmw_zp_service_add_new_query(rmw_zp_service_t* service,
                                       const z_loaned_bytes_t* attachment,
                                       const z_loaned_bytes_t* payload,
                                       const z_loaned_query_t* query) {
  z_mutex_lock(z_loan_mut(service->message_queue_mutex));

  if (service->message_queue.size >= service->adapted_qos_profile.depth) {
    // TODO: Log warning if message is discarded due to hitting the queue depth

    // Adapted QoS has depth guaranteed to be >= 1
    if (rmw_zp_message_queue_pop_front(&service->message_queue, NULL) != RMW_RET_OK) {
      z_mutex_unlock(z_loan_mut(service->message_queue_mutex));
      return RMW_RET_ERROR;
    }
  }

  const rmw_zp_message_t* message;

  if (rmw_zp_message_queue_push_back(&service->message_queue, attachment, payload, &message) !=
      RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(service->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  if (rmw_zp_query_map_insert(&service->query_map, query, message->attachment_data.sequence_number,
                              message->attachment_data.source_gid) < 0) {
    // TODO(bjsowa): pop the message pushed to message queue
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(service->message_queue_mutex));

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_service_pop_next_query(rmw_zp_service_t* service, rmw_zp_message_t* query_data) {
  z_mutex_lock(z_loan_mut(service->message_queue_mutex));

  if (service->message_queue.size == 0) {
    RMW_SET_ERROR_MSG("Trying to pop message from empty message queue");
    z_mutex_unlock(z_loan_mut(service->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_pop_front(&service->message_queue, query_data) != RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(service->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(service->message_queue_mutex));

  return RMW_RET_OK;
}