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
    goto fail_init_message_queue;
  }

  if (z_mutex_init(&subscription->condition_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    goto fail_init_condition_mutex;
  }

  return RMW_RET_OK;

fail_init_condition_mutex:
  rmw_zp_message_queue_fini(&subscription->message_queue, allocator);
fail_init_message_queue:
  z_drop(z_move(subscription->message_queue_mutex));
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_zp_subscription_fini(rmw_zp_subscription_t* subscription,
                                   rcutils_allocator_t* allocator) {
  rmw_ret_t ret = RMW_RET_OK;

  if (z_drop(z_move(subscription->condition_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

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
  rmw_zp_subscription_t* sub_data = data;
  if (sub_data == NULL) {
    // TODO(bjsowa): report error
    return;
  }

  const z_loaned_bytes_t* attachment = z_sample_attachment(sample);
  if (!_z_bytes_check(attachment)) {
    // TODO(bjsowa): report error
    return;
  }

  const z_loaned_bytes_t* payload = z_sample_payload(sample);

  if (rmw_zp_subscription_add_new_message(sub_data, attachment, payload) != RMW_RET_OK) {
    // TODO(bjsowa): report error
  }
}

rmw_ret_t rmw_zp_subscription_add_new_message(rmw_zp_subscription_t* subscription,
                                              const z_loaned_bytes_t* attachment,
                                              const z_loaned_bytes_t* payload) {
  z_mutex_lock(z_loan_mut(subscription->message_queue_mutex));

  if (subscription->message_queue.size >= subscription->adapted_qos_profile.depth) {
    // TODO(bjsowa): Log warning if message is discarded due to hitting the queue depth

    // Adapted QoS has depth guaranteed to be >= 1
    if (rmw_zp_message_queue_pop_front(&subscription->message_queue, NULL) != RMW_RET_OK) {
      z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
      return RMW_RET_ERROR;
    }
  }

  if (rmw_zp_message_queue_push_back(&subscription->message_queue, attachment, payload, NULL) !=
      RMW_RET_OK) {
    z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));
    return RMW_RET_ERROR;
  }

  z_mutex_unlock(z_loan_mut(subscription->message_queue_mutex));

  rmw_zp_subscription_notify(subscription);

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_subscription_pop_next_message(rmw_zp_subscription_t* subscription,
                                               rmw_zp_message_t* msg_data) {
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

bool rmw_zp_subscription_queue_has_data_and_attach_condition_if_not(
    rmw_zp_subscription_t* subscription, rmw_zp_wait_set_t* wait_set) {
  z_mutex_lock(z_loan_mut(subscription->condition_mutex));

  if (subscription->message_queue.size > 0) {
    z_mutex_unlock(z_loan_mut(subscription->condition_mutex));
    return true;
  }

  subscription->wait_set_data = wait_set;

  z_mutex_unlock(z_loan_mut(subscription->condition_mutex));

  return false;
}

void rmw_zp_subscription_notify(rmw_zp_subscription_t* subscription) {
  z_mutex_lock(z_loan_mut(subscription->condition_mutex));

  if (subscription->wait_set_data != NULL) {
    z_mutex_lock(z_loan_mut(subscription->wait_set_data->condition_mutex));
    subscription->wait_set_data->triggered = true;
    z_condvar_signal(z_loan_mut(subscription->wait_set_data->condition_variable));
    z_mutex_unlock(z_loan_mut(subscription->wait_set_data->condition_mutex));
  }

  z_mutex_unlock(z_loan_mut(subscription->condition_mutex));
}

bool rmw_zp_subscription_detach_condition_and_queue_is_empty(rmw_zp_subscription_t* subscription) {
  z_mutex_lock(z_loan_mut(subscription->condition_mutex));

  subscription->wait_set_data = NULL;

  z_mutex_unlock(z_loan_mut(subscription->condition_mutex));

  return subscription->message_queue.size == 0;
}