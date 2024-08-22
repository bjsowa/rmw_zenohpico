#include "detail/subscription.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_init_subscription_allocation(const rosidl_message_type_support_t* type_support,
                                           const rosidl_runtime_c__Sequence__bound* message_bounds,
                                           rmw_subscription_allocation_t* allocation) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_fini_subscription_allocation(rmw_subscription_allocation_t* allocation) {
  return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t* rmw_create_subscription(
    const rmw_node_t* node, const rosidl_message_type_support_t* type_support,
    const char* topic_name, const rmw_qos_profile_t* qos_policies,
    const rmw_subscription_options_t* subscription_options) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t* node, rmw_subscription_t* subscription) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_count_matched_publishers(const rmw_subscription_t* subscription,
                                                    size_t* publisher_count) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_get_actual_qos(const rmw_subscription_t* subscription,
                                          rmw_qos_profile_t* qos) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_set_content_filter(
    rmw_subscription_t* subscription, const rmw_subscription_content_filter_options_t* options) {
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_get_content_filter(const rmw_subscription_t* subscription,
                                              rcutils_allocator_t* allocator,
                                              rmw_subscription_content_filter_options_t* options) {
  return RMW_RET_UNSUPPORTED;
}
