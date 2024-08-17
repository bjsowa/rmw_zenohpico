#ifndef RMW_ZENOHPICO_DETAIL__GUARD_CONDITION_H_
#define RMW_ZENOHPICO_DETAIL__GUARD_CONDITION_H_

#include <stdint.h>

#include "./wait_set.h"
#include "rmw/ret_types.h"
#include "zenoh-pico.h"

typedef struct rmw_zenohpico_guard_condition_s {
  z_owned_mutex_t internal_mutex;
  bool has_triggered;
  rmw_zenohpico_wait_set_t* wait_set;
} rmw_zenohpico_guard_condition_t;

rmw_ret_t rmw_zenohpico_guard_condition_init(rmw_zenohpico_guard_condition_t* guard_condition);

rmw_ret_t rmw_zenohpico_guard_condition_fini(rmw_zenohpico_guard_condition_t* guard_condition);

void rmw_zenohpico_guard_condition_trigger(rmw_zenohpico_guard_condition_t* guard_condition);

bool rmw_zenohpico_guard_condition_check_and_attach_condition_if_not(
    rmw_zenohpico_guard_condition_t* guard_condition, rmw_zenohpico_wait_set_t* wait_set);

bool rmw_zenohpico_guard_condition_detach_condition_and_is_trigger_set(
    rmw_zenohpico_guard_condition_t* guard_condition);

#endif