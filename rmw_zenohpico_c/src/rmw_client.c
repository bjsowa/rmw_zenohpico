#include "detail/attachment_helpers.h"
#include "detail/client.h"
#include "detail/identifiers.h"
#include "detail/node.h"
#include "detail/rmw_data_types.h"
#include "detail/ros_topic_name_to_zenoh_key.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

rmw_client_t* rmw_create_client(const rmw_node_t* node,
                                const rosidl_service_type_support_t* type_supports,
                                const char* service_name, const rmw_qos_profile_t* qos_profile) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return NULL);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, NULL);
  if (0 == strlen(service_name)) {
    RMW_SET_ERROR_MSG("service_name argument is an empty string");
    return NULL;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_profile, NULL);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, NULL);

  RMW_CHECK_FOR_NULL_WITH_MSG(node->context, "expected initialized context", return NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->context->impl, "expected initialized context impl",
                              return NULL);

  // rmw_context_impl_t* context_impl = node->context->impl;
  // RMW_CHECK_FOR_NULL_WITH_MSG(context_impl->enclave, "expected initialized enclave",
  // return nullptr);

  rmw_zp_node_t* node_data = node->data;
  RMW_CHECK_ARGUMENT_FOR_NULL(node_data, NULL);

  rcutils_allocator_t* allocator = &node->context->options.allocator;

  if (!qos_profile->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(service_name, &validation_result, NULL);
    if (RMW_RET_OK != ret) {
      return NULL;
    }
    if (RMW_TOPIC_VALID != validation_result) {
      const char* reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service_name argument is invalid: %s", reason);
      return NULL;
    }
  }

  rmw_client_t* rmw_client = allocator->zero_allocate(1, sizeof(rmw_client_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_client, "failed to allocate memory for the client", return NULL);

  rmw_zp_client_t* client_data =
      allocator->zero_allocate(1, sizeof(rmw_zp_client_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(client_data, "failed to allocate memory for client data",
                              goto fail_allocate_client_data);

  if (rmw_zp_client_init(client_data, qos_profile, allocator) != RMW_RET_OK) {
    goto fail_init_client_data;
  }

  z_random_fill(client_data->client_gid, RMW_GID_STORAGE_SIZE);

  client_data->context = node->context;

  client_data->type_support =
      allocator->zero_allocate(1, sizeof(rmw_zp_service_type_support_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(client_data->type_support,
                              "Failed to allocate zenohpico type support",
                              goto fail_allocate_type_support);

  if (rmw_zp_service_type_support_init(client_data->type_support, type_supports, allocator) !=
      RMW_RET_OK) {
    goto fail_init_type_support;
  }

  // Populate the rmw_client.
  rmw_client->implementation_identifier = rmw_zp_identifier;
  rmw_client->service_name = rcutils_strdup(service_name, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_client->service_name, "failed to allocate service name",
                              goto fail_allocate_service_name);

  // Convert the type hash to a string so that it can be included in
  // the keyexpr.
  char* type_hash_c_str = NULL;
  rcutils_ret_t stringify_ret = rosidl_stringify_type_hash(client_data->type_support->type_hash,
                                                           *allocator, &type_hash_c_str);
  if (RCUTILS_RET_BAD_ALLOC == stringify_ret) {
    RMW_SET_ERROR_MSG("Failed to allocate type_hash_c_str.");
    goto fail_allocate_type_hash_c_str;
  }

  client_data->keyexpr_c_str =
      ros_topic_name_to_zenoh_key(node->context->actual_domain_id, service_name,
                                  client_data->type_support->type_name, type_hash_c_str, allocator);
  if (client_data->keyexpr_c_str == NULL) {
    goto fail_create_zenoh_key;
  }

  if (z_view_keyexpr_from_str(&client_data->keyexpr, client_data->keyexpr_c_str) < 0) {
    RMW_SET_ERROR_MSG("Failed to create zenoh keyexpr");
    goto fail_create_keyexpr;
  }

  // TODO(bjsowa): liveliness tokens stuff

  rmw_client->data = client_data;

  allocator->deallocate(type_hash_c_str, allocator->state);

  return rmw_client;

fail_create_keyexpr:
  allocator->deallocate((char*)client_data->keyexpr_c_str, allocator->state);
fail_create_zenoh_key:
  allocator->deallocate(type_hash_c_str, allocator->state);
fail_allocate_type_hash_c_str:
  allocator->deallocate((char*)rmw_client->service_name, allocator->state);
fail_allocate_service_name:
  rmw_zp_service_type_support_fini(client_data->type_support, allocator);
fail_init_type_support:
  allocator->deallocate(client_data->type_support, allocator->state);
fail_allocate_type_support:
  rmw_zp_client_fini(client_data, allocator);
fail_init_client_data:
  allocator->deallocate(client_data, allocator->state);
fail_allocate_client_data:
  allocator->deallocate(rmw_client, allocator->state);
  return NULL;
}

rmw_ret_t rmw_destroy_client(rmw_node_t* node, rmw_client_t* client) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(client->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_ret_t ret = RMW_RET_OK;

  rcutils_allocator_t* allocator = &node->context->options.allocator;
  rmw_zp_client_t* client_data = client->data;

  allocator->deallocate((char*)client_data->keyexpr_c_str, allocator->state);
  allocator->deallocate((char*)client->service_name, allocator->state);

  if (rmw_zp_service_type_support_fini(client_data->type_support, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(client_data->type_support, allocator->state);

  if (rmw_zp_client_fini(client_data, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(client_data, allocator->state);
  allocator->deallocate(client, allocator->state);

  return ret;
}

rmw_ret_t rmw_client_request_publisher_get_actual_qos(const rmw_client_t* client,
                                                      rmw_qos_profile_t* qos) {
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_client_t* client_data = client->data;

  *qos = client_data->adapted_qos_profile;
  return RMW_RET_OK;
}

rmw_ret_t rmw_client_response_subscription_get_actual_qos(const rmw_client_t* client,
                                                          rmw_qos_profile_t* qos) {
  // The same QoS profile is used for sending requests and receiving responses.
  return rmw_client_request_publisher_get_actual_qos(client, qos);
}

rmw_ret_t rmw_send_request(const rmw_client_t* client, const void* ros_request,
                           int64_t* sequence_id) {
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(client->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(sequence_id, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zp_client_t* client_data = client->data;

  if (client_data->is_shutdown) {
    return RMW_RET_ERROR;
  }

  rmw_context_impl_t* context_impl = client_data->context->impl;
  rcutils_allocator_t* allocator = &(client_data->context->options.allocator);

  // Serialize request
  size_t serialized_size = rmw_zp_service_type_support_get_request_serialized_size(
      client_data->type_support, ros_request);

  uint8_t* request_bytes = allocator->allocate(serialized_size, allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(request_bytes, "failed allocate request message bytes",
                              return RMW_RET_BAD_ALLOC);

  if (rmw_zp_service_type_support_serialize_request(client_data->type_support, ros_request,
                                                    request_bytes, serialized_size) != RMW_RET_OK) {
    goto fail_serialize_ros_request;
  }

  // Create attachment
  *sequence_id = rmw_zp_client_get_next_sequence_number(client_data);

  zp_time_since_epoch time_since_epoch;
  if (zp_get_time_since_epoch(&time_since_epoch) < 0) {
    RMW_SET_ERROR_MSG("Zenoh-pico port does not support zp_get_time_since_epoch");
    goto fail_get_time_since_epoch;
  }

  int64_t source_timestamp =
      (int64_t)time_since_epoch.secs * 1000000000ll + (int64_t)time_since_epoch.nanos;

  rmw_zp_attachment_data_t attachment_data = {.sequence_number = *sequence_id,
                                              .source_timestamp = source_timestamp};
  memcpy(attachment_data.source_gid, client_data->client_gid, RMW_GID_STORAGE_SIZE);

  z_owned_bytes_t attachment;
  if (rmw_zp_attachment_data_serialize_to_zbytes(&attachment_data, &attachment) != RMW_RET_OK) {
    goto fail_serialize_attachment;
  }

  z_get_options_t opts;
  z_get_options_default(&opts);
  opts.attachment = z_move(attachment);

  // See the comment about the "num_in_flight" class variable in the
  // rmw_client_data_t class for why we need to do this.
  rmw_zp_client_increment_queries_in_flight(client_data);

  opts.target = Z_QUERY_TARGET_ALL_COMPLETE;
  // The default timeout for a z_get query is 10 seconds and if a response is
  // not received within this window, the queryable will return an invalid
  // reply. However, it is common for actions, which are implemented using
  // services, to take an extended duration to complete. Hence, we set the
  // timeout_ms to the largest supported value to account for most realistic
  // scenarios.
  opts.timeout_ms = UINT32_MAX;
  // Latest consolidation guarantees unicity of replies for the same key
  // expression, which optimizes bandwidth. The default is "None", which imples
  // replies may come in any order and any number.
  opts.consolidation = z_query_consolidation_latest();

  z_owned_bytes_t payload;
  z_bytes_from_static_buf(&payload, request_bytes, serialized_size);
  opts.payload = z_move(payload);

  z_owned_closure_reply_t callback;
  z_closure(&callback, rmw_zp_client_data_handler, rmw_zp_client_data_dropper, client_data);

  if (z_get(z_loan(context_impl->session), z_loan(client_data->keyexpr), "", z_move(callback),
            &opts) < 0) {
    RMW_SET_ERROR_MSG("Failed to send zenoh query");
    goto fail_send_zenoh_query;
  }

  allocator->deallocate(request_bytes, allocator->state);

  return RMW_RET_OK;

fail_send_zenoh_query:
  z_drop(opts.attachment);
fail_serialize_attachment:
fail_get_time_since_epoch:
fail_serialize_ros_request:
  allocator->deallocate(request_bytes, allocator->state);
  return RMW_RET_ERROR;
}

rmw_ret_t rmw_take_response(const rmw_client_t* client, rmw_service_info_t* request_header,
                            void* ros_response, bool* taken) {
  RCUTILS_UNUSED(request_header);

  *taken = false;

  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(client->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(client->service_name, "client has no service name",
                              RMW_RET_INVALID_ARGUMENT);

  rmw_zp_client_t* client_data = client->data;

  rmw_zp_message_t reply_data;
  if (rmw_zp_client_pop_next_reply(client_data, &reply_data) != RMW_RET_OK) {
    // This tells rcl that the check for a new message was done, but no messages
    // have come in yet.
    return RMW_RET_OK;
  }

  const uint8_t* payload = z_slice_data(z_loan(reply_data.payload));
  const size_t payload_len = z_slice_len(z_loan(reply_data.payload));

  if (rmw_zp_service_type_support_deserialize_response(client_data->type_support, payload,
                                                       payload_len, ros_response) != RMW_RET_OK) {
    z_drop(z_move(reply_data.payload));
    return RMW_RET_ERROR;
  }

  request_header->received_timestamp = reply_data.received_timestamp;
  request_header->request_id.sequence_number = reply_data.attachment_data.sequence_number;
  request_header->source_timestamp = reply_data.attachment_data.source_timestamp;
  memcpy(request_header->request_id.writer_guid, reply_data.attachment_data.source_gid,
         RMW_GID_STORAGE_SIZE);

  z_drop(z_move(reply_data.payload));
  *taken = true;

  return RMW_RET_OK;
}

rmw_ret_t rmw_get_gid_for_client(const rmw_client_t* client, rmw_gid_t* gid) {
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(gid, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_client_t* client_data = client->data;

  gid->implementation_identifier = rmw_zp_identifier;
  memcpy(gid->data, client_data->client_gid, RMW_GID_STORAGE_SIZE);

  return RMW_RET_OK;
}

rmw_ret_t rmw_client_set_on_new_response_callback(rmw_client_t* client,
                                                  rmw_event_callback_t callback,
                                                  const void* user_data) {
  RCUTILS_UNUSED(client);
  RCUTILS_UNUSED(callback);
  RCUTILS_UNUSED(user_data);
  return RMW_RET_UNSUPPORTED;
}