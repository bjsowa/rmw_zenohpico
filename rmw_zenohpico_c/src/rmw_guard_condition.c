#include "detail/guard_condition.h"
#include "detail/identifier.h"
#include "detail/rmw_data_types.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/types.h"

//==============================================================================
/// Create a guard condition and return a handle to that guard condition.
rmw_guard_condition_t *rmw_create_guard_condition(rmw_context_t *context) {
  rcutils_allocator_t *allocator = &context->options.allocator;

  rmw_guard_condition_t *guard_condition = (rmw_guard_condition_t *)allocator->zero_allocate(
      1, sizeof(rmw_guard_condition_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(guard_condition, "unable to allocate memory for guard_condition",
                              return NULL);

  guard_condition->implementation_identifier = rmw_zenohpico_identifier;
  guard_condition->context = context;

  guard_condition->data =
      allocator->zero_allocate(1, sizeof(rmw_zenohpico_guard_condition_t), allocator->state);
  RMW_CHECK_FOR_NULL_WITH_MSG(guard_condition->data,
                              "unable to allocate memory for guard condition data",
                              goto fail_allocate_guard_condition_data;);

  rmw_zenohpico_guard_condition_t *guard_condition_data = guard_condition->data;

  if (rmw_zenohpico_guard_condition_init(guard_condition_data) != RMW_RET_OK) {
    goto fail_init_guard_condition_data;
  }

  return guard_condition;

fail_init_guard_condition_data:
  allocator->deallocate(guard_condition->data, allocator->state);
fail_allocate_guard_condition_data:
  allocator->deallocate(guard_condition, allocator->state);
  return NULL;
}

/// Finalize a given guard condition handle, reclaim the resources, and
/// deallocate the handle.
rmw_ret_t rmw_destroy_guard_condition(rmw_guard_condition_t *guard_condition) {
  RMW_CHECK_ARGUMENT_FOR_NULL(guard_condition, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(guard_condition, guard_condition->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rcutils_allocator_t *allocator = &guard_condition->context->options.allocator;

  if (guard_condition->data) {
    rmw_zenohpico_guard_condition_t *guard_condition_data = guard_condition->data;
    rmw_zenohpico_guard_condition_fini(guard_condition_data);
    allocator->deallocate(guard_condition->data, allocator->state);
  }

  allocator->deallocate(guard_condition, allocator->state);
  return RMW_RET_OK;
}

//==============================================================================
rmw_ret_t rmw_trigger_guard_condition(const rmw_guard_condition_t *guard_condition) {
  RMW_CHECK_ARGUMENT_FOR_NULL(guard_condition, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(guard_condition, guard_condition->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zenohpico_guard_condition_t *guard_condition_data = guard_condition->data;

  rmw_zenohpico_guard_condition_trigger(guard_condition_data);

  return RMW_RET_OK;
}