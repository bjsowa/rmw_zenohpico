#include "detail/identifier.h"
#include "detail/wait_set.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

//==============================================================================
/// Create a wait set to store conditions that the middleware can wait on.
rmw_wait_set_t *rmw_create_wait_set(rmw_context_t *context, size_t max_conditions) {
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(context, context->implementation_identifier,
                                   rmw_zenohpico_identifier, return NULL);

  rcutils_allocator_t *allocator = &context->options.allocator;

  rmw_wait_set_t *wait_set =
      (rmw_wait_set_t *)allocator->zero_allocate(1, sizeof(rmw_wait_set_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set, "failed to allocate wait set", return NULL);

  wait_set->implementation_identifier = rmw_zenohpico_identifier;

  rmw_zenohpico_wait_set_t *wait_set_data =
      allocator->zero_allocate(1, sizeof(rmw_zenohpico_wait_set_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set->data, "failed to allocate wait set data",
                              goto fail_allocate_wait_set_data);

  if (rmw_zenohpico_wait_set_init(wait_set_data) != RMW_RET_OK) {
    goto fail_init_wait_set_data;
  }

  wait_set_data->context = context;
  wait_set->data = wait_set_data;

  return wait_set;

  rmw_zenohpico_wait_set_fini(wait_set_data);
fail_init_wait_set_data:
  allocator->deallocate(wait_set->data, allocator->state);
fail_allocate_wait_set_data:
  allocator->deallocate(wait_set, allocator->state);
  return NULL;
}

//==============================================================================
/// Destroy a wait set.
rmw_ret_t rmw_destroy_wait_set(rmw_wait_set_t *wait_set) {
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set->data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(wait_set, wait_set->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zenohpico_wait_set_t *wait_set_data = wait_set->data;
  rcutils_allocator_t *allocator = &wait_set_data->context->options.allocator;

  rmw_ret_t ret = RMW_RET_OK;

  ret = rmw_zenohpico_wait_set_fini(wait_set_data);
  allocator->deallocate(wait_set_data, allocator->state);

  allocator->deallocate(wait_set, allocator->state);

  return ret;
}
