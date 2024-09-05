#include "detail/attachment_helpers.h"
#include "detail/identifiers.h"
#include "detail/node.h"
#include "detail/publisher.h"
#include "detail/rmw_data_types.h"
#include "detail/ros_topic_name_to_zenoh_key.h"
#include "detail/type_support.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_microxrcedds_c/message_type_support.h"

//==============================================================================
/// Initialize a publisher allocation to be used with later publications.
rmw_ret_t rmw_init_publisher_allocation(const rosidl_message_type_support_t *type_support,
                                        const rosidl_runtime_c__Sequence__bound *message_bounds,
                                        rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

//==============================================================================
/// Destroy a publisher allocation object.
rmw_ret_t rmw_fini_publisher_allocation(rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

//==============================================================================
/// Create a publisher and return a handle to that publisher.
rmw_publisher_t *rmw_create_publisher(const rmw_node_t *node,
                                      const rosidl_message_type_support_t *type_supports,
                                      const char *topic_name, const rmw_qos_profile_t *qos_profile,
                                      const rmw_publisher_options_t *publisher_options) {
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
  if (!qos_profile->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(topic_name, &validation_result, NULL);
    if (RMW_RET_OK != ret) {
      return NULL;
    }
    if (RMW_TOPIC_VALID != validation_result) {
      const char *reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid topic name: %s", reason);
      return NULL;
    }
  }

  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, NULL);
  if (publisher_options->require_unique_network_flow_endpoints ==
      RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED) {
    RMW_SET_ERROR_MSG(
        "Strict requirement on unique network flow endpoints for publishers not supported");
    return NULL;
  }

  rmw_zp_node_t *node_data = node->data;
  RMW_CHECK_ARGUMENT_FOR_NULL(node_data, NULL);

  RMW_CHECK_FOR_NULL_WITH_MSG(node->context, "expected initialized context", return NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->context->impl, "expected initialized context impl",
                              return NULL);
  rmw_context_impl_t *context_impl = node->context->impl;
  RMW_CHECK_FOR_NULL_WITH_MSG(context_impl, "unable to get rmw_context_impl_t", return NULL);
  // RMW_CHECK_FOR_NULL_WITH_MSG(context_impl->enclave, "expected initialized enclave",
  // return NULL);

  rcutils_allocator_t *allocator = &node->context->options.allocator;

  // Create the publisher.
  rmw_publisher_t *rmw_publisher =
      (rmw_publisher_t *)allocator->zero_allocate(1, sizeof(rmw_publisher_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_publisher, "failed to allocate memory for the publisher",
                              return NULL);

  rmw_zp_publisher_t *publisher_data = (rmw_zp_publisher_t *)allocator->zero_allocate(
      1, sizeof(rmw_zp_publisher_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(publisher_data, "failed to allocate memory for publisher data",
                              goto fail_allocate_publisher_data);

  if (rmw_zp_publisher_init(publisher_data, qos_profile) != RMW_RET_OK) {
    goto fail_init_publisher_data;
  }

  z_random_fill(publisher_data->pub_gid, RMW_GID_STORAGE_SIZE);

  // publisher_data->type_hash = message_type_support->get_type_hash_func(message_type_support);
  // publisher_data->type_support_impl = message_type_support->data;

  publisher_data->type_support =
      allocator->zero_allocate(1, sizeof(rmw_zp_message_type_support_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(publisher_data->type_support,
                              "Failed to allocate zenohpico type support",
                              goto fail_allocate_type_support);

  if (rmw_zp_message_type_support_init(publisher_data->type_support, type_supports, allocator) !=
      RMW_RET_OK) {
    goto fail_init_type_support;
  }

  publisher_data->context = node->context;
  rmw_publisher->data = publisher_data;
  rmw_publisher->implementation_identifier = rmw_zp_identifier;
  rmw_publisher->options = *publisher_options;
  rmw_publisher->can_loan_messages = false;

  rmw_publisher->topic_name = rcutils_strdup(topic_name, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_publisher->topic_name, "Failed to allocate topic name",
                              goto fail_allocate_topic_name);

  // Convert the type hash to a string so that it can be included in the keyexpr.
  char *type_hash_c_str = NULL;
  rcutils_ret_t stringify_ret = rosidl_stringify_type_hash(publisher_data->type_support->type_hash,
                                                           *allocator, &type_hash_c_str);
  if (RCUTILS_RET_BAD_ALLOC == stringify_ret) {
    RMW_SET_ERROR_MSG("Failed to allocate type_hash_c_str.");
    goto fail_allocate_type_hash_c_str;
  }

  const char *keyexpr_c_str = ros_topic_name_to_zenoh_key(
      node->context->actual_domain_id, topic_name, publisher_data->type_support->type_name,
      type_hash_c_str, allocator);
  if (keyexpr_c_str == NULL) {
    goto fail_create_zenoh_key;
  }

  z_view_keyexpr_t keyexpr;
  z_view_keyexpr_from_str(&keyexpr, keyexpr_c_str);

  // TODO(bjsowa): Create a Publication Cache if durability is transient_local.

  // Set congestion_control to BLOCK if appropriate.
  z_publisher_options_t opts;
  z_publisher_options_default(&opts);
  opts.congestion_control = Z_CONGESTION_CONTROL_DROP;
  if (publisher_data->adapted_qos_profile.history == RMW_QOS_POLICY_HISTORY_KEEP_ALL &&
      publisher_data->adapted_qos_profile.reliability == RMW_QOS_POLICY_RELIABILITY_RELIABLE) {
    opts.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
  }

  if (z_declare_publisher(&publisher_data->pub, z_loan(context_impl->session), z_loan(keyexpr),
                          &opts) < 0) {
    RMW_SET_ERROR_MSG("unable to create zenoh publisher");
    goto fail_create_zenoh_publisher;
  }

  // TODO(bjsowa): liveliness token stuff

  allocator->deallocate((char *)keyexpr_c_str, allocator->state);
  allocator->deallocate(type_hash_c_str, allocator->state);

  return rmw_publisher;

  z_undeclare_publisher(z_move(publisher_data->pub));
fail_create_zenoh_publisher:
  allocator->deallocate((char *)keyexpr_c_str, allocator->state);
fail_create_zenoh_key:
  allocator->deallocate(type_hash_c_str, allocator->state);
fail_allocate_type_hash_c_str:
  rmw_zp_message_type_support_fini(publisher_data->type_support, allocator);
fail_init_type_support:
  allocator->deallocate(publisher_data->type_support, allocator->state);
fail_allocate_type_support:
  allocator->deallocate((char *)rmw_publisher->topic_name, allocator->state);
fail_allocate_topic_name:
  rmw_zp_publisher_fini(publisher_data);
fail_init_publisher_data:
  allocator->deallocate(publisher_data, allocator->state);
fail_allocate_publisher_data:
  allocator->deallocate(rmw_publisher, allocator->state);
  return NULL;
}

//==============================================================================
/// Finalize a given publisher handle, reclaim the resources, and deallocate the
/// publisher handle.
rmw_ret_t rmw_destroy_publisher(rmw_node_t *node, rmw_publisher_t *publisher) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(publisher, publisher->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_ret_t ret = RMW_RET_OK;

  rcutils_allocator_t *allocator = &node->context->options.allocator;
  rmw_zp_publisher_t *publisher_data = publisher->data;

  if (z_undeclare_publisher(z_move(publisher_data->pub)) < 0) {
    RMW_SET_ERROR_MSG("failed to undeclare pub");
    ret = RMW_RET_ERROR;
  }

  if (rmw_zp_message_type_support_fini(publisher_data->type_support, allocator)) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(publisher_data->type_support, allocator->state);
  allocator->deallocate((char *)publisher->topic_name, allocator->state);

  if (rmw_zp_publisher_fini(publisher_data) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(publisher_data, allocator->state);
  allocator->deallocate(publisher, allocator->state);

  return ret;
}

rmw_ret_t rmw_borrow_loaned_message(const rmw_publisher_t *publisher,
                                    const rosidl_message_type_support_t *type_support,
                                    void **ros_message) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(ros_message);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_publisher(const rmw_publisher_t *publisher,
                                                   void *loaned_message) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(loaned_message);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_count_matched_subscriptions(const rmw_publisher_t *publisher,
                                                    size_t *subscription_count) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(subscription_count);
  // TODO(bjsowa): implement once graph cache is working
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_publisher_get_actual_qos(const rmw_publisher_t *publisher, rmw_qos_profile_t *qos) {
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(publisher, publisher->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_publisher_t *pub_data = publisher->data;
  RMW_CHECK_ARGUMENT_FOR_NULL(pub_data, RMW_RET_INVALID_ARGUMENT);

  *qos = pub_data->adapted_qos_profile;
  return RMW_RET_OK;
}

rmw_ret_t rmw_publisher_assert_liveliness(const rmw_publisher_t *publisher) {
  RCUTILS_UNUSED(publisher);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_wait_for_all_acked(const rmw_publisher_t *publisher,
                                           rmw_time_t wait_timeout) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(wait_timeout);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_gid_for_publisher(const rmw_publisher_t *publisher, rmw_gid_t *gid) {
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(gid, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_publisher_t *pub_data = publisher->data;
  RMW_CHECK_ARGUMENT_FOR_NULL(pub_data, RMW_RET_INVALID_ARGUMENT);

  gid->implementation_identifier = rmw_zp_identifier;
  memcpy(gid->data, pub_data->pub_gid, RMW_GID_STORAGE_SIZE);

  return RMW_RET_OK;
}

rmw_ret_t rmw_publish(const rmw_publisher_t *publisher, const void *ros_message,
                      rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(allocation);

  RMW_CHECK_FOR_NULL_WITH_MSG(publisher, "publisher handle is null",
                              return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(publisher, publisher->implementation_identifier,
                                   rmw_zp_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros message handle is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rmw_zp_publisher_t *publisher_data = publisher->data;
  RMW_CHECK_FOR_NULL_WITH_MSG(publisher_data, "publisher_data is null",
                              return RMW_RET_INVALID_ARGUMENT);

  rcutils_allocator_t *allocator = &(publisher_data->context->options.allocator);

  // Serialize data.
  size_t serialized_size =
      rmw_zp_message_type_support_get_serialized_size(publisher_data->type_support, ros_message);

  // To store serialized message byte array.
  uint8_t *msg_bytes = allocator->allocate(serialized_size, allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(msg_bytes, "bytes for message is null", return RMW_RET_BAD_ALLOC);

  if (rmw_zp_message_type_support_serialize(publisher_data->type_support, ros_message, msg_bytes,
                                            serialized_size) != RMW_RET_OK) {
    goto fail_serialize_ros_message;
  }

  // create attachment
  int64_t sequence_number = rmw_zp_publisher_get_next_sequence_number(publisher_data);

  zp_time_since_epoch time_since_epoch;
  if (zp_get_time_since_epoch(&time_since_epoch) < 0) {
    RMW_SET_ERROR_MSG("Zenoh-pico port does not support zp_get_time_since_epoch");
    goto fail_get_time_since_epoch;
  }

  int64_t source_timestamp =
      (int64_t)time_since_epoch.secs * 1000000000ll + (int64_t)time_since_epoch.nanos;

  rmw_zp_attachment_data_t attachment_data = {.sequence_number = sequence_number,
                                              .source_timestamp = source_timestamp};
  memcpy(attachment_data.source_gid, publisher_data->pub_gid, RMW_GID_STORAGE_SIZE);

  z_owned_bytes_t attachment;
  if (rmw_zp_attachment_data_serialize_to_zbytes(&attachment_data, &attachment) != RMW_RET_OK) {
    goto fail_serialize_attachment;
  }

  // The encoding is simply forwarded and is useful when key expressions in the
  // session use different encoding formats. In our case, all key expressions
  // will be encoded with CDR so it does not really matter.
  z_publisher_put_options_t options;
  z_publisher_put_options_default(&options);
  options.attachment = z_move(attachment);

  z_owned_bytes_t payload;
  z_bytes_from_static_buf(&payload, msg_bytes, serialized_size);

  if (z_publisher_put(z_loan(publisher_data->pub), z_move(payload), &options)) {
    RMW_SET_ERROR_MSG("unable to publish message");
    goto fail_publish_message;
  }

  allocator->deallocate(msg_bytes, allocator->state);

  return RMW_RET_OK;

fail_publish_message:
  z_drop(options.attachment);
fail_serialize_attachment:
fail_get_time_since_epoch:
fail_serialize_ros_message:
  allocator->deallocate(msg_bytes, allocator->state);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_publish_loaned_message(const rmw_publisher_t *publisher, void *ros_message,
                                     rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publish_serialized_message(const rmw_publisher_t *publisher,
                                         const rmw_serialized_message_t *serialized_message,
                                         rmw_publisher_allocation_t *allocation) {
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(serialized_message);
  RCUTILS_UNUSED(allocation);
  return RMW_RET_ERROR;
}