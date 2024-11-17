#ifndef RMW_ZENOHPICO_DETAIL__GUARD_CONDITION_H_
#define RMW_ZENOHPICO_DETAIL__GUARD_CONDITION_H_

#include <stdint.h>

#include "./wait_set.h"
#include "rmw/ret_types.h"
#include "zenoh-pico.h"

typedef struct {
  z_owned_mutex_t internal_mutex;
  bool has_triggered;
  rmw_zp_wait_set_t* wait_set;
} rmw_zp_guard_condition_t;

rmw_ret_t rmw_zp_guard_condition_init(rmw_zp_guard_condition_t* guard_condition);

rmw_ret_t rmw_zp_guard_condition_fini(rmw_zp_guard_condition_t* guard_condition);

rmw_ret_t rmw_zp_guard_condition_trigger(rmw_zp_guard_condition_t* guard_condition);

bool rmw_zp_guard_condition_check_and_attach_condition_if_not(
    rmw_zp_guard_condition_t* guard_condition, rmw_zp_wait_set_t* wait_set);

bool rmw_zp_guard_condition_detach_condition_and_is_trigger_set(
    rmw_zp_guard_condition_t* guard_condition);

#endif