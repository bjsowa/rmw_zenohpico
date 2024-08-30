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

  if (rmw_zp_client_init(client_data, qos_profile) != RMW_RET_OK) {
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
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_client->service_name, "failed to allocate client name",
                              goto fail_allocate_client_name);

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

  allocator->deallocate(type_hash_c_str, allocator->state);

  return rmw_client;

fail_create_keyexpr:
  allocator->deallocate((char*)client_data->keyexpr_c_str, allocator->state);
fail_create_zenoh_key:
  allocator->deallocate(type_hash_c_str, allocator->state);
fail_allocate_type_hash_c_str:
  allocator->deallocate((char*)rmw_client->service_name, allocator->state);
fail_allocate_client_name:
  rmw_zp_service_type_support_fini(client_data->type_support, allocator);
fail_init_type_support:
  allocator->deallocate(client_data->type_support, allocator->state);
fail_allocate_type_support:
  rmw_zp_client_fini(client_data);
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

  if (rmw_zp_client_fini(client_data) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(client_data, allocator->state);
  allocator->deallocate(client, allocator->state);

  return ret;
}