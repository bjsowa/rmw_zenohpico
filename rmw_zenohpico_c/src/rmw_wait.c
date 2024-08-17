#include "detail/guard_condition.h"
#include "detail/identifiers.h"
#include "detail/wait_set.h"
#include "rmw/check_type_identifiers_match.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "zenoh-pico.h"

static bool check_and_attach_condition(const rmw_subscriptions_t *const subscriptions,
                                       const rmw_guard_conditions_t *const guard_conditions,
                                       const rmw_services_t *const services,
                                       const rmw_clients_t *const clients,
                                       const rmw_events_t *const events,
                                       rmw_zenohpico_wait_set_t *wait_set_data) {
  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      rmw_zenohpico_guard_condition_t *gc = guard_conditions->guard_conditions[i];
      if (gc == NULL) {
        continue;
      }
      if (rmw_zenohpico_guard_condition_check_and_attach_condition_if_not(gc, wait_set_data)) {
        return true;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      rmw_event_t *event = events->events[i];
      // TODO
      //       rmw_zenoh_cpp::rmw_zenoh_event_type_t zenoh_event_type =
      //           rmw_zenoh_cpp::zenoh_event_from_rmw_event(event->event_type);
      //       if (zenoh_event_type == rmw_zenoh_cpp::ZENOH_EVENT_INVALID) {
      //         RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      //             "has_triggered_condition() called with unknown event %u. Report "
      //             "this bug.",
      //             event->event_type);
      //         continue;
      //       }

      //       auto event_data =
      //           static_cast<rmw_zenoh_cpp::EventsManager *>(event->data);
      //       if (event_data == nullptr) {
      //         continue;
      //       }

      //       if (event_data->queue_has_data_and_attach_condition_if_not(
      //               zenoh_event_type, wait_set_data)) {
      //         return true;
      //       }
    }
  }

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      // TODO
      //       auto sub_data = static_cast<rmw_zenoh_cpp::rmw_subscription_data_t *>(
      //           subscriptions->subscribers[i]);
      //       if (sub_data == nullptr) {
      //         continue;
      //       }
      //       if (sub_data->queue_has_data_and_attach_condition_if_not(wait_set_data)) {
      //         return true;
      //       }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      // TODO
      //       auto serv_data = static_cast<rmw_zenoh_cpp::rmw_service_data_t *>(
      //           services->services[i]);
      //       if (serv_data == nullptr) {
      //         continue;
      //       }
      //       if (serv_data->queue_has_data_and_attach_condition_if_not(
      //               wait_set_data)) {
      //         return true;
      //       }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      // TODO
      //       rmw_zenoh_cpp::rmw_client_data_t *client_data =
      //           static_cast<rmw_zenoh_cpp::rmw_client_data_t *>(clients->clients[i]);
      //       if (client_data == nullptr) {
      //         continue;
      //       }
      //       if (client_data->queue_has_data_and_attach_condition_if_not(
      //               wait_set_data)) {
      //         return true;
      //       }
    }
  }

  return false;
}

static size_t rmw_time_to_us(const rmw_time_t *time) {
  // TODO: handle overflows?
  return time->sec * 1000000 + time->nsec / 1000;
}

//==============================================================================
/// Waits on sets of different entities and returns when one is ready.
rmw_ret_t rmw_wait(rmw_subscriptions_t *subscriptions, rmw_guard_conditions_t *guard_conditions,
                   rmw_services_t *services, rmw_clients_t *clients, rmw_events_t *events,
                   rmw_wait_set_t *wait_set, const rmw_time_t *wait_timeout) {
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(wait set handle, wait_set->implementation_identifier,
                                   rmw_zenohpico_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_zenohpico_wait_set_t *wait_set_data = wait_set->data;
  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set_data, "waitset data struct is null", return RMW_RET_ERROR);

  // rmw_wait should return *all* entities that have data available, and let the
  // caller decide how to handle them.
  //
  // If there is no data currently available in any of the entities we were told
  // to wait on, we we attach a context-global condition variable to each
  // entity, calculate a timeout based on wait_timeout, and then sleep on the
  // condition variable.  If any of the entities has an event during that time,
  // it will wake up from that sleep.
  //
  // If there is data currently available in one or more of the entities, then
  // we'll skip attaching the condition variable, and skip the sleep, and
  // instead just go to the last part.
  //
  // In the last part, we check every entity and see if there are conditions
  // that make it ready. If that entity is not ready, then we set the pointer to
  // it to nullptr in the wait set, which signals to the upper layers that it
  // isn't ready.  If something is ready, then we leave it as a valid pointer.

  bool skip_wait = check_and_attach_condition(subscriptions, guard_conditions, services, clients,
                                              events, wait_set_data);
  if (!skip_wait) {
    z_mutex_lock(z_loan(wait_set_data->condition_mutex));

    // According to the RMW documentation, if wait_timeout is NULL that means
    // "wait forever", if it specified as 0 it means "never wait", and if it is
    // anything else wait for that amount of time.
    if (wait_timeout == NULL) {
      while (!wait_set_data->triggered) {
        z_condvar_wait(z_loan(wait_set_data->condition_variable),
                       z_loan(wait_set_data->condition_mutex));
      }
    } else {
      if (wait_timeout->sec != 0 || wait_timeout->nsec != 0) {
        const size_t wait_timeout_us = rmw_time_to_us(wait_timeout);
        const z_clock_t clock_start = z_clock_now();

        while (!wait_set_data->triggered) {
          const size_t time_elapsed_us = z_clock_elapsed_us(&clock_start);
          if (time_elapsed_us >= wait_timeout_us) {
            break;
          } else {
            z_condvar_wait_for_us(z_loan(wait_set_data->condition_variable),
                                  z_loan(wait_set_data->condition_mutex),
                                  wait_timeout_us - time_elapsed_us);
          }
        }
      }
    }

    // It is important to reset this here while still holding the lock,
    // otherwise every subsequent call to rmw_wait() will be immediately ready.
    // We could handle this another way by making "triggered" a stack variable
    // in this function and "attaching" it during "check_and_attach_condition",
    // but that isn't clearly better so leaving this.
    wait_set_data->triggered = false;
    z_mutex_unlock(z_loan(wait_set_data->condition_mutex));
  }

  bool wait_result = false;

  // According to the documentation for rmw_wait in rmw.h, entries in the
  // various arrays that have *not* been triggered should be set to NULL
  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      rmw_zenohpico_guard_condition_t *gc = guard_conditions->guard_conditions[i];
      if (gc == NULL) {
        continue;
      }
      if (!rmw_zenohpico_guard_condition_detach_condition_and_is_trigger_set(gc)) {
        // Setting to NULL lets rcl know that this guard condition is not ready
        guard_conditions->guard_conditions[i] = NULL;
      } else {
        wait_result = true;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      rmw_event_t *event = events->events[i];
      // TODO
      //       auto event_data =
      //           static_cast<rmw_zenoh_cpp::EventsManager *>(event->data);
      //       if (event_data == nullptr) {
      //         continue;
      //       }

      //       rmw_zenoh_cpp::rmw_zenoh_event_type_t zenoh_event_type =
      //           rmw_zenoh_cpp::zenoh_event_from_rmw_event(event->event_type);
      //       if (zenoh_event_type == rmw_zenoh_cpp::ZENOH_EVENT_INVALID) {
      //         continue;
      //       }

      //       if (event_data->detach_condition_and_event_queue_is_empty(
      //               zenoh_event_type)) {
      //         // Setting to nullptr lets rcl know that this subscription is not ready
      //         events->events[i] = nullptr;
      //       } else {
      //         wait_result = true;
      //       }
    }
  }

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      // TODO
      //       auto sub_data = static_cast<rmw_zenoh_cpp::rmw_subscription_data_t *>(
      //           subscriptions->subscribers[i]);
      //       if (sub_data == nullptr) {
      //         continue;
      //       }

      //       if (sub_data->detach_condition_and_queue_is_empty()) {
      //         // Setting to nullptr lets rcl know that this subscription is not ready
      //         subscriptions->subscribers[i] = nullptr;
      //       } else {
      //         wait_result = true;
      //       }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      // TODO
      //       auto serv_data = static_cast<rmw_zenoh_cpp::rmw_service_data_t *>(
      //           services->services[i]);
      //       if (serv_data == nullptr) {
      //         continue;
      //       }

      //       if (serv_data->detach_condition_and_queue_is_empty()) {
      //         // Setting to nullptr lets rcl know that this service is not ready
      //         services->services[i] = nullptr;
      //       } else {
      //         wait_result = true;
      //       }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      // TODO
      //       rmw_zenoh_cpp::rmw_client_data_t *client_data =
      //           static_cast<rmw_zenoh_cpp::rmw_client_data_t *>(clients->clients[i]);
      //       if (client_data == nullptr) {
      //         continue;
      //       }

      //       if (client_data->detach_condition_and_queue_is_empty()) {
      //         // Setting to nullptr lets rcl know that this client is not ready
      //         clients->clients[i] = nullptr;
      //       } else {
      //         wait_result = true;
      //       }
    }
  }

  return wait_result ? RMW_RET_OK : RMW_RET_TIMEOUT;
}