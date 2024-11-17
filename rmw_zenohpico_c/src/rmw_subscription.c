#include <inttypes.h>

#include "detail/identifiers.h"
#include "detail/node.h"
#include "detail/rmw_data_types.h"
#include "detail/ros_topic_name_to_zenoh_key.h"
#include "detail/subscription.h"
#include "detail/type_support.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_init_subscription_allocation(const rosidl_message_type_support_t* type_support,
                                           const rosidl_runtime_c__Sequence__bound* message_bounds,
                                           rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_fini_subscription_allocation(rmw_subscription_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t* rmw_create_subscription(
    const rmw_node_t* node, const rosidl_message_type_support_t* type_supports,
    const char* topic_name, const rmw_qos_profile_t* qos_profile,
    const rmw_subscription_options_t* subscription_options) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return NULL);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, NULL);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, NULL);
  if (topic_name[0] == '\0') {
    RMW_SET_ERROR_MSG("topic_name argument is an empty string");
    return NULL;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_profile, NULL);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_options, NULL);

  RMW_CHECK_ARGUMENT_FOR_NULL(node->data, NULL);
  rmw_zp_node_t* node_data = node->data;
  RMW_CHECK_FOR_NULL_WITH_MSG(node_data, "unable to create subscription as node_data is invalid.",
                              return NULL);
  // TODO: Check if a duplicate entry for the same topic name + topic
  // type is present in node_data->subscriptions and if so return error;
  RMW_CHECK_FOR_NULL_WITH_MSG(node->context, "expected initialized context", return NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->context, "expected initialized context", return NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->context->impl, "expected initialized context impl",
                              return NULL);
  rmw_context_impl_t* context_impl = node->context->impl;
  RMW_CHECK_FOR_NULL_WITH_MSG(context_impl, "unable to get rmw_context_impl_t", return NULL);
  // RMW_CHECK_FOR_NULL_WITH_MSG(context_impl->enclave, "expected initialized enclave",
  // return NULL);

  rcutils_allocator_t* allocator = &node->context->options.allocator;

  // Create the rmw_subscription.
  rmw_subscription_t* rmw_subscription =
      allocator->zero_allocate(1, sizeof(rmw_subscription_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_subscription, "failed to allocate memory for the subscription",
                              return NULL);

  rmw_zp_subscription_t* sub_data =
      allocator->zero_allocate(1, sizeof(rmw_zp_subscription_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(sub_data, "failed to allocate memory for subscription data",
                              goto fail_allocate_subscription_data);

  if (rmw_zp_subscription_init(sub_data, qos_profile, allocator) != RMW_RET_OK) {
    goto fail_init_subscription_data;
  }

  sub_data->type_support =
      allocator->zero_allocate(1, sizeof(rmw_zp_message_type_support_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(sub_data->type_support, "Failed to allocate zenohpico type support",
                              goto fail_allocate_type_support);

  if (rmw_zp_message_type_support_init(sub_data->type_support, type_supports, allocator) !=
      RMW_RET_OK) {
    goto fail_init_type_support;
  }

  sub_data->context = node->context;

  rmw_subscription->data = sub_data;
  rmw_subscription->implementation_identifier = rmw_zp_identifier;

  rmw_subscription->topic_name = rcutils_strdup(topic_name, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_subscription->topic_name, "Failed to allocate topic name",
                              goto fail_allocate_topic_name);

  rmw_subscription->options = *subscription_options;
  rmw_subscription->can_loan_messages = false;
  rmw_subscription->is_cft_enabled = false;

  // Convert the type hash to a string so that it can be included in the keyexpr.
  char* type_hash_c_str = NULL;
  rcutils_ret_t stringify_ret =
      rosidl_stringify_type_hash(sub_data->type_support->type_hash, *allocator, &type_hash_c_str);
  if (RCUTILS_RET_BAD_ALLOC == stringify_ret) {
    RMW_SET_ERROR_MSG("Failed to allocate type_hash_c_str.");
    goto fail_allocate_type_hash_c_str;
  }

  const char* keyexpr_c_str =
      ros_topic_name_to_zenoh_key(node->context->actual_domain_id, topic_name,
                                  sub_data->type_support->type_name, type_hash_c_str, allocator);
  if (keyexpr_c_str == NULL) {
    goto fail_create_zenoh_key;
  }

  z_view_keyexpr_t keyexpr;
  z_view_keyexpr_from_str(&keyexpr, keyexpr_c_str);

  // Everything above succeeded and is setup properly.  Now declare a subscriber
  // with Zenoh; after this, callbacks may come in at any time.
  z_owned_closure_sample_t callback;
  z_closure(&callback, rmw_zp_sub_data_handler, NULL, sub_data);

  // TODO(bjsowa): support transient local qos via querying subscriber
  z_subscriber_options_t sub_options;
  z_subscriber_options_default(&sub_options);

  if (z_declare_subscriber(z_loan(context_impl->session), &sub_data->sub, z_loan(keyexpr),
                           z_move(callback), &sub_options)) {
    RMW_SET_ERROR_MSG("unable to create zenoh subscription");
    goto fail_create_zenoh_subscription;
  }

  allocator->deallocate((char*)keyexpr_c_str, allocator->state);
  allocator->deallocate(type_hash_c_str, allocator->state);

  return rmw_subscription;

  z_undeclare_subscriber(z_move(sub_data->sub));
fail_create_zenoh_subscription:
  allocator->deallocate((char*)keyexpr_c_str, allocator->state);
fail_create_zenoh_key:
  allocator->deallocate(type_hash_c_str, allocator->state);
fail_allocate_type_hash_c_str:
  allocator->deallocate((char*)rmw_subscription->topic_name, allocator->state);
fail_allocate_topic_name:
  rmw_zp_message_type_support_fini(sub_data->type_support, allocator->state);
fail_init_type_support:
  allocator->deallocate(sub_data->type_support, allocator->state);
fail_allocate_type_support:
  rmw_zp_subscription_fini(sub_data, allocator);
fail_init_subscription_data:
  allocator->deallocate(sub_data, allocator->state);
fail_allocate_subscription_data:
  allocator->deallocate(rmw_subscription, allocator->state);
  return NULL;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t* node, rmw_subscription_t* subscription) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_ret_t ret = RMW_RET_OK;

  rcutils_allocator_t* allocator = &node->context->options.allocator;
  rmw_zp_subscription_t* sub_data = subscription->data;

  if (z_undeclare_subscriber(z_move(sub_data->sub)) < 0) {
    RMW_SET_ERROR_MSG("failed to undeclare sub");
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate((char*)subscription->topic_name, allocator->state);

  if (rmw_zp_message_type_support_fini(sub_data->type_support, allocator->state) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(sub_data->type_support, allocator->state);

  if (rmw_zp_subscription_fini(sub_data, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(sub_data, allocator->state);
  allocator->deallocate(subscription, allocator->state);

  return ret;
}

rmw_ret_t rmw_subscription_count_matched_publishers(const rmw_subscription_t* subscription,
                                                    size_t* publisher_count) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(publisher_count);
  // TODO(bjsowa): implement once graph cache is working
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_subscription_get_actual_qos(const rmw_subscription_t* subscription,
                                          rmw_qos_profile_t* qos) {
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription, subscription->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_subscription_t* sub_data = subscription->data;
  RMW_CHECK_ARGUMENT_FOR_NULL(sub_data, RMW_RET_INVALID_ARGUMENT);

  *qos = sub_data->adapted_qos_profile;
  return RMW_RET_OK;
}

rmw_ret_t rmw_subscription_set_content_filter(
    rmw_subscription_t* subscription, const rmw_subscription_content_filter_options_t* options) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(options);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_get_content_filter(const rmw_subscription_t* subscription,
                                              rcutils_allocator_t* allocator,
                                              rmw_subscription_content_filter_options_t* options) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(options);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_set_on_new_message_callback(rmw_subscription_t* subscription,
                                                       rmw_event_callback_t callback,
                                                       const void* user_data) {
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}

static rmw_ret_t take_one(rmw_zp_subscription_t* sub_data, void* ros_message, bool* taken,
                          rmw_message_info_t* message_info) {
  rmw_zp_message_t msg_data;
  if (rmw_zp_subscription_pop_next_message(sub_data, &msg_data) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  const uint8_t* payload = z_slice_data(z_loan(msg_data.payload));
  const size_t payload_len = z_slice_len(z_loan(msg_data.payload));

  if (rmw_zp_message_type_support_deserialize(sub_data->type_support, payload, payload_len,
                                              ros_message) != RMW_RET_OK) {
    z_drop(z_move(msg_data.payload));
    return RMW_RET_ERROR;
  }

  z_drop(z_move(msg_data.payload));

  if (message_info != NULL) {
    message_info->reception_sequence_number = 0;
    message_info->publisher_gid.implementation_identifier = rmw_zp_identifier;
    message_info->from_intra_process = false;
    message_info->received_timestamp = msg_data.received_timestamp;
    message_info->source_timestamp = msg_data.attachment_data.source_timestamp;
    message_info->publication_sequence_number = msg_data.attachment_data.sequence_number;
    memcpy(message_info->publisher_gid.data, msg_data.attachment_data.source_gid,
           RMW_GID_STORAGE_SIZE);
  }

  *taken = true;

  return RMW_RET_OK;
}

static rmw_ret_t take_one_serialized(rmw_zp_subscription_t* sub_data,
                                     rmw_serialized_message_t* serialized_message, bool* taken,
                                     rmw_message_info_t* message_info) {
  rmw_zp_message_t msg_data;
  if (rmw_zp_subscription_pop_next_message(sub_data, &msg_data) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  const uint8_t* payload = z_slice_data(z_loan(msg_data.payload));
  const size_t payload_len = z_slice_len(z_loan(msg_data.payload));

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
    message_info->received_timestamp = msg_data.received_timestamp;
    message_info->source_timestamp = msg_data.attachment_data.source_timestamp;
    message_info->publication_sequence_number = msg_data.attachment_data.sequence_number;
    memcpy(message_info->publisher_gid.data, msg_data.attachment_data.source_gid,
           RMW_GID_STORAGE_SIZE);
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
