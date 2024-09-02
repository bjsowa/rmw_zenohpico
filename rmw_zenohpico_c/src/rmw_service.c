#include "detail/identifiers.h"
#include "detail/node.h"
#include "detail/rmw_data_types.h"
#include "detail/ros_topic_name_to_zenoh_key.h"
#include "detail/service.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

rmw_service_t* rmw_create_service(const rmw_node_t* node,
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

  rmw_context_impl_t* context_impl = node->context->impl;
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

  rmw_service_t* rmw_service = allocator->zero_allocate(1, sizeof(rmw_service_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_service, "failed to allocate memory for the client", return NULL);

  rmw_zp_service_t* service_data =
      allocator->zero_allocate(1, sizeof(rmw_zp_service_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(service_data, "failed to allocate memory for service data",
                              goto fail_allocate_service_data);

  if (rmw_zp_service_init(service_data, qos_profile, allocator) != RMW_RET_OK) {
    goto fail_init_service_data;
  }

  service_data->context = node->context;

  service_data->type_support =
      allocator->zero_allocate(1, sizeof(rmw_zp_service_type_support_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(service_data->type_support,
                              "Failed to allocate zenohpico type support",
                              goto fail_allocate_type_support);

  if (rmw_zp_service_type_support_init(service_data->type_support, type_supports, allocator) !=
      RMW_RET_OK) {
    goto fail_init_type_support;
  }

  // Populate the rmw_service.
  rmw_service->implementation_identifier = rmw_zp_identifier;
  rmw_service->service_name = rcutils_strdup(service_name, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(rmw_service->service_name, "failed to allocate service name",
                              goto fail_allocate_service_name);

  // Convert the type hash to a string so that it can be included in
  // the keyexpr.
  char* type_hash_c_str = NULL;
  rcutils_ret_t stringify_ret = rosidl_stringify_type_hash(service_data->type_support->type_hash,
                                                           *allocator, &type_hash_c_str);
  if (RCUTILS_RET_BAD_ALLOC == stringify_ret) {
    RMW_SET_ERROR_MSG("Failed to allocate type_hash_c_str.");
    goto fail_allocate_type_hash_c_str;
  }

  service_data->keyexpr_c_str = ros_topic_name_to_zenoh_key(
      node->context->actual_domain_id, service_name, service_data->type_support->type_name,
      type_hash_c_str, allocator);
  if (service_data->keyexpr_c_str == NULL) {
    goto fail_create_zenoh_key;
  }

  if (z_view_keyexpr_from_str(&service_data->keyexpr, service_data->keyexpr_c_str) < 0) {
    RMW_SET_ERROR_MSG("Failed to create zenoh keyexpr");
    goto fail_create_keyexpr;
  }

  z_owned_closure_query_t callback;
  z_closure(&callback, rmw_zp_service_data_handler, NULL, service_data);
  // Configure the queryable to process complete queries.
  z_queryable_options_t qable_options;
  z_queryable_options_default(&qable_options);
  qable_options.complete = true;
  if (z_declare_queryable(&service_data->qable, z_loan(context_impl->session),
                          z_loan(service_data->keyexpr), z_move(callback), &qable_options) < 0) {
    RMW_SET_ERROR_MSG("unable to create zenoh queryable");
    goto fail_create_zenoh_queryable;
  }

  // TODO(bjsowa): liveliness tokens stuff

  rmw_service->data = service_data;

  allocator->deallocate(type_hash_c_str, allocator->state);

  return rmw_service;

  z_undeclare_queryable(z_move(service_data->qable));
fail_create_zenoh_queryable:
fail_create_keyexpr:
  allocator->deallocate((char*)service_data->keyexpr_c_str, allocator->state);
fail_create_zenoh_key:
  allocator->deallocate(type_hash_c_str, allocator->state);
fail_allocate_type_hash_c_str:
  allocator->deallocate((char*)rmw_service->service_name, allocator->state);
fail_allocate_service_name:
  rmw_zp_service_type_support_fini(service_data->type_support, allocator);
fail_init_type_support:
  allocator->deallocate(service_data->type_support, allocator->state);
fail_allocate_type_support:
  rmw_zp_service_fini(service_data, allocator);
fail_init_service_data:
  allocator->deallocate(service_data, allocator->state);
fail_allocate_service_data:
  allocator->deallocate(rmw_service, allocator->state);
  return NULL;
}

rmw_ret_t rmw_destroy_service(rmw_node_t* node, rmw_service_t* service) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(service->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service, service->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rcutils_allocator_t* allocator = &node->context->options.allocator;
  rmw_zp_service_t* service_data = service->data;

  rmw_ret_t ret = RMW_RET_OK;

  if (z_undeclare_queryable(z_move(service_data->qable)) < 0) {
    RMW_SET_ERROR_MSG("Failed to undeclare zenoh queryable");
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate((char*)service_data->keyexpr_c_str, allocator->state);
  allocator->deallocate((char*)service->service_name, allocator->state);

  if (rmw_zp_service_type_support_fini(service_data->type_support, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(service_data->type_support, allocator->state);

  if (rmw_zp_service_fini(service_data, allocator) != RMW_RET_OK) {
    ret = RMW_RET_ERROR;
  }

  allocator->deallocate(service_data, allocator->state);
  allocator->deallocate(service, allocator->state);

  return ret;
}

rmw_ret_t rmw_service_request_subscription_get_actual_qos(const rmw_service_t* service,
                                                          rmw_qos_profile_t* qos) {
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(service->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service, service->implementation_identifier, rmw_zp_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_zp_service_t* service_data = service->data;
  *qos = service_data->adapted_qos_profile;

  return RMW_RET_OK;
}

rmw_ret_t rmw_service_response_publisher_get_actual_qos(const rmw_service_t* service,
                                                        rmw_qos_profile_t* qos) {
  // The same QoS profile is used for receiving requests and sending responses.
  return rmw_service_request_subscription_get_actual_qos(service, qos);
}

rmw_ret_t rmw_take_request(const rmw_service_t* service, rmw_service_info_t* request_header,
                           void* ros_request, bool* taken) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_send_response(const rmw_service_t* service, rmw_request_id_t* request_header,
                            void* ros_response) {
  return RMW_RET_UNSUPPORTED;
}
