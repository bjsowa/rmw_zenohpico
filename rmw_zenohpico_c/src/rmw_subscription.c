#include "detail/identifiers.h"
#include "detail/node.h"
#include "detail/rmw_data_types.h"
#include "detail/ros_topic_name_to_zenoh_key.h"
#include "detail/subscription.h"
#include "detail/type_support.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
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

  // TODO: support transient local qos via querying subscriber
  z_subscriber_options_t sub_options;
  z_subscriber_options_default(&sub_options);
#ifdef Z_FEATURE_UNSTABLE_API
  if (qos_profile->reliability == RMW_QOS_POLICY_RELIABILITY_RELIABLE) {
    sub_options.reliability = Z_RELIABILITY_RELIABLE;
  }
#endif

  if (z_declare_subscriber(&sub_data->sub, z_loan(context_impl->session), z_loan(keyexpr),
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
  // TODO
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
