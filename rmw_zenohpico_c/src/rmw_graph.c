#include "rcutils/macros.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/rmw.h"

rmw_ret_t rmw_get_node_names(const rmw_node_t* node, rcutils_string_array_t* node_names,
                             rcutils_string_array_t* node_namespaces) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(node_names);
  RCUTILS_UNUSED(node_namespaces);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_node_names_with_enclaves(const rmw_node_t* node,
                                           rcutils_string_array_t* node_names,
                                           rcutils_string_array_t* node_namespaces,
                                           rcutils_string_array_t* enclaves) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(node_names);
  RCUTILS_UNUSED(node_namespaces);
  RCUTILS_UNUSED(enclaves);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_count_publishers(const rmw_node_t* node, const char* topic_name, size_t* count) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(topic_name);
  RCUTILS_UNUSED(count);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_count_subscribers(const rmw_node_t* node, const char* topic_name, size_t* count) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(topic_name);
  RCUTILS_UNUSED(count);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_count_clients(const rmw_node_t* node, const char* service_name, size_t* count) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(service_name);
  RCUTILS_UNUSED(count);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_count_services(const rmw_node_t* node, const char* service_name, size_t* count) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(service_name);
  RCUTILS_UNUSED(count);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_service_server_is_available(const rmw_node_t* node, const rmw_client_t* client,
                                          bool* is_available) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(client);
  RCUTILS_UNUSED(is_available);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_topic_names_and_types(const rmw_node_t* node, rcutils_allocator_t* allocator,
                                        bool no_demangle,
                                        rmw_names_and_types_t* topic_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(no_demangle);
  RCUTILS_UNUSED(topic_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_subscriber_names_and_types_by_node(const rmw_node_t* node,
                                                     rcutils_allocator_t* allocator,
                                                     const char* node_name,
                                                     const char* node_namespace, bool no_demangle,
                                                     rmw_names_and_types_t* topic_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(node_name);
  RCUTILS_UNUSED(node_namespace);
  RCUTILS_UNUSED(no_demangle);
  RCUTILS_UNUSED(topic_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_publisher_names_and_types_by_node(const rmw_node_t* node,
                                                    rcutils_allocator_t* allocator,
                                                    const char* node_name,
                                                    const char* node_namespace, bool no_demangle,
                                                    rmw_names_and_types_t* topic_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(node_name);
  RCUTILS_UNUSED(node_namespace);
  RCUTILS_UNUSED(no_demangle);
  RCUTILS_UNUSED(topic_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_service_names_and_types(const rmw_node_t* node, rcutils_allocator_t* allocator,
                                          rmw_names_and_types_t* service_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(service_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_service_names_and_types_by_node(const rmw_node_t* node,
                                                  rcutils_allocator_t* allocator,
                                                  const char* node_name, const char* node_namespace,
                                                  rmw_names_and_types_t* service_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(node_name);
  RCUTILS_UNUSED(node_namespace);
  RCUTILS_UNUSED(service_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_client_names_and_types_by_node(const rmw_node_t* node,
                                                 rcutils_allocator_t* allocator,
                                                 const char* node_name, const char* node_namespace,
                                                 rmw_names_and_types_t* service_names_and_types) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(node_name);
  RCUTILS_UNUSED(node_namespace);
  RCUTILS_UNUSED(service_names_and_types);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_publishers_info_by_topic(const rmw_node_t* node, rcutils_allocator_t* allocator,
                                           const char* topic_name, bool no_mangle,
                                           rmw_topic_endpoint_info_array_t* publishers_info) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(topic_name);
  RCUTILS_UNUSED(no_mangle);
  RCUTILS_UNUSED(publishers_info);
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_get_subscriptions_info_by_topic(const rmw_node_t* node,
                                              rcutils_allocator_t* allocator,
                                              const char* topic_name, bool no_mangle,
                                              rmw_topic_endpoint_info_array_t* subscriptions_info) {
  RCUTILS_UNUSED(node);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(topic_name);
  RCUTILS_UNUSED(no_mangle);
  RCUTILS_UNUSED(subscriptions_info);
  return RMW_RET_UNSUPPORTED;
}