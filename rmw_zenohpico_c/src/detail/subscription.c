#include "./subscription.h"

#include "./attachment_helpers.h"
#include "./message_queue.h"
#include "./qos.h"
#include "rmw/error_handling.h"
#include "zenoh-pico.h"

rmw_ret_t rmw_zp_subscription_init(rmw_zp_subscription_t* subscription,
                                   const rmw_qos_profile_t* qos_profile,
                                   rcutils_allocator_t* allocator) {
  subscription->adapted_qos_profile = *qos_profile;
  if (rmw_zp_adapt_qos_profile(&subscription->adapted_qos_profile) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (z_mutex_init(&subscription->message_queue_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_init(&subscription->message_queue,
                                subscription->adapted_qos_profile.depth, allocator) != RMW_RET_OK) {
    z_drop(z_move(subscription->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_subscription_fini(rmw_zp_subscription_t* subscription,
                                   rcutils_allocator_t* allocator) {
  rmw_ret_t ret = RMW_RET_OK;

  if (rmw_zp_message_queue_fini(&subscription->message_queue, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  if (z_drop(z_move(subscription->message_queue_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  return ret;
}

void rmw_zp_sub_data_handler(const z_loaned_sample_t* sample, void* data) {
  z_view_string_t keystr;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

  rmw_zp_subscription_t* sub_data = data;
  if (sub_data == NULL) {
    // TODO: report error
    return;
  }

  rmw_zp_attachment_data_t attachment_data;

  const z_loaned_bytes_t* attachment = z_sample_attachment(sample);
  if (!_z_bytes_check(attachment)) {
    // TODO: report error
    // Set fallback attachment data
    attachment_data.sequence_number = 0;
    attachment_data.source_timestamp = 0;
    memset(attachment_data.source_gid, 0, RMW_GID_STORAGE_SIZE);
  } else if (rmw_zp_attachment_data_deserialize_from_zbytes(attachment, &attachment_data) !=
             RMW_RET_OK) {
    // Deserialize method will set default values for attachment data it fails to deserialize
    // TODO: report error
  }

  const z_loaned_bytes_t* payload = z_sample_payload(sample);

  // TODO: avoid dynamic allocation
  z_owned_slice_t slice;
  z_bytes_deserialize_into_slice(payload, &slice);

  if (rmw_zp_subscription_add_new_message(sub_data, &attachment_data, z_move(slice)) !=
      RMW_RET_OK) {
    // TODO: report error
  }
}

rmw_ret_t rmw_zp_subscription_add_new_message(rmw_zp_subscription_t* subscription,
                                              rmw_zp_attachment_data_t* attachment_data,
                                              z_moved_slice_t* payload) {
  RCUTILS_UNUSED(attachment_data);

  z_mutex_lock(z_loan_mut(subscription->message_queue_mutex));

  if (subscription->message_queue.size >= subscription->adapted_qos_profile.depth) {
    // TODO: Log warning if message is discarded due to hitting the queue depth

    // Adapted QoS has depth guaranteed to be >= 1
    if (rmw_zp_message_queue_pop_front(&subscription->message_queue, NULL) != RMW_RET_OK) {
      z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
      return RMW_RET_ERROR;
    }
  }

  if (rmw_zp_message_queue_push_back(&subscription->message_queue, payload) != RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_subscription_pop_next_message(rmw_zp_subscription_t* subscription,
                                               z_owned_slice_t* msg_data) {
  z_mutex_lock(z_loan_mut(subscription->message_queue_mutex));

  if (subscription->message_queue.size == 0) {
    RMW_SET_ERROR_MSG("Trying to pop message from empty message queue");
    z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  if (rmw_zp_message_queue_pop_front(&subscription->message_queue, msg_data) != RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));

  return RMW_RET_OK;
}