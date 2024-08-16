#include "detail/defaults.h"
#include "detail/identifier.h"
#include "detail/rmw_data_types.h"
#include "rcutils/allocator.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/init_options.h"
#include "zenoh-pico.h"

//==============================================================================
/// Initialize given init options with the default values and implementation specific values.
rmw_ret_t rmw_init_options_init(rmw_init_options_t* init_options, rcutils_allocator_t allocator) {
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  if (NULL != init_options->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }

  init_options->instance_id = 0;
  init_options->implementation_identifier = rmw_zenohpico_identifier;
  init_options->allocator = allocator;
  init_options->enclave = "/";
  init_options->domain_id = RMW_DEFAULT_DOMAIN_ID;
  init_options->security_options = rmw_get_default_security_options();
  init_options->localhost_only = RMW_LOCALHOST_ONLY_DEFAULT;
  init_options->discovery_options = rmw_get_zero_initialized_discovery_options();

  rmw_ret_t ret = rmw_discovery_options_init(&(init_options->discovery_options), 0, &allocator);
  if (RMW_RET_OK != ret) {
    goto fail_discovery_options;
  }

  init_options->impl = (rmw_init_options_impl_t*)(allocator.zero_allocate(
      1, sizeof(rmw_init_options_impl_t), allocator.state));
  RMW_CHECK_FOR_NULL_WITH_MSG(init_options->impl, "failed to allocate init_options impl", {
    ret = RMW_RET_BAD_ALLOC;
    goto fail_allocate_init_options_impl;
  });

  z_config_new(&init_options->impl->config);

  if (zp_config_insert(z_loan_mut(init_options->impl->config), Z_CONFIG_MODE_KEY,
                       rmw_zenohpico_default_mode) < 0 ||
      zp_config_insert(z_loan_mut(init_options->impl->config), Z_CONFIG_CONNECT_KEY,
                       rmw_zenohpico_default_clocator) < 0) {
    ret = RMW_RET_ERROR;
    goto fail_z_config_insert;
  }

  return RMW_RET_OK;

fail_z_config_insert:
  z_config_drop(z_move(init_options->impl->config));
  allocator.deallocate(init_options->impl, allocator.state);
fail_allocate_init_options_impl:
  rmw_discovery_options_fini(&(init_options->discovery_options));
fail_discovery_options:
  return ret;
}

//==============================================================================
/// Copy the given source init options to the destination init options.
rmw_ret_t rmw_init_options_copy(const rmw_init_options_t* src, rmw_init_options_t* dst) {
  RMW_CHECK_ARGUMENT_FOR_NULL(src, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(dst, RMW_RET_INVALID_ARGUMENT);
  if (NULL == src->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(src, src->implementation_identifier, rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (NULL != dst->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }
  rcutils_allocator_t allocator = src->allocator;
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);

  rmw_init_options_t tmp;
  tmp.instance_id = src->instance_id;
  tmp.implementation_identifier = rmw_zenohpico_identifier;
  tmp.domain_id = src->domain_id;
  tmp.security_options = rmw_get_zero_initialized_security_options();
  rmw_ret_t ret =
      rmw_security_options_copy(&src->security_options, &allocator, &tmp.security_options);
  if (RMW_RET_OK != ret) {
    goto fail_security_options;
  }

  tmp.localhost_only = src->localhost_only;
  tmp.discovery_options = rmw_get_zero_initialized_discovery_options();
  ret = rmw_discovery_options_copy(&src->discovery_options, &allocator, &tmp.discovery_options);
  if (RMW_RET_OK != ret) {
    goto fail_discovery_options;
  }

  tmp.enclave = NULL;
  if (NULL != src->enclave) {
    tmp.enclave = rcutils_strdup(src->enclave, allocator);
    if (NULL == tmp.enclave) {
      ret = RMW_RET_BAD_ALLOC;
      goto fail_enclave;
    }
  }

  tmp.allocator = src->allocator;

  tmp.impl = (rmw_init_options_impl_t*)(allocator.allocate(sizeof(rmw_init_options_impl_t),
                                                           allocator.state));
  RMW_CHECK_FOR_NULL_WITH_MSG(tmp.impl, "failed to allocate init_options impl", {
    ret = RMW_RET_BAD_ALLOC;
    goto fail_allocate_init_options_impl;
  });

  // TODO is there a way to copy it?
  tmp.impl->config = src->impl->config;

  *dst = tmp;

  return RMW_RET_OK;

fail_allocate_init_options_impl:
  allocator.deallocate(tmp.enclave, allocator.state);
fail_enclave:
  rmw_discovery_options_fini(&tmp.discovery_options);
fail_discovery_options:
  rmw_security_options_fini(&tmp.security_options, &allocator);
fail_security_options:
  return ret;
}

//==============================================================================
/// Finalize the given init options.
rmw_ret_t rmw_init_options_fini(rmw_init_options_t* init_options) {
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  if (NULL == init_options->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(init_options, init_options->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  rcutils_allocator_t* allocator = &init_options->allocator;
  RCUTILS_CHECK_ALLOCATOR(allocator, return RMW_RET_INVALID_ARGUMENT);

  allocator->deallocate(init_options->enclave, allocator->state);
  rmw_ret_t ret = rmw_security_options_fini(&init_options->security_options, allocator);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  ret = rmw_discovery_options_fini(&init_options->discovery_options);
  *init_options = rmw_get_zero_initialized_init_options();

  return ret;
}