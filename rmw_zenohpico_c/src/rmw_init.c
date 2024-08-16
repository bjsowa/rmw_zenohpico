#include "detail/identifier.h"
#include "detail/rmw_data_types.h"
#include "rcutils/strdup.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/domain_id.h"
#include "rmw/error_handling.h"
#include "rmw/init.h"
#include "zenoh-pico.h"

//==============================================================================
/// Initialize the middleware with the given options, and yielding an context.
rmw_ret_t rmw_init(const rmw_init_options_t* options, rmw_context_t* context) {
  RMW_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(options->implementation_identifier,
                              "expected initialized init options", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(options, options->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (NULL != context->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected a zero-initialized context");
    return RMW_RET_INVALID_ARGUMENT;
  }

  rmw_ret_t ret;

  context->instance_id = options->instance_id;
  context->implementation_identifier = rmw_zenohpico_identifier;
  // No custom handling of RMW_DEFAULT_DOMAIN_ID. Simply use a reasonable domain id.
  context->actual_domain_id = RMW_DEFAULT_DOMAIN_ID != options->domain_id ? options->domain_id : 0u;

  const rcutils_allocator_t* allocator = &options->allocator;

  context->impl = (rmw_context_impl_t*)(allocator->zero_allocate(1, sizeof(rmw_context_impl_t),
                                                                 allocator->state));
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "failed to allocate context impl", {
    ret = RMW_RET_BAD_ALLOC;
    goto fail_allocate_context;
  });

  if ((ret = rmw_init_options_copy(options, &context->options)) != RMW_RET_OK) {
    goto fail_init_options_copy;
  }

  // Initialize context's implementation
  context->impl->is_shutdown = false;

  // Initialize the zenoh session.
  context->impl->session = z_open(z_move(context->options.impl->config));
  if (!z_session_check(&context->impl->session)) {
    RMW_SET_ERROR_MSG("Error setting up zenoh session");
    ret = RMW_RET_ERROR;
    goto fail_session_open;
  }

  return RMW_RET_OK;

  z_close(z_move(context->impl->session));
fail_session_open:
  rmw_init_options_fini(&context->options);
fail_init_options_copy:
  allocator->deallocate(context->impl, allocator->state);
fail_allocate_context:
  *context = rmw_get_zero_initialized_context();
  return ret;
}

//==============================================================================
/// Shutdown the middleware for a given context.
rmw_ret_t rmw_shutdown(rmw_context_t* context) {
  RMW_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "expected initialized context",
                              return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(context, context->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  // z_undeclare_subscriber(z_move(context->impl->graph_subscriber));
  // if (context->impl->shm_manager.has_value()) {
  // z_drop(z_move(context->impl->shm_manager.value()));
  // }
  // Close the zenoh session
  if (z_close(z_move(context->impl->session)) < 0) {
    RMW_SET_ERROR_MSG("Error while closing zenoh session");
    return RMW_RET_ERROR;
  }

  context->impl->is_shutdown = true;

  return RMW_RET_OK;
}

//==============================================================================
/// Finalize a context.
rmw_ret_t rmw_context_fini(rmw_context_t* context) {
  RMW_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "expected initialized context",
                              return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(context, context->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!context->impl->is_shutdown) {
    RCUTILS_SET_ERROR_MSG("context has not been shutdown");
    return RMW_RET_INVALID_ARGUMENT;
  }

  const rcutils_allocator_t* allocator = &context->options.allocator;

  // RMW_TRY_DESTRUCTOR(
  // static_cast<rmw_zenoh_cpp::GuardCondition*>(context->impl->graph_guard_condition->data)
  // ->~GuardCondition(),
  // rmw_zenoh_cpp::GuardCondition, );
  // allocator->deallocate(context->impl->graph_guard_condition->data, allocator->state);

  // allocator->deallocate(context->impl->graph_guard_condition, allocator->state);
  // context->impl->graph_guard_condition = nullptr;

  allocator->deallocate(context->impl, allocator->state);

  rmw_ret_t ret = rmw_init_options_fini(&context->options);

  *context = rmw_get_zero_initialized_context();

  return ret;
}
