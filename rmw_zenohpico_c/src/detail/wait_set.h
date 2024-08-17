#ifndef RMW_ZENOHPICO_DETAIL__WAIT_SET_H_
#define RMW_ZENOHPICO_DETAIL__WAIT_SET_H_

#include <stdint.h>

#include "rmw/init.h"
#include "zenoh-pico.h"

typedef struct rmw_zenohpico_wait_set_s {
  // The combination of condition_variable, condition_mutex, and triggered are used to make sure
  // there isn't a race in rmw_wait().  That could happen because of the following sequence:
  //
  // 1. Without taking a lock, rmw_wait() checks if any of the entities in the wait_set are ready,
  //    and if not attachs the condition_variable to it.
  // 2. It then takes the lock, and sleeps on the condition_variable.
  //
  // However, doing step 1 takes time, and checks in a particular order: guard_conditions, events,
  // subscriptions, services, clients. The race could happen because a subscription may come in
  // subscriptions have been checked in the above list (while services and clients are being
  // checked). In that case, rmw_wait() will unnecessarily go to sleep on the condition_variable,
  // even though something is ready. This increases the latency.
  //
  // To solve the issue, rmw_wait() still does all of the checking and attaching unlocked. However,
  // the "notify" method of each entity takes the condition_mutex lock, and in addition to kicking
  // the condition_variable it sets "triggreed" in this structure to "true". Then after rmw_wait()
  // has finished attaching, it takes the lock and sets a predicate on the condition_variable so
  // that it will quit if wait_set_data->triggered is true. Thus, if one of the entities became
  // ready after we checked, the condition_variable will notice it and not go to sleep.
  //
  // This also deals with "spurious" wakeups, where the condition_variable was woken up even though
  // nothing in this wait_set became ready.

  z_owned_condvar_t condition_variable;
  z_owned_mutex_t condition_mutex;

  bool triggered;

  rmw_context_t* context;
} rmw_zenohpico_wait_set_t;

rmw_ret_t rmw_zenohpico_wait_set_init(rmw_zenohpico_wait_set_t* wait_set);

rmw_ret_t rmw_zenohpico_wait_set_fini(rmw_zenohpico_wait_set_t* wait_set);

#endif