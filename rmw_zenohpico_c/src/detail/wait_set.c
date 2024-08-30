#include "./wait_set.h"

#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_wait_set_init(rmw_zp_wait_set_t* wait_set) {
  wait_set->triggered = false;

  if (z_mutex_init(&wait_set->condition_mutex) < 0) {
    RCUTILS_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  if (z_condvar_init(&wait_set->condition_variable) < 0) {
    z_drop(z_move(wait_set->condition_mutex));
    RCUTILS_SET_ERROR_MSG("Failed to initialize zenohpico condvar");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_wait_set_fini(rmw_zp_wait_set_t* wait_set) {
  rmw_ret_t ret = RMW_RET_OK;

  if (z_drop(z_move(wait_set->condition_mutex)) < 0) {
    RCUTILS_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    ret = RMW_RET_ERROR;
  }

  if (z_drop(z_move(wait_set->condition_variable)) < 0) {
    RCUTILS_SET_ERROR_MSG("Failed to drop zenohpico condvar");
    ret = RMW_RET_ERROR;
  }

  return ret;
}
