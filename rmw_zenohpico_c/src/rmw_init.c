#include "detail/identifier.h"
#include "detail/rmw_data_types.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/domain_id.h"
#include "rmw/error_handling.h"
#include "rmw/init.h"
#include "zenoh-pico.h"

rmw_ret_t rmw_init(const rmw_init_options_t* options, rmw_context_t* context) {
  RMW_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(options->implementation_identifier,
                              "expected initialized init options", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(options, options->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(options->enclave, "expected non-null enclave",
                              return RMW_RET_INVALID_ARGUMENT);
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

  // context->impl->enclave = rcutils_strdup(options->enclave, *allocator);
  // RMW_CHECK_FOR_NULL_WITH_MSG(
  //   context->impl->enclave,
  //   "failed to allocate enclave",
  //   return RMW_RET_BAD_ALLOC);

  // auto free_enclave = rcpputils::make_scope_exit(
  //   [context, allocator]() {
  //     allocator->deallocate(context->impl->enclave, allocator->state);
  //   });

  // Initialize context's implementation
  // context->impl->is_shutdown = false;

  // If not already defined, set the logging environment variable for Zenoh sessions
  // to warning level by default.
  // TODO(Yadunund): Switch to rcutils_get_env once it supports not overwriting values.
  // if (setenv(ZENOH_LOG_ENV_VAR_STR, ZENOH_LOG_WARN_LEVEL_STR, 0) != 0) {
  //   RMW_SET_ERROR_MSG("Error configuring Zenoh logging.");
  //   return RMW_RET_ERROR;
  // }

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