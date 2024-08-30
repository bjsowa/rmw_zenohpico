#include <inttypes.h>

#include "detail/identifiers.h"
#include "detail/rmw_data_types.h"
#include "detail/subscription.h"
#include "rcutils/macros.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "zenoh-pico.h"

static rmw_ret_t take_one(rmw_zp_subscription_t* sub_data, void* ros_message, bool* taken,
                          rmw_message_info_t* message_info) {
  z_owned_slice_t msg_data;
  if (rmw_zp_subscription_pop_next_message(sub_data, &msg_data) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  const uint8_t* payload = z_slice_data(z_loan(msg_data));
  const size_t payload_len = z_slice_len(z_loan(msg_data));

  if (rmw_zp_type_support_deserialize_ros_message(sub_data->type_support, payload, payload_len,
                                                  ros_message) != RMW_RET_OK) {
    z_drop(z_move(msg_data));
    return RMW_RET_ERROR;
  }

  z_drop(z_move(msg_data));

  if (message_info != NULL) {
    message_info->reception_sequence_number = 0;
    message_info->publisher_gid.implementation_identifier = rmw_zp_identifier;
    message_info->from_intra_process = false;

    // TODO: Fill the rest of message info
    // message_info->source_timestamp = msg_data->source_timestamp;
    // message_info->received_timestamp = msg_data->recv_timestamp;
    // message_info->publication_sequence_number = msg_data->sequence_number;
    // memcpy(message_info->publisher_gid.data, msg_data->publisher_gid,
    //          RMW_GID_STORAGE_SIZE);
  }

  *taken = true;

  return RMW_RET_OK;
}

static rmw_ret_t take_one_serialized(rmw_zp_subscription_t* sub_data,
                                     rmw_serialized_message_t* serialized_message, bool* taken,
                                     rmw_message_info_t* message_info) {
  z_owned_slice_t msg_data;
  if (rmw_zp_subscription_pop_next_message(sub_data, &msg_data) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  const uint8_t* payload = z_slice_data(z_loan(msg_data));
  const size_t payload_len = z_slice_len(z_loan(msg_data));

  if (serialized_message->buffer_capacity < payload_len) {
    rmw_ret_t ret = rmw_serialized_message_resize(serialized_message, payload_len);
    if (ret != RMW_RET_OK) {
      return ret;  // Error message already set
    }
  }
  serialized_message->buffer_length = payload_len;
  memcpy(serialized_message->buffer, payload, payload_len);

  *taken = true;

  if (message_info != NULL) {
    message_info->reception_sequence_number = 0;
    message_info->publisher_gid.implementation_identifier = rmw_zp_identifier;
    message_info->from_intra_process = false;

    // TODO: Fill the rest of message info
    // message_info->source_timestamp = msg_data->source_timestamp;
    // message_info->received_timestamp = msg_data->recv_timestamp;
    // message_info->publication_sequence_number = msg_data->sequence_number;
    // memcpy(message_info->publisher_gid.data, msg_data->publisher_gid,
    //          RMW_GID_STORAGE_SIZE);
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_take(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                   rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->topic_name, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->data, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  *taken = false;

  rmw_zp_subscription_t* sub_data = subscription->data;

  return take_one(sub_data, ros_message, taken, NULL);
}

rmw_ret_t rmw_take_with_info(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                             rmw_message_info_t* message_info,
                             rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->topic_name, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->data, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  *taken = false;

  rmw_zp_subscription_t* sub_data = subscription->data;

  return take_one(sub_data, ros_message, taken, message_info);
}

rmw_ret_t rmw_take_sequence(const rmw_subscription_t* subscription, size_t count,
                            rmw_message_sequence_t* message_sequence,
                            rmw_message_info_sequence_t* message_info_sequence, size_t* taken,
                            rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->topic_name, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->data, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_sequence, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_info_sequence, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  if (0u == count) {
    RMW_SET_ERROR_MSG("count cannot be 0");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (count > message_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insuffient capacity in message_sequence");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (count > message_info_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insuffient capacity in message_info_sequence");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (count > UINT32_MAX) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Cannot take %zu samples at once, limit is %" PRIu32,
                                         count, UINT32_MAX);
    return RMW_RET_ERROR;
  }

  *taken = 0;

  rmw_zp_subscription_t* sub_data = subscription->data;

  if (sub_data->context->impl->is_shutdown) {
    return RMW_RET_OK;
  }

  rmw_ret_t ret;

  while (*taken < count) {
    bool one_taken = false;

    ret = take_one(sub_data, message_sequence->data[*taken], &one_taken,
                   &message_info_sequence->data[*taken]);
    if (ret != RMW_RET_OK) {
      // If we are taking a sequence and the 2nd take in the sequence failed,
      // we'll report RMW_RET_ERROR to the caller, but we will *also* tell the
      // caller that there are valid messages already taken (via the
      // message_sequence size).  It is up to the caller to deal with that
      // situation appropriately.
      break;
    }

    if (!one_taken) {
      // No error, but there was nothing left to be taken, so break out of the
      // loop
      break;
    }

    (*taken)++;
  }

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  return ret;
}

rmw_ret_t rmw_take_serialized_message(const rmw_subscription_t* subscription,
                                      rmw_serialized_message_t* serialized_message, bool* taken,
                                      rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->topic_name, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->data, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(serialized_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zp_subscription_t* sub_data = subscription->data;

  return take_one_serialized(sub_data, serialized_message, taken, NULL);
}

rmw_ret_t rmw_take_serialized_message_with_info(const rmw_subscription_t* subscription,
                                                rmw_serialized_message_t* serialized_message,
                                                bool* taken, rmw_message_info_t* message_info,
                                                rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->topic_name, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription->data, RMW_RET_ERROR);
  RMW_CHECK_ARGUMENT_FOR_NULL(serialized_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zp_subscription_t* sub_data = subscription->data;

  return take_one_serialized(sub_data, serialized_message, taken, message_info);
}

rmw_ret_t rmw_take_loaned_message(const rmw_subscription_t* subscription, void** loaned_message,
                                  bool* taken, rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message_with_info(const rmw_subscription_t* subscription,
                                            void** loaned_message, bool* taken,
                                            rmw_message_info_t* message_info,
                                            rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_subscription(const rmw_subscription_t* subscription,
                                                      void* loaned_message) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  return RMW_RET_UNSUPPORTED;
}
