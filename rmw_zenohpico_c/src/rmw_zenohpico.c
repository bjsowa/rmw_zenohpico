#include "detail/identifier.h"
#include "detail/rmw_data_types.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/domain_id.h"
#include "rmw/error_handling.h"
#include "rmw/init.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"\
#include "zenoh-pico.h"

//==============================================================================
/// Create a node and return a handle to that node.
rmw_node_t *rmw_create_node(rmw_context_t *context, const char *name, const char *namespace_) {
  RMW_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(context, context->implementation_identifier,
                                   rmw_zenohpico_identifier, return NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "expected initialized context", return NULL);
  if (context->impl->is_shutdown) {
    RCUTILS_SET_ERROR_MSG("context has been shutdown");
    return NULL;
  }

  int validation_result = RMW_NODE_NAME_VALID;
  rmw_ret_t ret = rmw_validate_node_name(name, &validation_result, NULL);
  if (RMW_RET_OK != ret) {
    return NULL;
  }
  if (RMW_NODE_NAME_VALID != validation_result) {
    const char *reason = rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node name: %s", reason);
    return NULL;
  }

  validation_result = RMW_NAMESPACE_VALID;
  ret = rmw_validate_namespace(namespace_, &validation_result, NULL);
  if (RMW_RET_OK != ret) {
    return NULL;
  }
  if (RMW_NAMESPACE_VALID != validation_result) {
    const char *reason = rmw_namespace_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node namespace: %s", reason);
    return NULL;
  }

  rcutils_allocator_t *allocator = &context->options.allocator;

  rmw_node_t *node =
      (rmw_node_t *)(allocator->zero_allocate(1, sizeof(rmw_node_t), allocator->state));
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "unable to allocate memory for rmw_node_t",
                              goto fail_allocate_node;);

  node->name = rcutils_strdup(name, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->name, "unable to allocate memory for node name",
                              goto fail_allocate_node_name;);

  node->namespace_ = rcutils_strdup(namespace_, *allocator);
  RMW_CHECK_FOR_NULL_WITH_MSG(node->namespace_, "unable to allocate memory for node namespace",
                              goto fail_allocate_node_namespace;);

  // Put metadata into node->data.
  rmw_node_data_t *node_data =
      (rmw_node_data_t *)allocator->allocate(sizeof(rmw_node_data_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_data, "failed to allocate memory for node data",
                              goto fail_allocate_node_data;);

  node->implementation_identifier = rmw_zenohpico_identifier;
  node->context = context;
  node->data = node_data;

  return node;

  allocator->deallocate(node_data, allocator->state);
fail_allocate_node_data:
  allocator->deallocate(node->namespace_, allocator->state);
fail_allocate_node_namespace:
  allocator->deallocate(node->name, allocator->state);
fail_allocate_node_name:
  allocator->deallocate(node, allocator->state);
fail_allocate_node:
  return NULL;
}

//==============================================================================
/// Finalize a given node handle, reclaim the resources, and deallocate the node
/// handle.
rmw_ret_t rmw_destroy_node(rmw_node_t *node) {
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->context->impl, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rcutils_allocator_t *allocator = &node->context->options.allocator;

  // Undeclare liveliness token for the node to advertise that the node has
  // ridden off into the sunset.
  rmw_node_data_t *node_data = (rmw_node_data_t *)node->data;
  if (node_data != NULL) {
    allocator->deallocate(node_data, allocator->state);
  }

  allocator->deallocate(node->namespace_, allocator->state);
  allocator->deallocate(node->name, allocator->state);
  allocator->deallocate(node, allocator->state);

  return RMW_RET_OK;
}