#include "./guard_condition.h"

#include "rmw/error_handling.h"

rmw_ret_t rmw_zp_guard_condition_init(rmw_zp_guard_condition_t* guard_condition) {
  guard_condition->has_triggered = false;
  guard_condition->wait_set = NULL;

  if (z_mutex_init(&guard_condition->internal_mutex) < 0) {
    RMW_SET_ERROR_MSG("Failed to initialize zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_guard_condition_fini(rmw_zp_guard_condition_t* guard_condition) {
  if (z_drop(z_move(guard_condition->internal_mutex)) < 0) {
    RMW_SET_ERROR_MSG("Failed to drop zenohpico mutex");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_zp_guard_condition_trigger(rmw_zp_guard_condition_t* guard_condition) {
  z_mutex_lock(z_loan_mut(guard_condition->internal_mutex));

  // the change to has_triggered needs to be mutually exclusive with
  // rmw_wait() which checks has_triggered and decides if wait() needs to
  // be called
  guard_condition->has_triggered = true;

  if (guard_condition->wait_set != NULL) {
    z_mutex_lock(z_loan_mut(guard_condition->wait_set->condition_mutex));

    guard_condition->wait_set->triggered = true;
    
    if (z_condvar_signal(z_loan_mut(guard_condition->wait_set->condition_variable)) < 0) {
      RMW_SET_ERROR_MSG("Failed to signal condition variable.");
      z_mutex_unlock(z_loan_mut(guard_condition->wait_set->condition_mutex));
      z_mutex_unlock(z_loan_mut(guard_condition->internal_mutex));
      return RMW_RET_ERROR;
    }

    z_mutex_unlock(z_loan_mut(guard_condition->wait_set->condition_mutex));
  }

  z_mutex_unlock(z_loan_mut(guard_condition->internal_mutex));

  return RMW_RET_OK;
}

bool rmw_zp_guard_condition_check_and_attach_condition_if_not(
    rmw_zp_guard_condition_t* guard_condition, rmw_zp_wait_set_t* wait_set) {
  z_mutex_lock(z_loan_mut(guard_condition->internal_mutex));

  if (guard_condition->has_triggered) {
    return true;
  }

  guard_condition->wait_set = wait_set;

  z_mutex_unlock(z_loan_mut(guard_condition->internal_mutex));

  return false;
}

bool rmw_zp_guard_condition_detach_condition_and_is_trigger_set(
    rmw_zp_guard_condition_t* guard_condition) {
  z_mutex_lock(z_loan_mut(guard_condition->internal_mutex));

  guard_condition->wait_set = NULL;

  bool ret = guard_condition->has_triggered;

  guard_condition->has_triggered = false;

  z_mutex_unlock(z_loan_mut(guard_condition->internal_mutex));

  return ret;
}